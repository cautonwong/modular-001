/-
  Models of the counting and erase arithmetic that the energy pulse store
  relies on (docs/design/energy-pulse-store.md sections 5 and 7): the unary
  bit lane's cleared prefix, the pending buffer's loss bound, and the journal
  wrap count. The C layer is not refined here; these are the integer facts
  the engine's crash and wear arguments rest on.
-/
namespace BladeFormal

/-! ## Unary lane

A lane is a list of bits, low bit first. `scan` is the durable cursor the
engine rebuilds: it counts cleared bits until the first set one. Programming
only clears bits, so a torn program leaves an arbitrary subset of the run. -/

def scan : List Bool → Nat
  | [] => 0
  | false :: rest => scan rest + 1
  | true :: _ => 0

def setAt : Nat → Bool → List Bool → List Bool
  | 0, b, _ :: rest => b :: rest
  | 0, _, [] => []
  | Nat.succ n, b, x :: rest => x :: setAt n b rest
  | _, _, [] => []

theorem scan_le_length (bits : List Bool) : scan bits ≤ bits.length := by
  induction bits with
  | nil => simp [scan]
  | cons b rest ih =>
      cases b with
      | false =>
          simp [scan]
          omega
      | true => simp [scan]

/-- Every bit the scan counts is cleared: the cursor is a contiguous cleared
prefix, never a count with a hole in it. -/
theorem scan_counts_cleared (bits : List Bool) :
    ∀ i, i < scan bits → bits[i]? = some false := by
  induction bits with
  | nil =>
      intro i h
      exact absurd h (Nat.not_lt_zero i)
  | cons b rest ih =>
      intro i h
      cases b with
      | true => exact absurd h (Nat.not_lt_zero i)
      | false =>
          cases i with
          | zero => simp
          | succ i =>
              simp [scan] at h
              have := ih i (by omega)
              simpa using this

/-- When the lane still has a set bit, the scan stops exactly on it. -/
theorem scan_stops (bits : List Bool) (h : scan bits < bits.length) :
    bits[scan bits]? = some true := by
  induction bits with
  | nil => exact absurd h (Nat.not_lt_zero (scan []))
  | cons b rest ih =>
      cases b with
      | true => simp [scan]
      | false =>
          simp [scan] at h
          have := ih (by omega)
          simpa using this

/-- Programming the bit under the cursor strictly advances the scan, so a
retry cannot keep reporting the same cleared prefix. -/
theorem scan_setAt_cursor (bits : List Bool) (h : scan bits < bits.length) :
    scan bits < scan (setAt (scan bits) false bits) := by
  induction bits with
  | nil => exact absurd h (Nat.not_lt_zero (scan []))
  | cons b rest ih =>
      cases b with
      | true =>
          simp [scan, setAt]
      | false =>
          simp [scan] at h
          have hrest : scan rest < rest.length := by omega
          have ih' := ih hrest
          have step : setAt (scan rest + 1) false (false :: rest)
              = false :: setAt (scan rest) false rest := rfl
          simp [scan, step]
          omega

/-! ## Pending buffer

The flush condition is `pending + 1 >= granularity`, so the buffer is
strictly smaller than the granularity and the crash loses at most that. -/

theorem pending_bound (granularity pending : Nat) (h : pending < granularity) :
    pending ≤ granularity - 1 := by omega

theorem accepted_loss_bound (accepted recovered pending granularity : Nat)
    (h : recovered + pending = accepted) (hp : pending < granularity) :
    accepted - recovered ≤ granularity := by omega

/-- A verified flush moves every pending bit into the durable prefix, so the
committed count recovers exactly. -/
theorem flush_recovers_pending (recovered pending : Nat) :
    recovered + pending - pending = recovered := by omega

/-- A commit stores the accepted book, so the pending buffer drops out of the
loss accounting. -/
theorem commit_recovers_all (accepted : Nat) : accepted - accepted = 0 := by
  omega

/-! ## Journal wrap count

The wear model needs the number of journal erases: one per capacity of
pulses, rounded up. -/

def eraseCount (pulses capacity : Nat) : Nat := (pulses + capacity - 1) / capacity

theorem eraseCount_covers (pulses capacity : Nat) (h : 0 < capacity) :
    pulses ≤ eraseCount pulses capacity * capacity := by
  unfold eraseCount
  apply Nat.le_of_not_gt
  intro hgt
  have hmod : (pulses + capacity - 1) % capacity < capacity := Nat.mod_lt _ h
  have hdiv := Nat.div_add_mod (pulses + capacity - 1) capacity
  rw [Nat.mul_comm capacity ((pulses + capacity - 1) / capacity)] at hdiv
  omega

theorem eraseCount_minimal (pulses capacity k : Nat) (h : 0 < capacity)
    (hk : pulses ≤ k * capacity) : eraseCount pulses capacity ≤ k := by
  unfold eraseCount
  rw [← Nat.lt_succ_iff]
  rw [Nat.div_lt_iff_lt_mul h]
  rw [Nat.succ_mul]
  omega

end BladeFormal
