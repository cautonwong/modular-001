--------------------------- MODULE EnergyPulse ----------------------------
(***************************************************************************)
(* An abstract model of the energy pulse journal engine in                 *)
(* infra/storage/pulse_store.c, covering the protocol in                   *)
(* docs/design/energy-pulse-store.md sections 3 to 6.                      *)
(*                                                                         *)
(* The engine has two directions with unary bit lanes, a tariff marker     *)
(* timeline, two alternating base slots with generations, and two journal  *)
(* headers stamped with the base generation. Every media write is its own  *)
(* action, so TLC explores a crash at every point between them. A torn     *)
(* write ends the run: the model drops RAM and recovers from the bytes,    *)
(* which is the crash matrix's premise. Device errors that leave the       *)
(* engine running are also modeled, because they must not be mistaken for  *)
(* normal states: a base slot that is non-blank but invalid is a fault,    *)
(* not a first boot; a base write whose programs succeeded keeps its       *)
(* committed generation even when the verify read fails; and a marker      *)
(* program or verify failure does not consume its slot, so later appends   *)
(* cannot replay under the wrong tariff.                                   *)
(*                                                                         *)
(* Bounds: Capacity bits per lane, Markers marker slots, Granularity       *)
(* buffered pulses, MaxGen commits, MaxPulses offers per direction. Within *)
(* those bounds TLC exhaustively checks Inv at every reachable state, so   *)
(* every crash point in the space is covered.                              *)
(*                                                                         *)
(* accepted is cumulative; run_accepted counts the pulses accepted since   *)
(* the last attach or base commit. The crash matrix bounds the loss of     *)
(* pulses accepted since the last commit, not the cumulative sum: every    *)
(* power cut can drop its own pending window.                              *)
(***************************************************************************)
EXTENDS Naturals, FiniteSets, TLC

CONSTANTS Capacity, Markers, Granularity, MaxGen, MaxPulses, Tariffs

ASSUME Capacity \in 1..8
ASSUME Markers \in 1..3
ASSUME Granularity \in 1..Capacity
ASSUME MaxGen \in 1..3
ASSUME MaxPulses \in 1..16
ASSUME Tariffs \in 1..3

Dirs == {0, 1}
Lanes == 1..2
Phases == {"DETACHED", "NO_BASE", "INVALID", "ACTIVE", "FAULT"}

Counts == [Dirs -> 0..MaxPulses]
Cursors == [Dirs -> 0..Capacity]
ZeroCounts == [d \in Dirs |-> 0]
ZeroCursors == [d \in Dirs |-> 0]

\* Tagged records: TLC needs one shape per slot so `#` across variants is
\* well typed. "blank" is an erased slot, "torn" is a programmed but
\* unreadable slot (the two must not be confused), "valid" holds the record.
BlankBase == [kind |-> "blank", generation |-> 0, counts |-> ZeroCounts]
TornBase == [kind |-> "torn", generation |-> 0, counts |-> ZeroCounts]
BaseSlot ==
    {BlankBase, TornBase} \cup
    {[kind |-> "valid", generation |-> g, counts |-> c]
        : g \in 1..MaxGen, c \in Counts}

BlankMarker == [kind |-> "blank", cursors |-> ZeroCursors, tariff |-> 1]
CorruptMarker == [kind |-> "corrupt", cursors |-> ZeroCursors, tariff |-> 1]
MarkerSlot ==
    {BlankMarker, CorruptMarker} \cup
    {[kind |-> "valid", cursors |-> c, tariff |-> t]
        : c \in Cursors, t \in 1..Tariffs}

VARIABLES phase, gen, cursor, pending, tariff, marker_count, marker_pending,
          erased, hdr, bits, markers, base, attempted, accepted, run_accepted,
          highest

vars == <<phase, gen, cursor, pending, tariff, marker_count, marker_pending,
          erased, hdr, bits, markers, base, attempted, accepted, run_accepted,
          highest>>

\* Programmed positions in a lane, low bit first. A set, so a torn program
\* can leave any subset of the intended run programmed.
RECURSIVE PrefixFrom(_, _)
PrefixFrom(programmed, n) ==
    IF n < Capacity /\ n \in programmed
    THEN PrefixFrom(programmed, n + 1)
    ELSE n

\* The durable cursor the scan rebuilds: leading programmed bits.
PrefixLen(programmed) == PrefixFrom(programmed, 0)

MaxOf(S) == CHOOSE x \in S : \A y \in S : y <= x
AnyOf(S) == CHOOSE x \in S : TRUE

ValidBaseLanes == {i \in Lanes : base[i].kind = "valid"}

AllBaseBlank == \A i \in Lanes : base[i].kind = "blank"

\* A commit needs a generation to build on, or a region that was never
\* written. A torn region with no valid slot is a fault the engine may not
\* paper over with a fresh generation.
BaseUsable == ValidBaseLanes # {} \/ AllBaseBlank

BestGen ==
    LET gens == {base[i].generation : i \in ValidBaseLanes}
    IN IF gens = {} THEN 0 ELSE MaxOf(gens)

BestCounts(d) ==
    LET best == BestGen
        slots == {i \in ValidBaseLanes : base[i].generation = best}
    IN IF slots = {} THEN 0 ELSE base[AnyOf(slots)].counts[d]

JournalOK == BestGen > 0 /\ hdr[1] = BestGen /\ hdr[2] = BestGen

RECURSIVE ScanCount(_)
ScanCount(slot) ==
    IF slot > Markers THEN 0
    ELSE IF markers[slot].kind = "blank" THEN 0
    ELSE 1 + ScanCount(slot + 1)

RECURSIVE TariffFrom(_, _)
TariffFrom(slot, current) ==
    IF slot > Markers THEN current
    ELSE IF markers[slot].kind = "blank" THEN current
    ELSE IF markers[slot].kind = "corrupt" THEN TariffFrom(slot + 1, current)
    ELSE TariffFrom(slot + 1, markers[slot].tariff)

LastTariff == TariffFrom(1, 0)

RECURSIVE ReplayFrom(_, _, _, _)
ReplayFrom(slot, d, position, total) ==
    IF slot > Markers THEN total + (PrefixLen(bits[d]) - position)
    ELSE IF markers[slot].kind = "blank"
    THEN total + (PrefixLen(bits[d]) - position)
    ELSE IF markers[slot].kind = "corrupt"
    THEN ReplayFrom(slot + 1, d, position, total)
    ELSE LET next == markers[slot].cursors[d] IN
         IF next < position \/ next > PrefixLen(bits[d])
         THEN ReplayFrom(slot + 1, d, position, total)
         ELSE ReplayFrom(slot + 1, d, next, total + (next - position))

ReplayCount(d) == ReplayFrom(1, d, 0, 0)

JournalCount(d) == IF JournalOK THEN ReplayCount(d) ELSE 0

Recovered(d) == BestCounts(d) + JournalCount(d)

RECURSIVE MarkerCursorsOk(_, _)
MarkerCursorsOk(slot, last) ==
    IF slot > ScanCount(1) THEN TRUE
    ELSE IF markers[slot].kind = "corrupt"
         THEN MarkerCursorsOk(slot + 1, last)
    ELSE /\ \A d \in Dirs : last[d] <= markers[slot].cursors[d]
         /\ MarkerCursorsOk(slot + 1, markers[slot].cursors)

Init ==
    /\ phase = "DETACHED"
    /\ gen = 0
    /\ cursor = [d \in Dirs |-> 0]
    /\ pending = [d \in Dirs |-> 0]
    /\ tariff = 0
    /\ marker_count = 0
    /\ marker_pending = FALSE
    /\ erased = FALSE
    /\ hdr = [i \in Lanes |-> 0]
    /\ bits = [d \in Dirs |-> {}]
    /\ markers = [i \in 1..Markers |-> BlankMarker]
    /\ base = [i \in Lanes |-> BlankBase]
    /\ attempted = [d \in Dirs |-> 0]
    /\ accepted = [d \in Dirs |-> 0]
    /\ run_accepted = [d \in Dirs |-> 0]
    /\ highest = 0

\* Power loss drops RAM; the bytes, the counters and the run ledger remain
\* for the invariants to relate to the media. The next Attach starts a run.
RamLost ==
    /\ phase' = "DETACHED"
    /\ gen' = 0
    /\ cursor' = [d \in Dirs |-> 0]
    /\ pending' = [d \in Dirs |-> 0]
    /\ tariff' = 0
    /\ marker_count' = 0
    /\ marker_pending' = FALSE
    /\ erased' = FALSE

MediaAndCounters ==
    UNCHANGED <<hdr, bits, markers, base, attempted, accepted, run_accepted,
                highest>>

Crash == RamLost /\ MediaAndCounters

Restart ==
    \/ Crash
    \/ /\ phase = "DETACHED"
       /\ UNCHANGED vars

\* First operation: select the newest base, require both headers to match
\* that generation, then scan bits and markers. An all-blank region is a
\* first boot; a non-blank region with no valid slot is a fault and must
\* never be reported as EMPTY.
Attach ==
    /\ phase = "DETACHED"
    /\ gen' = BestGen
    /\ pending' = [d \in Dirs |-> 0]
    /\ marker_pending' = FALSE
    /\ run_accepted' = [d \in Dirs |-> 0]
    /\ UNCHANGED <<erased, hdr, bits, markers, base, attempted, accepted,
                   highest>>
    /\ IF BestGen = 0
       THEN \/ /\ AllBaseBlank
               /\ phase' = "NO_BASE"
               /\ cursor' = [d \in Dirs |-> 0]
               /\ tariff' = 0
               /\ marker_count' = 0
            \/ /\ ~AllBaseBlank
               /\ phase' = "FAULT"
               /\ cursor' = [d \in Dirs |-> 0]
               /\ tariff' = 0
               /\ marker_count' = 0
       ELSE IF JournalOK
            THEN phase' = "ACTIVE"
                 /\ cursor' = [d \in Dirs |-> PrefixLen(bits[d])]
                 /\ tariff' = LastTariff
                 /\ marker_count' = ScanCount(1)
            ELSE phase' = "INVALID"
                 /\ cursor' = [d \in Dirs |-> 0]
                 /\ tariff' = 0
                 /\ marker_count' = 0

\* A device read error parks the engine; commit can still repair the base.
Fault ==
    /\ phase \in {"NO_BASE", "INVALID", "ACTIVE"}
    /\ phase' = "FAULT"
    /\ UNCHANGED <<gen, cursor, pending, tariff, marker_count, marker_pending,
                   erased, hdr, bits, markers, base, attempted, accepted,
                   run_accepted, highest>>

SlotOf(g) == (g % 2) + 1

\* Base first, journal after. The next slot alternates, so a torn write can
\* only invalidate the stale slot and the current base stays readable. A
\* torn region with no valid slot blocks the commit instead of overwriting
\* the damage with generation 1.
CommitBase ==
    /\ phase # "DETACHED"
    /\ gen < MaxGen
    /\ BaseUsable
    /\ (
         \/ LET next == gen + 1 IN
            /\ base' = [base EXCEPT
                          ![SlotOf(next)] =
                              [kind |-> "valid", generation |-> next,
                               counts |-> accepted]]
            /\ gen' = next
            /\ highest' = next
            /\ cursor' = [d \in Dirs |-> 0]
            /\ pending' = [d \in Dirs |-> 0]
            /\ tariff' = 0
            /\ marker_count' = 0
            /\ marker_pending' = FALSE
            /\ run_accepted' = [d \in Dirs |-> 0]
            /\ erased' = FALSE
            /\ phase' = "INVALID"
            /\ UNCHANGED <<hdr, bits, markers, attempted, accepted>>
         \/ \* torn base write: the slot is unreadable and the run dies
            /\ base' = [base EXCEPT ![SlotOf(gen + 1)] = TornBase]
            /\ RamLost
            /\ UNCHANGED <<hdr, bits, markers, attempted, accepted,
                           run_accepted, highest>>
       )

