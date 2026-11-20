/-
  Arithmetic of the profile record ring and the two-pass GET.

  The C code is `targets/idis_ip_meter/cosem/nd_profile_store.c` (ring mapping
  and slot validation) and `cosem/classes/nd_ic_profile_generic.c` (count_rows
  then write_rows).

  * `ringSlot_injective`: the live-window index -> physical slot map used by
    `nd_profile_store_read` is injective, so a scan reads each row once.
  * `spare_sector_disjoint`: `entries + sector_slots <= phys` keeps the sector
    the next append erases outside the live window.
  * `sectorsTouched`: a contiguous scan touches
    `(start % slotsPerSector + records - 1) / slotsPerSector + 1` sectors. For a
    sector-aligned scan of 128-byte slots in 4096-byte sectors that is
    `ceil(records * 128 / 4096)`, the fill count `test_sector_store_window`
    asserts.
  * `valueRangeReads`: the value-range GET reads one record per entry in the
    count pass and one per entry in the write pass, so `2 * entries` reads, the
    count `test_profile_value_range_get_read_counts` asserts.

  The planned time-window probe path is not implemented and is not modelled.
-/
namespace BladeFormal

/-! ### Ring mapping -/

/-- Congruence of `b + i` modulo `p` is injective on `[0, p)`. -/
theorem add_mod_injective {p b i j : Nat} (hp : 0 < p) (hi : i < p)
    (hj : j < p) (h : (b + i) % p = (b + j) % p) : i = j := by
  have hb : b % p < p := Nat.mod_lt _ hp
  have hstep : (b % p + i) % p = (b % p + j) % p := by
    calc (b % p + i) % p = (b % p + i % p) % p := by
          rw [Nat.mod_eq_of_lt hi]
      _ = (b + i) % p := (Nat.add_mod b i p).symm
      _ = (b + j) % p := h
      _ = (b % p + j % p) % p := Nat.add_mod b j p
      _ = (b % p + j) % p := by rw [Nat.mod_eq_of_lt hj]
  have hmod : ∀ k : Nat, k < p → (b % p + k) % p
      = if b % p + k < p then b % p + k else b % p + k - p := by
    intro k hk
    by_cases hlt : b % p + k < p
    · rw [if_pos hlt, Nat.mod_eq_of_lt hlt]
    · rw [if_neg hlt]
      have hle : p ≤ b % p + k := by omega
      rw [Nat.mod_eq_sub_mod hle]
      rw [Nat.mod_eq_of_lt (by omega)]
  rw [hmod i hi, hmod j hj] at hstep
  split at hstep <;> split at hstep <;> omega

/-- Physical slot of logical index `i` in the live window. `oldest` is
    `(head + phys - entries) % phys`, so this is exactly the mapping in
    `nd_profile_store_read`. -/
def ringSlot (phys head entries i : Nat) : Nat :=
  (head + phys - entries + i) % phys

/-- The logical-to-physical mapping is injective on the live window. -/
theorem ringSlot_injective {phys head entries : Nat} (hp : 0 < phys)
    (he : entries ≤ phys) :
    ∀ ⦃i j : Nat⦄, i < entries → j < entries →
      ringSlot phys head entries i = ringSlot phys head entries j → i = j := by
  intro i j hi hj h
  have hi' : i < phys := Nat.lt_of_lt_of_le hi he
  have hj' : j < phys := Nat.lt_of_lt_of_le hj he
  exact add_mod_injective hp hi' hj' (by simpa [ringSlot, Nat.add_assoc] using h)

/-! ### Erase budget -/

/-- A live slot is at circular distance `phys - entries` or more from the
    head; the spare sector is closer than `sector_slots`. When
    `entries + sector_slots <= phys` the two sets cannot meet, so the
    pre-append erase never touches a live record. -/
theorem spare_sector_disjoint (o entries sector_slots phys : Nat)
    (hcap : entries + sector_slots ≤ phys) (hspare : o < sector_slots)
    (hlive : phys - entries ≤ o) : False := by
  omega

/-! ### Sector fills -/

/-- Sectors touched by a contiguous scan of `records` slots starting at slot
    `start`; one window fill per sector. -/
def sectorsTouched (start records slotsPerSector : Nat) : Nat :=
  (start % slotsPerSector + records - 1) / slotsPerSector + 1

/-- A sector-aligned contiguous scan of `records` 128-byte slots touches
    `ceil(records * 128 / 4096)` sectors. -/
theorem sectorsTouched_aligned (records : Nat) (hrec : 0 < records) :
    sectorsTouched 0 records 32 = (records * 128 + 4095) / 4096 := by
  unfold sectorsTouched
  simp
  omega

/-- The 1152-slot PQ scan touches at most 37 sectors: 36 full sectors plus a
    partial first one. -/
theorem pq_full_scan_fills_le (start : Nat) :
    sectorsTouched start 1152 32 ≤ 37 := by
  unfold sectorsTouched
  have hmod : start % 32 < 32 := Nat.mod_lt _ (by decide)
  omega

/-! ### Two-pass read budget -/

/-- One record read per scanned index. -/
def scanReads : Nat → Nat
  | 0 => 0
  | n + 1 => 1 + scanReads n

theorem scanReads_eq (n : Nat) : scanReads n = n := by
  induction n with
  | zero => simp [scanReads]
  | succ k ih => simp [scanReads, ih]; omega

/-- `count_rows` scans every entry, then `write_rows` scans every entry again.
    A matching row costs no third read: the predicate hands its record to the
    encoder. -/
def valueRangeReads (entries : Nat) : Nat := scanReads entries + scanReads entries

theorem valueRangeReads_eq (entries : Nat) :
    valueRangeReads entries = 2 * entries := by
  simp [valueRangeReads, scanReads_eq]
  omega

/-- The three-row case `test_profile_value_range_get_read_counts` asserts:
    2 * 3 value-range reads, 3 full-GET reads, 2 entry-range reads. -/
theorem pq_test_read_counts :
    valueRangeReads 3 = 6 ∧ scanReads 3 = 3 ∧ scanReads 2 = 2 := by decide

end BladeFormal
