/-
  A model of the FIFO capacity arithmetic used by fifoBuf_getUsed and
  fifoBuf_getFree. The C code stores uint16_t indices; this model proves
  the integer contract that the implementation relies on.
-/
namespace BladeFormal

/-- Free space in a ring buffer. -/
def free (capacity used : Nat) : Nat := capacity - used

theorem free_le_capacity (capacity used : Nat) : free capacity used ≤ capacity :=
  Nat.sub_le capacity used

theorem free_of_full (capacity : Nat) : free capacity capacity = 0 := by
  simp [free]

theorem used_add_free (capacity used : Nat) (h : used ≤ capacity) :
    used + free capacity used = capacity := by
  simpa [free] using Nat.add_sub_of_le h

end BladeFormal