\* The base programs all returned success but the verify read failed or
\* rejected the CRC. The disk slot holds generation next, so the engine must
\* advance the committed epoch exactly as after a verified write; the stale
\* journal is invalidated and the next append rebuilds it at next. RAM must
\* never disagree with the disk about the generation.
CommitVerifyFailed ==
    /\ phase # "DETACHED"
    /\ gen < MaxGen
    /\ BaseUsable
    /\ LET next == gen + 1 IN
       /\ base' = [base EXCEPT
                     ![SlotOf(next)] =
                         [kind |-> "valid", generation |-> next,
                          counts |-> accepted]]
       /\ gen' = next
       /\ highest' = next
       /\ cursor' = [d \in Dirs |-> 0]
       /\ pending' = [d \in Dirs |-> 0]
       /\ tariff' = 0
       /\ marker_count' = 0
       /\ marker_pending' = FALSE
       /\ run_accepted' = [d \in Dirs |-> 0]
       /\ erased' = FALSE
       /\ phase' = "INVALID"
       /\ UNCHANGED <<hdr, bits, markers, attempted, accepted>>

\* Erase the journal region. Only a verified full erase admits headers.
JournalErase ==
    /\ phase = "INVALID"
    /\ (
         \/ /\ hdr' = [i \in Lanes |-> 0]
            /\ bits' = [d \in Dirs |-> {}]
            /\ markers' = [i \in 1..Markers |-> BlankMarker]
            /\ erased' = TRUE
            /\ marker_pending' = FALSE
            /\ UNCHANGED <<gen, cursor, pending, tariff, marker_count, phase,
                           base, attempted, accepted, run_accepted, highest>>
         \/ \* torn erase: a subset of sectors survives, the run dies. Markers
            \* are kept: a later successful erase blanks them before any
            \* header can make the journal scannable again.
            /\ \E kept \in SUBSET Lanes :
                   hdr' = [i \in Lanes |-> IF i \in kept THEN hdr[i] ELSE 0]
            /\ \E kept0 \in SUBSET bits[0], kept1 \in SUBSET bits[1] :
                   bits' = [d \in Dirs |-> IF d = 0 THEN kept0 ELSE kept1]
            /\ RamLost
            /\ UNCHANGED <<markers, base, attempted, accepted, run_accepted,
                           highest>>
       )

