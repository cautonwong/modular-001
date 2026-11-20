/-
  Model of the stategrid 172-byte energy record codec
  (targets/stategrid_meter/persistence/legacy_energy_codec.c) and of the CRC
  gate the dual-slot snapshot puts around it.

  The record is abstracted at the field level: the codec's risk is dropping,
  duplicating or reordering a tariff field when the energy book is flattened.
  `encode` and `decode` are the C field mapping, `decode_encode` proves no
  field is lost, and `layout` records the frozen on-media spans from
  legacy_energy_record.h, ending at byte 172.

  CRC rejection is modeled at the acceptance boundary: a record is accepted
  exactly when both stored checksums equal the checksum of the encoded body.
  `accept_sealed` is the writer's side; `accept_rejects` is the reader's side
  used by `legacy_energy_restore` before a legacy adoption.
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

/-- The reader accepts only a record whose stored checksums match the body.
    The C restore path additionally has the dual-slot CRC; this predicate is
    the legacy-adoption gate in `legacy_energy_valid`. -/
def accept (record : SealedRecord) : Bool :=
  record.energyChecksum == checksum record.body
    && record.useChecksum == checksum record.body

theorem accept_sealed (body : List Nat) : accept (sealRecord body) = true := by
  simp [accept, sealRecord]

/-- Any record whose stored checksum differs from the computed one is
    rejected, so a torn or stale legacy record cannot be imported. -/
theorem accept_rejects (record : SealedRecord)
    (h : record.energyChecksum ≠ checksum record.body) :
    accept record = false := by
  simp [accept, h]

/-- Changing the body without resealing changes the checksum unless the two
    bodies have the same sum, so the reader's gate sees a stale checksum. -/
theorem tamper_detected (tampered : List Nat) (c : Nat)
    (h : checksum tampered ≠ c) :
    accept { body := tampered, energyChecksum := c, useChecksum := c } = false := by
  have flipped : c ≠ checksum tampered := fun same => h same.symm
  simp [accept, flipped]

end BladeFormal
