# FAQ

## Why both Numeric128 and Numeric?
- `Numeric128` is fast and fixed-size (fits in `int128`).
- `Numeric` supports huge precision/scale with strict limits.

## Does parsing accept exponent notation?
No.

## Does Numeric trim trailing zeros?
Normalization trims internal zero limbs; string formatting keeps exact scale placement implied by `scale_`.
