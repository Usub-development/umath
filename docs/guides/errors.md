# Error handling rules

All core numeric types avoid exceptions.

## Err
- `None`: value is valid
- `Invalid`: parse failure / invalid input
- `Overflow`: exceeds limits or internal operation overflow
- `DivByZero`: division by zero

## Propagation
- If an operand is in error state, operators return that operand (first error wins).
- Comparisons on invalid values are not meaningful; prefer `ok()` checks before comparing.

## Conversions
- `Numeric128::checked()` returns `std::expected<Numeric128, Err>`.
- `Numeric::parse_checked()` and `Numeric::from_int64_checked()` return `std::expected<Numeric, Err>`.
