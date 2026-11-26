/-
  #check fails when a theorem is renamed or removed, and
  tools/check_lean_manifest.py cross-checks this list against the theorem
  declarations in every module.
-/
import BladeFormal.Fifo
import BladeFormal.MbaseSearch
import BladeFormal.ProfileRing
import BladeFormal.Pulse
import BladeFormal.Snapshot

open BladeFormal

-- Fifo (3)
#check free_le_capacity
#check free_of_full
#check used_add_free

-- MbaseSearch (14)
#check probeBound_mono
#check log2_half_le
#check probeBound_le_log2
#check ceilLog2_spec
#check log2_le_ceilLog2
#check probeBound_le_ceilLog2
#check probeBound_window
#check SearchStep.lo_le
#check SearchStep.hi_le
#check SearchStep.window_halves
#check searchPath_length_le
#check markProbe_bound
#check mbase_64_slot_probe_bound
#check recordIndex_injective

-- ProfileRing (8)
#check add_mod_injective
#check ringSlot_injective
#check spare_sector_disjoint
#check sectorsTouched_aligned
#check pq_full_scan_fills_le
#check scanReads_eq
#check valueRangeReads_eq
#check pq_test_read_counts

-- Pulse (10)
#check scan_le_length
#check scan_counts_cleared
#check scan_stops
#check scan_setAt_cursor
#check pending_bound
#check accepted_loss_bound
#check flush_recovers_pending
#check commit_recovers_all
#check eraseCount_covers
#check eraseCount_minimal

-- Snapshot (10)
#check decode_encode
#check encode_decode
#check layout_ends_at_172
#check layout_offsets_distinct
#check accept_sealed
#check accept_rejects_energy
#check accept_rejects_month
#check tamper_detected
#check torn_month_keeps_energy
#check torn_energy_keeps_month
