/-
  Arithmetic of the stategrid 698 mbase record path.

  The C code is `targets/stategrid_meter/tools/app_resource_tool/resource_tool.c`:
  `search_mid_way` probes marks at `mid = (min + max) / 2` and moves one
  window end past the probe, and `log_output_engine` reads record `i` at
  `(read_ptr + i) % span` where `span = len / unit_size`.

  * `markProbe_bound`: a search window of `n` slots is probed at most
    `ceil(log2 n) + 1` times. Each probe either matches or leaves a window
    of at most half the size; `probeBound` counts that worst case.
  * `SearchStep.lo_le`, `SearchStep.hi_le`: the window only moves inward.
  * `recordIndex_injective`: `(read_ptr + i) % span` is injective for
    `i < span`, so a record scan reads each physical slot at most once.

  The model stops when `search_mid_way` returns at a one-slot window or on
  an exact match. The planned cursor work (`GET-NEXT` continuation) is not
  implemented and is not modelled.
-/
import BladeFormal.ProfileRing

namespace BladeFormal

/-! ### Probe bound -/

/-- Worst-case probe count for a search window of `n` slots: one probe and
    then a window of at most `n / 2`. -/
def probeBound : Nat → Nat
  | 0 => 0
  | 1 => 1
  | n + 2 => 1 + probeBound ((n + 2) / 2)

/-- Ceiling of `log2 n`, with `ceilLog2 0 = 0`. -/
def ceilLog2 : Nat → Nat
  | 0 => 0
  | 1 => 0
  | n + 2 => 1 + ceilLog2 ((n + 3) / 2)

theorem probeBound_mono : ∀ {a b : Nat}, a ≤ b → probeBound a ≤ probeBound b := by
  intro a b h
  induction b using Nat.strongInductionOn generalizing a with
  | ind b ih =>
    rcases b with _ | _ | b
    · have : a = 0 := by omega
      simp [this, probeBound]
    · rcases a with _ | a
      · simp [probeBound]
      · have ha : a = 0 := by omega
        subst ha
        simp [probeBound]
    · rcases a with _ | _ | a
      · simp [probeBound]
      · simp [probeBound]
        omega
      · have hm : (b + 2) / 2 < b + 2 := Nat.div_lt_self (by omega) (by decide)
        have hhalf : (a + 2) / 2 ≤ (b + 2) / 2 := by omega
        have ih' := ih (a := (a + 2) / 2) ((b + 2) / 2) hm hhalf
        simp only [probeBound]
        omega

theorem log2_half_le {n : Nat} (h : 2 ≤ n) : (n / 2).log2 + 1 ≤ n.log2 := by
  have hn : n ≠ 0 := by omega
  rw [Nat.le_log2 hn]
  have h2 : n / 2 ≠ 0 := by omega
  have hpow : 2 ^ (n / 2).log2 ≤ n / 2 :=
    (Nat.le_log2 h2).mp (Nat.le_refl _)
  rw [Nat.pow_succ]
  have : 2 ^ (n / 2).log2 * 2 ≤ n := by omega
  simpa [Nat.mul_comm, Nat.mul_left_comm, Nat.mul_assoc] using this

theorem probeBound_le_log2 (n : Nat) : probeBound n ≤ n.log2 + 1 := by
  induction n using Nat.strongInductionOn with
  | ind n ih =>
    rcases n with _ | _ | k
    · simp [probeBound]
    · simp [probeBound]
    · have hlt : (k + 2) / 2 < k + 2 := Nat.div_lt_self (by omega) (by decide)
      have hih := ih ((k + 2) / 2) hlt
      have hhalf : ((k + 2) / 2).log2 + 1 ≤ (k + 2).log2 := log2_half_le (by omega)
      simp only [probeBound]
      have hk : k + 1 + 1 = k + 2 := rfl
      rw [hk]
      omega

theorem ceilLog2_spec : ∀ {n : Nat}, 0 < n → n ≤ 2 ^ ceilLog2 n := by
  intro n hn
  induction n using Nat.strongInductionOn with
  | ind n ih =>
    rcases n with _ | _ | k
    · omega
    · simp [ceilLog2]
    · have hm : 0 < (k + 3) / 2 := by omega
      have hlt : (k + 3) / 2 < k + 2 := by omega
      have hih := ih ((k + 3) / 2) hlt hm
      simp only [ceilLog2]
      rw [Nat.pow_add, Nat.pow_one]
      have _hmul : 2 * ((k + 3) / 2) ≤ 2 * 2 ^ ceilLog2 ((k + 3) / 2) :=
        Nat.mul_le_mul_left 2 hih
      have _hcover : k + 2 ≤ 2 * ((k + 3) / 2) := by omega
      omega

theorem log2_le_ceilLog2 {n : Nat} (hn : n ≠ 0) : n.log2 ≤ ceilLog2 n := by
  have hpos : 0 < n := by omega
  have hspec := ceilLog2_spec hpos
  have hlt : n < 2 ^ (ceilLog2 n + 1) := by
    rw [Nat.pow_add, Nat.pow_one]
    have hpow : 0 < 2 ^ ceilLog2 n := Nat.two_pow_pos _
    omega
  have := (Nat.log2_lt hn).mpr hlt
  omega

/-- Probe bound in the shape the C search needs: `ceil(log2 n) + 1`. -/
theorem probeBound_le_ceilLog2 (n : Nat) : probeBound n ≤ ceilLog2 n + 1 := by
  rcases n with _ | n
  · simp [probeBound, ceilLog2]
  · have hlog := probeBound_le_log2 (n + 1)
    have hceil := log2_le_ceilLog2 (n := n + 1) (by omega)
    omega

