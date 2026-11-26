--------------------------- MODULE ProfileGet ---------------------------
(***************************************************************************)
(* Models of the COSEM profile GET in nd_ic_profile_generic.c.              *)
(*                                                                          *)
(* SnapshotSpec, checked by ProfileGet.cfg, is the implemented protocol.    *)
(* nd_ic_profile_generic_get takes one nd_profile_store_snapshot after the  *)
(* parse of the selector and before count_rows. Both passes derive every    *)
(* slot from the snapshot's head/entries and validate the sequence stored   *)
(* in the record header against snapshot->next_sequence - entries + index.  *)
(* An append with room writes outside the snapshot window and leaves the    *)
(* response identical to the snapshot, unless it starts at a sector-aligned *)
(* head: append first erases the whole sector at head, so every other slot  *)
(* of that sector becomes blank. A snapshot-window slot in that sector the  *)
(* current pass has not read yet makes read_at reject it, and GET returns   *)
(* operation-not-possible; slots the pass already read stay in the          *)
(* response. An append that wraps a full ring overwrites a slot in the      *)
(* snapshot window, so its sequence changes and the read fails the same     *)
(* way; the client retries. A reset between the passes writes a mark at     *)
(* head and bumps next_sequence. If the mark and the sector it erases lie   *)
(* outside the snapshot's read window, the old records stay physically      *)
(* present, so the response is still the consistent pre-reset snapshot.     *)
(* When head is sector-aligned, reset first erases the whole sector         *)
(* starting at head, so the erase blanks every slot of that sector, not     *)
(* just the mark slot. A mark or a blank slot the current pass has not      *)
(* read yet makes read_at reject it, and GET returns operation-not-         *)
(* possible. Slots the current pass already emitted are not read again, so  *)
(* they leave the response the pre-reset snapshot. For a value-range        *)
(* selector both passes re-read every slot, so a count pass that already    *)
(* read a slot does not protect the emit pass.                              *)
(*                                                                          *)
(* Abstraction: sequence numbers are unbounded Naturals here, while the     *)
(* implementation stores them in 16 bits and compares modulo 65536. The     *)
(* model therefore does not cover the alias where a slot is rewritten       *)
(* exactly 65,536 appends after the snapshot and its stored sequence again  *)
(* matches the predicted value; that residual case is documented in         *)
(* docs/design/record-retrieval.md, not proved here.                        *)
(*                                                                          *)
(* Proved by TLC over the bounded instance:                                 *)
(*   - TypeOK.                                                              *)
(*   - CountEqualsEmitted: the array header written from the count pass is  *)
(*     an upper bound on emitted rows and exactly equals them when the GET  *)
(*     completes (mode = "idle").                                           *)
(*                                                                          *)
(* LegacySpec and LegacyConcurrentSpec keep the old two-pass protocol as    *)
(* documentation: count_rows captures entries, then write_rows re-reads     *)
(* nd_profile_store_entries and scans again, with no snapshot and no        *)
(* sequence check. LegacyConcurrentSpec still has the counterexample:       *)
(* append one matching record while mode = "counted"; the emit pass takes   *)
(* span = 2 and emits two rows under header one. Those two specs are        *)
(* defined but not checked by the cfg; SnapshotSpec is what stops the race. *)
(***************************************************************************)
EXTENDS Naturals

CONSTANTS Records, Matching, Capacity, SectorSlots

ASSUME Matching \subseteq Records
ASSUME Capacity \in Nat /\ Capacity > 0
ASSUME SectorSlots \in Nat /\ SectorSlots > 0

\* nd_profile_store.c: ceil(capacity / sector_slots) slots plus one spare
\* sector keeps the erase target outside the live window.
Phys == ((Capacity + SectorSlots - 1) \div SectorSlots) * SectorSlots
          + SectorSlots

Empty == 0

VARIABLES
    slots,          \* record per physical slot; Empty = unwritten
    seqs,           \* sequence written with the slot's record
    head,           \* next write slot
    entries,        \* live records, at most Capacity
    next_sequence,  \* sequence of the next append or reset mark
    mode,           \* "idle", "count", "counted", "emit", "aborted"
    cursor,         \* index read in the current pass
    span,           \* legacy pass length; live entries at pass start
    snap_head,      \* snapshot fixed before the count pass
    snap_entries,
    snap_next,
    count,          \* matches counted; written as the array header
    emitted         \* rows write_rows has emitted so far

vars == <<slots, seqs, head, entries, next_sequence, mode, cursor, span,
          snap_head, snap_entries, snap_next, count, emitted>>

Init ==
    /\ slots = [i \in 0..Phys-1 |-> Empty]
    /\ seqs = [i \in 0..Phys-1 |-> 0]
    /\ head = 0
    /\ entries = 0
    /\ next_sequence = 1
    /\ mode = "idle"
    /\ cursor = 0
    /\ span = 0
    /\ snap_head = 0
    /\ snap_entries = 0
    /\ snap_next = 1
    /\ count = 0
    /\ emitted = 0

\* The snapshot read: same slot derivation for both passes, plus the stored
\* sequence must still be the one the snapshot predicts.
SnapSlot(i) == (snap_head + Phys - snap_entries + i) % Phys
SnapExpected(i) == snap_next - snap_entries + i
SnapReadOK(i) ==
    /\ slots[SnapSlot(i)] # Empty
    /\ seqs[SnapSlot(i)] = SnapExpected(i)

Start ==
    /\ mode = "idle"
    /\ mode' = "count"
    /\ cursor' = 0
    /\ snap_head' = head
    /\ snap_entries' = entries
    /\ snap_next' = next_sequence
    /\ count' = 0
    /\ emitted' = 0
    /\ UNCHANGED <<slots, seqs, head, entries, next_sequence, span>>

CountStep ==
    /\ mode = "count"
    /\ cursor < snap_entries
    /\ SnapReadOK(cursor)
    /\ count' = count
               + (IF slots[SnapSlot(cursor)] \in Matching THEN 1 ELSE 0)
    /\ cursor' = cursor + 1
    /\ UNCHANGED <<slots, seqs, head, entries, next_sequence, mode, span,
                   snap_head, snap_entries, snap_next, emitted>>

CountAbort ==
    /\ mode = "count"
    /\ cursor < snap_entries
    /\ ~SnapReadOK(cursor)
    /\ mode' = "aborted"
    /\ UNCHANGED <<slots, seqs, head, entries, next_sequence, cursor, span,
                   snap_head, snap_entries, snap_next, count, emitted>>

CountDone ==
    /\ mode = "count"
    /\ cursor = snap_entries
    /\ mode' = "counted"
    /\ UNCHANGED <<slots, seqs, head, entries, next_sequence, cursor, span,
                   snap_head, snap_entries, snap_next, count, emitted>>

\* The emit pass reuses the snapshot; it does not re-read entries.
EmitStart ==
    /\ mode = "counted"
    /\ mode' = "emit"
    /\ cursor' = 0
    /\ UNCHANGED <<slots, seqs, head, entries, next_sequence, span,
                   snap_head, snap_entries, snap_next, count, emitted>>

EmitStep ==
    /\ mode = "emit"
    /\ cursor < snap_entries
    /\ SnapReadOK(cursor)
    /\ emitted' = emitted
                  + (IF slots[SnapSlot(cursor)] \in Matching THEN 1 ELSE 0)
    /\ cursor' = cursor + 1
    /\ UNCHANGED <<slots, seqs, head, entries, next_sequence, mode, span,
                   snap_head, snap_entries, snap_next, count>>

EmitAbort ==
    /\ mode = "emit"
    /\ cursor < snap_entries
    /\ ~SnapReadOK(cursor)
    /\ mode' = "aborted"
    /\ UNCHANGED <<slots, seqs, head, entries, next_sequence, cursor, span,
                   snap_head, snap_entries, snap_next, count, emitted>>

EmitDone ==
    /\ mode = "emit"
    /\ cursor = snap_entries
    /\ mode' = "idle"
    /\ UNCHANGED <<slots, seqs, head, entries, next_sequence, cursor, span,
                   snap_head, snap_entries, snap_next, count, emitted>>

\* The failed GET's partial response is discarded and the client retries.
AbortDone ==
    /\ mode = "aborted"
    /\ mode' = "idle"
    /\ cursor' = 0
    /\ count' = 0
    /\ emitted' = 0
    /\ UNCHANGED <<slots, seqs, head, entries, next_sequence, span,
                   snap_head, snap_entries, snap_next>>

\* A second actor may append or reset at any point, including mid-GET.
\* nd_profile_store_append erases the sector that starts at head when head is
\* sector-aligned, then writes the record at head.
SectorBase == (head \div SectorSlots) * SectorSlots

AppendSlots(r) ==
    IF head % SectorSlots = 0
    THEN [i \in 0..Phys-1 |->
              IF i >= SectorBase /\ i < SectorBase + SectorSlots
              THEN IF i = head THEN r ELSE Empty
              ELSE slots[i]]
    ELSE [slots EXCEPT ![head] = r]

AppendRecord(r) ==
    /\ r \in Records
    /\ slots' = AppendSlots(r)
    /\ seqs' = [seqs EXCEPT ![head] = next_sequence]
    /\ head' = (head + 1) % Phys
    /\ entries' = IF entries < Capacity THEN entries + 1 ELSE entries
    /\ next_sequence' = next_sequence + 1
    /\ UNCHANGED <<mode, cursor, span, snap_head, snap_entries, snap_next,
                   count, emitted>>

\* nd_profile_store_reset erases the sector that starts at head when head is
\* sector-aligned, then writes a mark at head with the current sequence. The
\* mark is not a record: modeling it as Empty makes SnapReadOK false. A
\* non-aligned reset reaches the mark through the sector store's
\* read-modify-erase-write, which rewrites the whole sector buffer, so the
\* other slots keep their bytes and only the mark slot changes.
ResetSlots ==
    IF head % SectorSlots = 0
    THEN [i \in 0..Phys-1 |->
              IF i >= SectorBase /\ i < SectorBase + SectorSlots
              THEN Empty
              ELSE slots[i]]
    ELSE [slots EXCEPT ![head] = Empty]

Reset ==
    /\ entries' = 0
    /\ slots' = ResetSlots
    /\ seqs' = [seqs EXCEPT ![head] = next_sequence]
    /\ head' = (head + 1) % Phys
    /\ next_sequence' = next_sequence + 1
    /\ UNCHANGED <<mode, cursor, span, snap_head, snap_entries, snap_next,
                   count, emitted>>

Next ==
    \/ Start
    \/ CountStep \/ CountAbort \/ CountDone
    \/ EmitStart \/ EmitStep \/ EmitAbort \/ EmitDone
    \/ AbortDone
    \/ \E r \in Records: AppendRecord(r)
    \/ Reset

Spec == Init /\ [][Next]_vars

(***************************************************************************)
(* Legacy two-pass protocol, kept as the counterexample documented above.   *)
(***************************************************************************)
LegacySlot(i) == (head + Phys - entries + i) % Phys
LegacyReadOK(i) == i < entries /\ slots[LegacySlot(i)] # Empty

LegacyStart ==
    /\ mode = "idle"
    /\ mode' = "count"
    /\ cursor' = 0
    /\ span' = entries
    /\ count' = 0
    /\ emitted' = 0
    /\ UNCHANGED <<slots, seqs, head, entries, next_sequence, snap_head,
                   snap_entries, snap_next>>

LegacyCountStep ==
    /\ mode = "count"
    /\ cursor < span
    /\ LegacyReadOK(cursor)
    /\ count' = count
               + (IF slots[LegacySlot(cursor)] \in Matching THEN 1 ELSE 0)
    /\ cursor' = cursor + 1
    /\ UNCHANGED <<slots, seqs, head, entries, next_sequence, mode, span,
                   snap_head, snap_entries, snap_next, emitted>>

LegacyCountAbort ==
    /\ mode = "count"
    /\ cursor < span
    /\ ~LegacyReadOK(cursor)
    /\ mode' = "aborted"
    /\ UNCHANGED <<slots, seqs, head, entries, next_sequence, cursor, span,
                   snap_head, snap_entries, snap_next, count, emitted>>

LegacyCountDone ==
    /\ mode = "count"
    /\ cursor = span
    /\ mode' = "counted"
    /\ UNCHANGED <<slots, seqs, head, entries, next_sequence, cursor, span,
                   snap_head, snap_entries, snap_next, count, emitted>>

\* The old defect: the emit pass re-reads the live entries.
LegacyEmitStart ==
    /\ mode = "counted"
    /\ mode' = "emit"
    /\ cursor' = 0
    /\ span' = entries
    /\ UNCHANGED <<slots, seqs, head, entries, next_sequence, snap_head,
                   snap_entries, snap_next, count, emitted>>

LegacyEmitStep ==
    /\ mode = "emit"
    /\ cursor < span
    /\ LegacyReadOK(cursor)
    /\ emitted' = emitted
                  + (IF slots[LegacySlot(cursor)] \in Matching THEN 1 ELSE 0)
    /\ cursor' = cursor + 1
    /\ UNCHANGED <<slots, seqs, head, entries, next_sequence, mode, span,
                   snap_head, snap_entries, snap_next, count>>

LegacyEmitAbort ==
    /\ mode = "emit"
    /\ cursor < span
    /\ ~LegacyReadOK(cursor)
    /\ mode' = "aborted"
    /\ UNCHANGED <<slots, seqs, head, entries, next_sequence, cursor, span,
                   snap_head, snap_entries, snap_next, count, emitted>>

LegacyEmitDone ==
    /\ mode = "emit"
    /\ cursor = span
    /\ mode' = "idle"
    /\ UNCHANGED <<slots, seqs, head, entries, next_sequence, cursor, span,
                   snap_head, snap_entries, snap_next, count, emitted>>

LegacyAbortDone ==
    /\ mode = "aborted"
    /\ mode' = "idle"
    /\ cursor' = 0
    /\ count' = 0
    /\ emitted' = 0
    /\ UNCHANGED <<slots, seqs, head, entries, next_sequence, span,
                   snap_head, snap_entries, snap_next>>

LegacyAppendIdle(r) ==
    /\ mode = "idle"
    /\ r \in Records
    /\ slots' = [slots EXCEPT ![head] = r]
    /\ seqs' = [seqs EXCEPT ![head] = next_sequence]
    /\ head' = (head + 1) % Phys
    /\ entries' = IF entries < Capacity THEN entries + 1 ELSE entries
    /\ next_sequence' = next_sequence + 1
    /\ UNCHANGED <<mode, cursor, span, snap_head, snap_entries, snap_next,
                   count, emitted>>

LegacyReset ==
    /\ mode = "idle"
    /\ entries' = 0
    /\ head' = (head + 1) % Phys
    /\ next_sequence' = next_sequence + 1
    /\ UNCHANGED <<slots, seqs, mode, cursor, span, snap_head, snap_entries,
                   snap_next, count, emitted>>

LegacyNext ==
    \/ LegacyStart
    \/ LegacyCountStep \/ LegacyCountAbort \/ LegacyCountDone
    \/ LegacyEmitStart \/ LegacyEmitStep \/ LegacyEmitAbort \/ LegacyEmitDone
    \/ LegacyAbortDone
    \/ \E r \in Records: LegacyAppendIdle(r)
    \/ LegacyReset

LegacySpec == Init /\ [][LegacyNext]_vars

\* Append between count_rows' return and write_rows' entries re-read.
LegacyAppendInGet(r) ==
    /\ mode = "counted"
    /\ r \in Records
    /\ slots' = [slots EXCEPT ![head] = r]
    /\ seqs' = [seqs EXCEPT ![head] = next_sequence]
    /\ head' = (head + 1) % Phys
    /\ entries' = IF entries < Capacity THEN entries + 1 ELSE entries
    /\ next_sequence' = next_sequence + 1
    /\ UNCHANGED <<mode, cursor, span, snap_head, snap_entries, snap_next,
                   count, emitted>>

LegacyConcurrentNext ==
    LegacyNext \/ \E r \in Records: LegacyAppendInGet(r)

LegacyConcurrentSpec == Init /\ [][LegacyConcurrentNext]_vars

(***************************************************************************)
(* Checks.                                                                  *)
(***************************************************************************)
TypeOK ==
    /\ slots \in [0..Phys-1 -> Records \cup {Empty}]
    /\ seqs \in [0..Phys-1 -> Nat]
    /\ head \in 0..Phys-1
    /\ entries \in 0..Capacity
    /\ next_sequence \in Nat
    /\ mode \in {"idle", "count", "counted", "emit", "aborted"}
    /\ cursor \in 0..Capacity
    /\ span \in 0..Capacity
    /\ snap_head \in 0..Phys-1
    /\ snap_entries \in 0..Capacity
    /\ snap_next \in Nat
    /\ count \in 0..Capacity
    /\ emitted \in 0..Capacity
    /\ Capacity + SectorSlots <= Phys

\* The array header is an upper bound on emitted rows and is exact at the end
\* of the GET.
CountEqualsEmitted ==
    /\ emitted <= count
    /\ (mode = "idle") => emitted = count

Inv == TypeOK /\ CountEqualsEmitted

\* Bounds the otherwise unbounded sequence counter so TLC's state space stays
\* finite. The cfg uses a small ring; 7 lets four appends carry a stale
\* snapshot's window under the sector an aligned reset then erases.
NextSequenceBound == next_sequence < 7

=============================================================================