\* One journal header, stamped with the base generation. Both must verify
\* after a full erase; until then the journal is not scannable.
JournalHeader(lane) ==
    /\ phase = "INVALID"
    /\ erased
    /\ hdr[lane] = 0
    /\ LET next_hdr == [hdr EXCEPT ![lane] = gen]
       IN (
            \/ /\ hdr' = next_hdr
               /\ IF next_hdr[1] = gen /\ next_hdr[2] = gen
                  THEN phase' = "ACTIVE"
                       /\ cursor' = [d \in Dirs |-> PrefixLen(bits[d])]
                       /\ pending' = [d \in Dirs |-> 0]
                       /\ tariff' = LastTariff
                       /\ marker_count' = ScanCount(1)
                       /\ marker_pending' = FALSE
                  ELSE phase' = "INVALID"
                       /\ UNCHANGED <<cursor, pending, tariff, marker_count,
                                      marker_pending>>
               /\ UNCHANGED <<gen, erased, bits, markers, base, attempted,
                              accepted, run_accepted, highest>>
            \/ \* torn header: still invalid, the run dies
               /\ RamLost
               /\ UNCHANGED <<hdr, bits, markers, base, attempted, accepted,
                              run_accepted, highest>>
          )

RunPos(d) == cursor[d]..(cursor[d] + pending[d] - 1)
RunWithNew(d) == cursor[d]..(cursor[d] + pending[d])
AddBits(b, d, S) == [b EXCEPT ![d] = @ \cup S]
PostCursor == [d \in Dirs |-> cursor[d] + pending[d]]

