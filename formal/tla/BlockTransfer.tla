--------------------------- MODULE BlockTransfer ---------------------------
(***************************************************************************)
(* A model of the GET block transfer state machine described in            *)
(* docs/idis-block-transfer.md. One association, one object. The model     *)
(* checks that blocks are served in order, that a block is never served    *)
(* twice, and that a new NORMAL GET resets the transfer.                   *)
(***************************************************************************)
EXTENDS Naturals, FiniteSets

CONSTANTS TotalBlocks

ASSUME TotalBlocks \in Nat /\ TotalBlocks > 0

VARIABLES active, next, sent

vars == <<active, next, sent>>

Init == active = FALSE /\ next = 1 /\ sent = {}

Start ==
    /\ active' = TRUE
    /\ next' = 1
    /\ sent' = {}

NextBlock(n) ==
    /\ active
    /\ n = next
    /\ n <= TotalBlocks
    /\ sent' = sent \union {n}
    /\ next' = n + 1
    /\ active' = (n < TotalBlocks)

Invalid(n) ==
    /\ ~(active /\ n = next /\ n <= TotalBlocks)
    /\ UNCHANGED vars

Next ==
    Start
    \/ \E n \in 1..TotalBlocks: NextBlock(n)
    \/ \E m \in 1..TotalBlocks: Invalid(m)

Spec == Init /\ [][Next]_vars

\* Every served block is exactly the prefix 1..next-1.
Ordered == sent = 1..(next - 1)

\* The cursor never runs past one past the last block.
Bounded == next \in 1..(TotalBlocks + 1)

\* No block is served twice.
NoRepeat == Cardinality(sent) = next - 1

\* While a transfer is active there is still a block to serve.
ActiveHasWork == active => next <= TotalBlocks

\* A completed transfer is closed and cannot serve another block.
CompletedClosed == (next = TotalBlocks + 1) => ~active

Inv == Ordered /\ Bounded /\ NoRepeat /\ ActiveHasWork /\ CompletedClosed

=============================================================================
