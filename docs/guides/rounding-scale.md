# Rounding & scale

## Rounding modes
- `Trunc`: toward zero.
- `HalfUp`: if the first dropped digit is >= 5, increment the kept value (preserves sign).

## Numeric128
- Scale is compile-time (`S`).
- Parsing rounds to `S`.
- `mul/div` round to `S`.

## Numeric
- Scale is stored per value.
- `rescale(new_scale, rnd)`:
  - `new_scale > scale`: multiply by 10^(delta)
  - `new_scale < scale`: divide by 10^(delta) and apply rounding
