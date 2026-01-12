# Parsing & formatting

## Accepted syntax
- Optional leading sign: `+` or `-`
- Digits with optional single `.`
- No exponent form (`e` / `E`)

Examples:
- `0`
- `-0.1`
- `123.0004`

## Formatting
- `uint128/int128`: minimal digits, no leading zeros.
- `Numeric128<P,S>`: prints exactly `S` digits after `.` when `S>0`.
- `Numeric`: prints according to current `scale()` (no trimming beyond internal normalization).
