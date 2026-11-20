--------------------------- MODULE Snapshot ----------------------------
(***************************************************************************)
(* An abstract model of the stategrid dual-slot energy snapshot in         *)
(* infra/storage/pulse_snapshot.c and docs/design/energy-pulse-store.md.   *)
(*                                                                         *)
(* Two alternating slots hold a generation and a value. A commit writes    *)
(* the header, then the blob, then the CRC; every write can tear, and a    *)
(* torn write leaves the target slot invalid. Validity requires the whole  *)
(* generation's header, blob and CRC. Recovery picks the valid slot with   *)
(* the highest generation.                                                 *)
(*                                                                         *)
(* The target slot of generation g holds generation g-2 (or is blank), so  *)
(* invalidating it on a torn write can never destroy the last committed    *)
(* generation g-1: it lives in the other slot. The model checks, for every *)
(* crash point reachable within the bounds:                                *)
(*   - the recovered generation is the last committed generation or the    *)
(*     previous one, or the in-flight one once its write fully verified;   *)
(*   - the recovered value equals that generation's value (no mix);        *)
(*   - recovery is 0 only when no slot is valid (a consumer fault).        *)
(* Device read errors are not modeled; the consumer returns a fault for    *)
(* them and is covered by tests/unit/test_energy_snapshot_adoption.c.      *)
(***************************************************************************)
EXTENDS Naturals, TLC

CONSTANTS MaxGen

ASSUME MaxGen \in 1..4

Generations == 1..MaxGen
ValueOf(g) == g

VARIABLES slot, pending, lastCommitted, writeStage, crashed, recovered, recoveredValue

vars == <<slot, pending, lastCommitted, writeStage, crashed, recovered, recoveredValue>>

Blank == [gen |-> 0, value |-> 0, blob |-> FALSE, crc |-> FALSE]

Valid(r) == r.gen > 0 /\ r.blob /\ r.crc

HighestValidSlot ==
    IF \E i \in {0, 1} : Valid(slot[i])
    THEN CHOOSE i \in {0, 1} :
             Valid(slot[i])
             /\ \A j \in {0, 1} : Valid(slot[j]) => slot[j].gen <= slot[i].gen
    ELSE 2

Init ==
    /\ slot = [i \in {0, 1} |-> Blank]
    /\ pending = 0
    /\ lastCommitted = 0
    /\ writeStage = "idle"
    /\ crashed = FALSE
    /\ recovered = 0
    /\ recoveredValue = 0

Start(g) ==
    /\ crashed = FALSE
    /\ recovered = 0
    /\ pending = 0
    /\ g = lastCommitted + 1
    /\ g \in Generations
    /\ pending' = g
    /\ writeStage' = "header"
    /\ crashed' = FALSE
    /\ UNCHANGED <<slot, lastCommitted, recovered, recoveredValue>>

\* A torn write leaves the target slot unusable. The other slot still holds
\* the last committed generation.
WriteHeader(g, ok) ==
    /\ crashed = FALSE
    /\ recovered = 0
    /\ pending = g
    /\ writeStage = "header"
    /\ IF ok
       THEN slot' = [slot EXCEPT ![g % 2] =
                        [gen |-> g, value |-> ValueOf(g),
                         blob |-> FALSE, crc |-> FALSE]]
       ELSE slot' = [slot EXCEPT ![g % 2] = Blank]
    /\ writeStage' = "blob"
    /\ UNCHANGED <<pending, lastCommitted, crashed, recovered, recoveredValue>>

WriteBlob(g, ok) ==
    /\ crashed = FALSE
    /\ recovered = 0
    /\ pending = g
    /\ writeStage = "blob"
    /\ IF ok
       THEN slot' = [slot EXCEPT ![g % 2].blob = TRUE]
       ELSE slot' = slot
    /\ writeStage' = "crc"
    /\ UNCHANGED <<pending, lastCommitted, crashed, recovered, recoveredValue>>

WriteCrc(g, ok) ==
    /\ crashed = FALSE
    /\ recovered = 0
    /\ pending = g
    /\ writeStage = "crc"
    /\ IF ok
       THEN slot' = [slot EXCEPT ![g % 2].crc = TRUE]
       ELSE slot' = slot
    /\ writeStage' = "done"
    /\ UNCHANGED <<pending, lastCommitted, crashed, recovered, recoveredValue>>

Finish(g) ==
    /\ crashed = FALSE
    /\ recovered = 0
    /\ pending = g
    /\ writeStage = "done"
    /\ Valid(slot[g % 2])
    /\ slot[g % 2].value = ValueOf(g)
    /\ lastCommitted' = g
    /\ pending' = 0
    /\ writeStage' = "idle"
    /\ UNCHANGED <<slot, crashed, recovered, recoveredValue>>

Crash ==
    /\ pending > 0
    /\ crashed' = TRUE
    /\ \/ (recovered' = 0 /\ recoveredValue' = 0 /\ HighestValidSlot = 2)
       \/ (HighestValidSlot < 2
           /\ recovered' = slot[HighestValidSlot].gen
           /\ recoveredValue' = slot[HighestValidSlot].value)
    /\ UNCHANGED <<slot, pending, lastCommitted, writeStage>>

Stutter == UNCHANGED vars

\* Reboot: adopt the verified generation, or drop the first in-flight
\* commit when nothing verifies.
Resume ==
    /\ pending > 0
    /\ recovered > 0
    /\ lastCommitted' = recovered
    /\ pending' = 0
    /\ writeStage' = "idle"
    /\ crashed' = FALSE
    /\ recovered' = 0
    /\ recoveredValue' = 0
    /\ UNCHANGED <<slot>>

ResumeEmpty ==
    /\ pending > 0
    /\ recovered = 0
    /\ pending' = 0
    /\ writeStage' = "idle"
    /\ crashed' = FALSE
    /\ UNCHANGED <<slot, lastCommitted, recovered, recoveredValue>>

Next ==
    \/ \E g \in Generations : Start(g)
    \/ \E g \in Generations, ok \in BOOLEAN : WriteHeader(g, ok)
    \/ \E g \in Generations, ok \in BOOLEAN : WriteBlob(g, ok)
    \/ \E g \in Generations, ok \in BOOLEAN : WriteCrc(g, ok)
    \/ \E g \in Generations : Finish(g)
    \/ Crash
    \/ Resume
    \/ ResumeEmpty
    \/ Stutter

\* After a crash the recovered generation is the last committed one, the
\* previous one, or the in-flight one once every write verified. It is never
\* a future or older generation.
RecoveredIsAdjacent ==
    recovered = 0
    \/ (lastCommitted > 0 /\ recovered \in {lastCommitted - 1, lastCommitted})
    \/ (lastCommitted + 1 <= MaxGen /\ recovered = lastCommitted + 1)

\* Recovery never mixes the value of one write with the generation of
\* another.
RecoveredValueMatches ==
    (recovered = 0 => recoveredValue = 0)
    /\ (recovered > 0 => recoveredValue = ValueOf(recovered))

\* A torn commit after a completed commit still leaves a base: the previous
\* generation lives in the other slot. Only the very first commit may
\* recover nothing (a consumer fault).
FallbackKeepsABase ==
    (crashed /\ lastCommitted >= 1) => recovered > 0

\* A recovered generation always comes from a slot whose header, blob and
\* CRC all verified. A torn write can never be adopted.
RecoveredSlotValid ==
    recovered = 0
    \/ (Valid(slot[recovered % 2]) /\ slot[recovered % 2].gen = recovered)

\* Every stored record's value belongs to its own generation.
SlotValueSound ==
    \A i \in {0, 1} : slot[i].gen = 0 \/ slot[i].value = ValueOf(slot[i].gen)

Inv ==
    /\ RecoveredIsAdjacent
    /\ RecoveredValueMatches
    /\ RecoveredSlotValid
    /\ FallbackKeepsABase
    /\ SlotValueSound

=============================================================================