\* No tariff change: buffer the bit, or program the lane's run. The bit the
\* pulse owns is at cursor + pending and must stay inside the lane; the
\* engine flushes at every byte end, so this is the C geometry's guarantee.
AppendSame(d, t) ==
    /\ phase = "ACTIVE"
    /\ attempted[d] < MaxPulses
    /\ t = tariff
    /\ marker_pending = FALSE
    /\ cursor[d] + pending[d] < Capacity
    /\ attempted' = [attempted EXCEPT ![d] = @ + 1]
    /\ (
         \/ \* buffered: accepted, durable only after the next program
            /\ pending[d] + 1 < Granularity
            /\ (cursor[d] + pending[d]) % 8 # 7
            /\ pending' = [pending EXCEPT ![d] = @ + 1]
            /\ accepted' = [accepted EXCEPT ![d] = @ + 1]
            /\ run_accepted' = [run_accepted EXCEPT ![d] = @ + 1]
            /\ UNCHANGED <<gen, cursor, tariff, marker_count, marker_pending,
                           erased, hdr, bits, markers, base, highest, phase>>
         \/ \* program verified: the run and the new bit are durable
            /\ pending[d] + 1 >= Granularity
               \/ (cursor[d] + pending[d]) % 8 = 7
            /\ bits' = AddBits(bits, d, RunWithNew(d))
            /\ cursor' = [cursor EXCEPT ![d] = cursor[d] + pending[d] + 1]
            /\ pending' = [pending EXCEPT ![d] = 0]
            /\ accepted' = [accepted EXCEPT ![d] = @ + 1]
            /\ run_accepted' = [run_accepted EXCEPT ![d] = @ + 1]
            /\ UNCHANGED <<gen, tariff, marker_count, marker_pending, erased,
                           hdr, markers, base, highest, phase>>
         \/ \* torn program: a subset of the run landed, then the power died
            /\ pending[d] + 1 >= Granularity
               \/ (cursor[d] + pending[d]) % 8 = 7
            /\ \E survived \in SUBSET RunWithNew(d) :
                   bits' = AddBits(bits, d, survived)
            /\ RamLost
            /\ UNCHANGED <<hdr, markers, base, accepted, run_accepted,
                           highest>>
       )