theorem probeBound_window {n : Nat} (h : 0 < n) :
    probeBound n = 1 + probeBound (n / 2) := by
  rcases n with _ | _ | k
  · omega
  · simp [probeBound]
  · simp only [probeBound]

/-! ### Search window -/

/-- Inclusive window size of the mark search. -/
def searchWindow (lo hi : Nat) : Nat := hi + 1 - lo

/-- One continuing probe of `search_mid_way`: the key is below or above the
    mark at `mid = (lo + hi) / 2`, so one window end moves past `mid`. A
    probe at a one-slot window returns instead of stepping. -/
inductive SearchStep (mark : Nat → Nat) (key : Nat) :
    Nat × Nat → Nat × Nat → Prop
  | left {lo hi : Nat} (h : lo ≤ hi) (hmore : lo < (lo + hi) / 2)
      (hlt : key < mark ((lo + hi) / 2)) :
      SearchStep mark key (lo, hi) (lo, (lo + hi) / 2 - 1)
  | right {lo hi : Nat} (h : lo ≤ hi) (hmore : (lo + hi) / 2 < hi)
      (hgt : mark ((lo + hi) / 2) < key) :
      SearchStep mark key (lo, hi) ((lo + hi) / 2 + 1, hi)

/-- Both window ends only move inward. -/
theorem SearchStep.lo_le {mark : Nat → Nat} {key : Nat} {w w' : Nat × Nat}
    (h : SearchStep mark key w w') : w.1 ≤ w'.1 := by
  cases h with
  | left h _ _ => simp
  | right h _ _ => simp; omega

theorem SearchStep.hi_le {mark : Nat → Nat} {key : Nat} {w w' : Nat × Nat}
    (h : SearchStep mark key w w') : w'.2 ≤ w.2 := by
  cases h with
  | left h _ _ => simp; omega
  | right h _ _ => simp

/-- A continuing probe at least halves the window. -/
theorem SearchStep.window_halves {mark : Nat → Nat} {key : Nat} {lo hi : Nat}
    {w' : Nat × Nat} (h : SearchStep mark key (lo, hi) w') :
    searchWindow w'.1 w'.2 ≤ searchWindow lo hi / 2 := by
  cases h with
  | left h hm _ =>
    simp [searchWindow]
    omega
  | right h hm _ =>
    simp [searchWindow]
    omega

/-- A finite sequence of continuing probes. -/
inductive SearchPath (mark : Nat → Nat) (key : Nat) :
    Nat → Nat × Nat → Nat × Nat → Prop
  | nil (w : Nat × Nat) : SearchPath mark key 0 w w
  | step {n : Nat} {w w' w'' : Nat × Nat} :
      SearchStep mark key w w' →
      SearchPath mark key n w' w'' →
      SearchPath mark key (n + 1) w w''

theorem searchPath_length_le {mark : Nat → Nat} {key : Nat} :
    ∀ {n : Nat} {w w' : Nat × Nat},
      SearchPath mark key n w w' →
      n ≤ probeBound (searchWindow w.1 w.2) := by
  intro n w w' path
  induction path with
  | nil w => simp [probeBound]
  | step hs rest ih =>
    rename_i w0 w1 w2
    have hhalf : searchWindow w1.1 w1.2 ≤ searchWindow w0.1 w0.2 / 2 := hs.window_halves
    have hpos : 0 < searchWindow w0.1 w0.2 := by
      cases hs with
      | left h _ _ => simp [searchWindow]; omega
      | right h _ _ => simp [searchWindow]; omega
    have hmono := probeBound_mono hhalf
    have hpb := probeBound_window hpos
    omega

/-- Any mark search over an `n`-slot window probes at most
    `ceil(log2 n) + 1` times. -/
theorem markProbe_bound {mark : Nat → Nat} {key : Nat} {lo hi n : Nat}
    {w' : Nat × Nat} (h : SearchPath mark key n (lo, hi) w') :
    n ≤ ceilLog2 (searchWindow lo hi) + 1 := by
  have hlen := searchPath_length_le h
  have hpb := probeBound_le_ceilLog2 (searchWindow lo hi)
  simp only [Prod.fst, Prod.snd] at hlen
  omega

/-- The 64-slot window `test_mbase_search_counts` instruments: the worst case
    is 7 probes, inside the documented `ceil(log2 64) + 1 = 7` bound (the
    looser `probeBound ≤ ceilLog2 + 1` form gives the same 7). -/
theorem mbase_64_slot_probe_bound : probeBound (searchWindow 0 63) ≤ 7 := by
  simp [probeBound, searchWindow]

/-! ### Record index -/

/-- Logical-to-physical mapping of one record read in the circular log:
    `(read_ptr + i) % span`. -/
def recordIndex (read_ptr span i : Nat) : Nat := (read_ptr + i) % span

/-- The record index read is injective on one span, so a scan reads each
    physical slot at most once. This is the `% span` form of
    `add_mod_injective`. -/
theorem recordIndex_injective {read_ptr span : Nat} (hspan : 0 < span) :
    ∀ ⦃i j : Nat⦄, i < span → j < span →
      recordIndex read_ptr span i = recordIndex read_ptr span j → i = j := by
  intro i j hi hj h
  exact add_mod_injective hspan hi hj (by simpa [recordIndex, Nat.add_assoc] using h)

end BladeFormal
