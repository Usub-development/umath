# Representation

## uint128 / int128
Stored as two 64-bit limbs:
- `hi` (most significant)
- `lo` (least significant)

Signed `int128` is stored in two's complement.

## Numeric128
Stores scaled integer:
- `raw = value * 10^S`

## Numeric
Stores magnitude in base 1e9:
- `mag_[0]` is least significant limb
- separate `scale_` (decimal digits after point)
- separate sign flag