\* Tariff change: flush both lanes, write a marker, then the pulse. A marker
\* program or verify error does not consume the slot and does not change the
\* current tariff; the pulse is not accepted, and later appends stay blocked
\* on marker_pending until a marker is verified in that same slot.
AppendAcross(d, t) ==
    /\ phase = "ACTIVE"
    /\ attempted[d] < MaxPulses
    /\ t # tariff
    /\ marker_pending = FALSE
    /\ marker_count < Markers
    /\ attempted' = [attempted EXCEPT ![d] = @ + 1]
    /\ (
         \/ \* lane 0 tears mid-flush
            /\ \E survived \in SUBSET RunPos(0) :
                   bits' = AddBits(bits, 0, survived)
            /\ RamLost
            /\ UNCHANGED <<hdr, markers, base, accepted, run_accepted,
                           highest>>
         \/ \* lane 0 verified, lane 1 tears
            /\ \E survived \in SUBSET RunPos(1) :
                   bits' = AddBits(AddBits(bits, 0, RunPos(0)), 1, survived)
            /\ RamLost
            /\ UNCHANGED <<hdr, markers, base, accepted, run_accepted,
                           highest>>
         \/ \* both lanes verified, marker torn: the slot burns, the run dies
            /\ bits' = AddBits(AddBits(bits, 0, RunPos(0)), 1, RunPos(1))
            /\ markers' = [markers EXCEPT
                             ![marker_count + 1] = CorruptMarker]
            /\ RamLost
            /\ UNCHANGED <<hdr, base, accepted, run_accepted, highest>>
         \/ \* both lanes verified, marker program error before it is written:
            \* the slot stays blank and is not consumed
            /\ bits' = AddBits(AddBits(bits, 0, RunPos(0)), 1, RunPos(1))
            /\ cursor' = PostCursor
            /\ pending' = [x \in Dirs |-> 0]
            /\ marker_pending' = TRUE
            /\ UNCHANGED <<gen, tariff, marker_count, erased, hdr, markers,
                           base, accepted, run_accepted, highest, phase>>
         \/ \* both lanes verified, marker landed but its verify read failed:
            \* the slot holds the new marker, the engine has not consumed it
            /\ bits' = AddBits(AddBits(bits, 0, RunPos(0)), 1, RunPos(1))
            /\ markers' = [markers EXCEPT
                             ![marker_count + 1] =
                                 [kind |-> "valid", cursors |-> PostCursor,
                                  tariff |-> t]]
            /\ cursor' = PostCursor
            /\ pending' = [x \in Dirs |-> 0]
            /\ marker_pending' = TRUE
            /\ UNCHANGED <<gen, tariff, marker_count, erased, hdr, base,
                           accepted, run_accepted, highest, phase>>
         \/ \* both lanes verified, marker written, direction already full
            /\ PostCursor[d] >= Capacity
            /\ bits' = AddBits(AddBits(bits, 0, RunPos(0)), 1, RunPos(1))
            /\ markers' = [markers EXCEPT
                             ![marker_count + 1] =
                                 [kind |-> "valid", cursors |-> PostCursor,
                                  tariff |-> t]]
            /\ marker_count' = marker_count + 1
            /\ tariff' = t
            /\ marker_pending' = FALSE
            /\ cursor' = PostCursor
            /\ pending' = [x \in Dirs |-> 0]
            /\ UNCHANGED <<gen, erased, hdr, base, accepted, run_accepted,
                           highest, phase>>
         \/ \* marker written, the new bit buffers
            /\ PostCursor[d] < Capacity
            /\ 1 < Granularity
            /\ PostCursor[d] % 8 # 7
            /\ bits' = AddBits(AddBits(bits, 0, RunPos(0)), 1, RunPos(1))
            /\ markers' = [markers EXCEPT
                             ![marker_count + 1] =
                                 [kind |-> "valid", cursors |-> PostCursor,
                                  tariff |-> t]]
            /\ marker_count' = marker_count + 1
            /\ tariff' = t
            /\ marker_pending' = FALSE
            /\ cursor' = PostCursor
            /\ pending' = [x \in Dirs |-> IF x = d THEN 1 ELSE 0]
            /\ accepted' = [accepted EXCEPT ![d] = @ + 1]
            /\ run_accepted' = [run_accepted EXCEPT ![d] = @ + 1]
            /\ UNCHANGED <<gen, erased, hdr, base, highest, phase>>
         \/ \* marker written, the new bit programs and verifies
            /\ PostCursor[d] < Capacity
            /\ (PostCursor[d] % 8 = 7 \/ 1 >= Granularity)
            /\ bits' = AddBits(AddBits(AddBits(bits, 0, RunPos(0)), 1,
                                       RunPos(1)), d, {PostCursor[d]})
            /\ markers' = [markers EXCEPT
                             ![marker_count + 1] =
                                 [kind |-> "valid", cursors |-> PostCursor,
                                  tariff |-> t]]
            /\ marker_count' = marker_count + 1
            /\ tariff' = t
            /\ marker_pending' = FALSE
            /\ cursor' = [PostCursor EXCEPT ![d] = PostCursor[d] + 1]
            /\ pending' = [x \in Dirs |-> 0]
            /\ accepted' = [accepted EXCEPT ![d] = @ + 1]
            /\ run_accepted' = [run_accepted EXCEPT ![d] = @ + 1]
            /\ UNCHANGED <<gen, erased, hdr, base, highest, phase>>
         \/ \* marker written, the new bit's program tears
            /\ PostCursor[d] < Capacity
            /\ (PostCursor[d] % 8 = 7 \/ 1 >= Granularity)
            /\ \E survived \in SUBSET {PostCursor[d]} :
                   bits' = AddBits(AddBits(AddBits(bits, 0, RunPos(0)), 1,
                                           RunPos(1)), d, survived)
            /\ markers' = [markers EXCEPT
                             ![marker_count + 1] =
                                 [kind |-> "valid", cursors |-> PostCursor,
                                  tariff |-> t]]
            /\ RamLost
            /\ UNCHANGED <<hdr, base, accepted, run_accepted, highest>>
       )

