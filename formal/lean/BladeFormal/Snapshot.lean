/-
  Model of the stategrid 172-byte energy record codec
  (targets/stategrid_meter/persistence/legacy_energy_codec.c) and of the CRC
  gate the dual-slot snapshot puts around it.

  The record is abstracted at the field level: the codec's risk is dropping,
  duplicating or reordering a tariff field when the energy book is flattened.
  `encode` and `decode` are the C field mapping, `decode_encode` proves no
  field is lost, and `layout` records the frozen on-media spans from
  legacy_energy_record.h, ending at byte 172.

  CRC rejection is modeled per half: `acceptEnergy` checks the energy
  checksum, `acceptMonth` checks the month checksum, and `accept` is their
  conjunction. `accept_sealed` is the writer's side; the two rejection
  theorems are the reader's gates, and the torn theorems show a valid half is
  still adopted while the whole record is rejected. `legacy_energy_store.c`
  adopts the energy and month halves independently, so a torn half of one
  source does not discard the other half.
-/
namespace BladeFormal

/-- One energy book as the codec sees it: four 13-slot channels plus the
    monthly amount. Values are already the stored integer/fraction pairs. -/
structure LegacyEnergy where
  activeP : List Nat
  activeN : List Nat
  activePDec : List Nat
  activeNDec : List Nat
  monthDec : Nat
  month : Nat
deriving DecidableEq, Repr

/-- The flattened media image, in `legacy_energy_record_t` field order. The
    padding word and the two checksum words are carried separately by `seal`. -/
structure EncodedEnergy where
  activeP : List Nat
  activeN : List Nat
  activePDec : List Nat
  activeNDec : List Nat
  monthDec : Nat
  month : Nat
deriving DecidableEq, Repr

def encode (energy : LegacyEnergy) : EncodedEnergy where
  activeP := energy.activeP
  activeN := energy.activeN
  activePDec := energy.activePDec
  activeNDec := energy.activeNDec
  monthDec := energy.monthDec
  month := energy.month

def decode (words : EncodedEnergy) : LegacyEnergy where
  activeP := words.activeP
  activeN := words.activeN
  activePDec := words.activePDec
  activeNDec := words.activeNDec
  monthDec := words.monthDec
  month := words.month

/-- The codec is a bijection on the six field groups: no field is dropped,
    duplicated or swapped. -/
theorem decode_encode (energy : LegacyEnergy) : decode (encode energy) = energy := by
  cases energy
  rfl

theorem encode_decode (words : EncodedEnergy) : encode (decode words) = words := by
  cases words
  rfl

/-- Frozen field spans from legacy_energy_record.h: offset and width in bytes.
    Every value is an 8-bit addressable byte in the real record. -/
def layout : List (Nat × Nat) :=
  [ (0, 52), (52, 52), (104, 26), (130, 26),
    (156, 4), (160, 2), (162, 2), (164, 4), (168, 4) ]

theorem layout_ends_at_172 : layout.foldl (fun total span => total + span.2) 0 = 172 := by
  decide

/-- Every field starts at a distinct byte offset. -/
def distinct : List Nat → Bool
  | [] => true
  | x :: xs => !xs.contains x && distinct xs

theorem layout_offsets_distinct : distinct (layout.map (fun span => span.1)) = true := by
  decide

/-- Stored checksum over one byte body, mirroring `legacy_energy_checksum`:
    0xA5A5 plus every word, truncated to 16 bits. -/
def checksum (body : List Nat) : Nat :=
  (0xA5A5 + body.foldl (fun total word => total + word) 0) % 65536

/-- A sealed record: the encoded body, the mandatory padding word (always 0)
    and the two checksum words the reader validates. -/
structure SealedRecord where
  body : List Nat
  energyChecksum : Nat
  useChecksum : Nat
deriving DecidableEq, Repr

def sealRecord (body : List Nat) : SealedRecord where
  body := body
  energyChecksum := checksum body
  useChecksum := checksum body

def acceptEnergy (record : SealedRecord) : Bool :=
  record.energyChecksum == checksum record.body

def acceptMonth (record : SealedRecord) : Bool :=
  record.useChecksum == checksum record.body

/-- The whole-record gate: both halves valid. The C restore path additionally
    has the dual-slot CRC; this predicate is the legacy-adoption gate in
    `legacy_energy_valid`. -/
def accept (record : SealedRecord) : Bool :=
  acceptEnergy record && acceptMonth record

theorem accept_sealed (body : List Nat) : accept (sealRecord body) = true := by
  simp [accept, acceptEnergy, acceptMonth, sealRecord]

theorem accept_rejects_energy (record : SealedRecord)
    (h : record.energyChecksum ≠ checksum record.body) :
    accept record = false := by
  simp [accept, acceptEnergy, h]

theorem accept_rejects_month (record : SealedRecord)
    (h : record.useChecksum ≠ checksum record.body) :
    accept record = false := by
  simp [accept, acceptMonth, h]

/-- Changing the body without resealing changes the checksum unless the two
    bodies have the same sum, so the reader's gate sees a stale checksum. -/
theorem tamper_detected (tampered : List Nat) (c : Nat)
    (h : checksum tampered ≠ c) :
    accept { body := tampered, energyChecksum := c, useChecksum := c } = false := by
  have flipped : c ≠ checksum tampered := fun same => h same.symm
  simp [accept, acceptEnergy, acceptMonth, flipped]

theorem torn_month_keeps_energy (body : List Nat) (bad : Nat)
    (h : bad ≠ checksum body) :
    acceptEnergy { body := body, energyChecksum := checksum body,
                   useChecksum := bad } = true
      ∧ acceptMonth { body := body, energyChecksum := checksum body,
                      useChecksum := bad } = false
      ∧ accept { body := body, energyChecksum := checksum body,
                 useChecksum := bad } = false := by
  simp [accept, acceptEnergy, acceptMonth, h]

theorem torn_energy_keeps_month (body : List Nat) (bad : Nat)
    (h : bad ≠ checksum body) :
    acceptMonth { body := body, energyChecksum := bad,
                  useChecksum := checksum body } = true
      ∧ acceptEnergy { body := body, energyChecksum := bad,
                       useChecksum := checksum body } = false
      ∧ accept { body := body, energyChecksum := bad,
                 useChecksum := checksum body } = false := by
  simp [accept, acceptEnergy, acceptMonth, h]

end BladeFormal
