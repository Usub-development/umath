# Limits & guarantees

## uint128 / int128
- Exact integer arithmetic mod 2^128 (for unsigned) / two's complement representation (signed)
- When `__int128` is present, ops use it; otherwise use portable fallback.

## Numeric128<P,S>
- `P <= 38`
- magnitude must be `< 10^P` in scaled integer form (after applying scale)
- division by zero => `Err::DivByZero`

## Numeric
- up to 131072 digits before decimal point
- up to 16383 digits after decimal point
- internal base = 1e9 limbs