\* Retry the pending marker in its own slot. The append that triggered it
\* may still be refused; a later AppendSame or AppendAcross carries the
\* pulse once the marker verifies.
ResolveMarker(t) ==
    /\ phase = "ACTIVE"
    /\ marker_pending
    /\ marker_count < Markers
    /\ bits' = AddBits(AddBits(bits, 0, RunPos(0)), 1, RunPos(1))
    /\ cursor' = PostCursor
    /\ pending' = [x \in Dirs |-> 0]
    /\ (
         \/ \* program and verify succeeded: the slot is consumed
            /\ markers' = [markers EXCEPT
                             ![marker_count + 1] =
                                 [kind |-> "valid", cursors |-> PostCursor,
                                  tariff |-> t]]
            /\ marker_count' = marker_count + 1
            /\ tariff' = t
            /\ marker_pending' = FALSE
         \/ \* program error before the write: still not consumed
            /\ markers' = markers
            /\ marker_pending' = TRUE
            /\ UNCHANGED <<tariff, marker_count>>
         \/ \* landed, verify read failed: still not consumed
            /\ markers' = [markers EXCEPT
                             ![marker_count + 1] =
                                 [kind |-> "valid", cursors |-> PostCursor,
                                  tariff |-> t]]
            /\ marker_pending' = TRUE
            /\ UNCHANGED <<tariff, marker_count>>
       )
    /\ UNCHANGED <<gen, erased, hdr, base, attempted, accepted, run_accepted,
                   highest, phase>>

AppendFullCapacity(d, t) ==
    /\ phase = "ACTIVE"
    /\ attempted[d] < MaxPulses
    /\ t = tariff
    /\ marker_pending = FALSE
    /\ cursor[d] >= Capacity
    /\ attempted' = [attempted EXCEPT ![d] = @ + 1]
    /\ UNCHANGED <<gen, cursor, pending, tariff, marker_count, marker_pending,
                   erased, hdr, bits, markers, base, accepted, run_accepted,
                   highest, phase>>

AppendFullMarkers(d, t) ==
    /\ phase = "ACTIVE"
    /\ attempted[d] < MaxPulses
    /\ t # tariff
    /\ marker_pending = FALSE
    /\ marker_count >= Markers
    /\ attempted' = [attempted EXCEPT ![d] = @ + 1]
    /\ UNCHANGED <<gen, cursor, pending, tariff, marker_count, marker_pending,
                   erased, hdr, bits, markers, base, accepted, run_accepted,
                   highest, phase>>

Next ==
    \/ Restart
    \/ Attach
    \/ Fault
    \/ CommitBase
    \/ CommitVerifyFailed
    \/ JournalErase
    \/ \E lane \in Lanes : JournalHeader(lane)
    \/ \E d \in Dirs, t \in 1..Tariffs : AppendSame(d, t)
    \/ \E d \in Dirs, t \in 1..Tariffs : AppendAcross(d, t)
    \/ \E t \in 1..Tariffs : ResolveMarker(t)
    \/ \E d \in Dirs, t \in 1..Tariffs : AppendFullCapacity(d, t)
    \/ \E d \in Dirs, t \in 1..Tariffs : AppendFullMarkers(d, t)

Spec == Init /\ [][Next]_vars

\* Crash matrix rows 1-3: recovery never counts a pulse twice. Recovered can
\* exceed accepted by the bits of an offered pulse whose append did not
\* return OK (in flight or reported an error), so the bound is against the
\* offered count, matching test_pulse_store_crash.c.
NoDoubleCount == \A d \in Dirs : Recovered(d) <= attempted[d]

\* Row 15 and the granularity bound: the pulses accepted since the last
\* commit or attach that the journal does not hold are exactly the pending
\* buffer, which is strictly smaller than Granularity. A second power cut
\* drops its own window, so this is per commit interval, not cumulative.
AcceptedLossBound ==
    \A d \in Dirs : run_accepted[d] - JournalCount(d) <= Granularity

AcceptedIsOffered == \A d \in Dirs : accepted[d] <= attempted[d]
RunAcceptedIsOffered == \A d \in Dirs : run_accepted[d] <= accepted[d]
PendingBound == \A d \in Dirs : pending[d] < Granularity
CursorBound == \A d \in Dirs : cursor[d] + pending[d] <= Capacity

\* Rows 7-12: the journal counts only under the selected base generation,
\* that generation is the highest ever written, and it survives restarts.
JournalAtBaseGeneration == JournalOK => hdr[1] = BestGen /\ hdr[2] = BestGen
BaseGenerationIsHighest == BestGen = highest
AttachedGenerationMatchesBase == phase # "DETACHED" => gen = BestGen

\* Rows 4, 5, 9, 10: restart restores the marker count and tariff, and the
\* marker timeline stays ordered inside the current epoch. A pending marker
\* may already be on the medium (its verify read failed), at most one slot
\* ahead of what RAM has consumed.
MarkerScanAgrees ==
    phase = "ACTIVE" =>
        /\ (marker_pending => ScanCount(1) \in {marker_count, marker_count + 1})
        /\ (~marker_pending =>
                marker_count = ScanCount(1) /\ tariff = LastTariff)

\* A pending marker keeps the bit lanes quiescent: AppendSame and
\* AppendAcross both require marker_pending = FALSE, so no accepted pulse can
\* be written after the marker's cursor until it is consumed or the journal is
\* rebuilt. The pending slot is always the next marker slot.
MarkerPendingSafe ==
    marker_pending => pending = [d \in Dirs |-> 0] /\ marker_count < Markers

\* Row 13: EMPTY means a region that was never written. A non-blank region
\* with no valid slot is a fault and is never mistaken for a first boot.
NoBaseIsBlankOnly == phase = "NO_BASE" => AllBaseBlank
ReplayCoversCursor == \A d \in Dirs :
    JournalOK => ReplayCount(d) = PrefixLen(bits[d])
ValidMarkerCursors == JournalOK => \A i \in 1..ScanCount(1) :
    IF markers[i].kind = "corrupt" THEN TRUE
    ELSE \A d \in Dirs : markers[i].cursors[d] <= PrefixLen(bits[d])
ValidMarkerMonotone == JournalOK =>
    MarkerCursorsOk(1, [d \in Dirs |-> 0])

\* Row 1: the bits recovery counts are a contiguous cleared prefix.
CountedPrefixDense ==
    \A d \in Dirs : \A i \in 0..(PrefixLen(bits[d]) - 1) : i \in bits[d]

TypeOK ==
    /\ phase \in Phases
    /\ gen \in 0..MaxGen
    /\ highest \in 0..MaxGen
    /\ \A d \in Dirs : cursor[d] \in 0..Capacity
    /\ \A d \in Dirs : pending[d] \in 0..Capacity
    /\ \A d \in Dirs : attempted[d] \in 0..MaxPulses
    /\ \A d \in Dirs : accepted[d] \in 0..MaxPulses
    /\ \A d \in Dirs : run_accepted[d] \in 0..MaxPulses
    /\ tariff \in 0..Tariffs
    /\ marker_count \in 0..Markers
    /\ marker_pending \in BOOLEAN
    /\ erased \in BOOLEAN
    /\ \A i \in Lanes : hdr[i] \in 0..MaxGen /\ base[i] \in BaseSlot
    /\ \A i \in 1..Markers : markers[i] \in MarkerSlot
    /\ \A d \in Dirs : bits[d] \subseteq 0..(Capacity - 1)

Inv ==
    /\ TypeOK
    /\ NoDoubleCount
    /\ AcceptedLossBound
    /\ AcceptedIsOffered
    /\ RunAcceptedIsOffered
    /\ PendingBound
    /\ CursorBound
    /\ JournalAtBaseGeneration
    /\ BaseGenerationIsHighest
    /\ AttachedGenerationMatchesBase
    /\ NoBaseIsBlankOnly
    /\ MarkerScanAgrees
    /\ MarkerPendingSafe
    /\ ReplayCoversCursor
    /\ ValidMarkerCursors
    /\ ValidMarkerMonotone
    /\ CountedPrefixDense

=============================================================================
