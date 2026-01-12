# Numeric128

`Numeric128<P,S>` is a fixed-scale decimal backed by `int128`.

- `P` = precision (digits), `1..38`
- `S` = scale (fraction digits), `0..P`
- Stored as `raw = value * 10^S` (`int128`)

## Construction
```cpp
Numeric128<38,4> a("12.34005");              // default HalfUp
Numeric128<38,4> b("12.34005", Rounding::Trunc);
Numeric128<38,4> c(int64_t{42});
```

## Error model
- No exceptions.
- Each value carries `Err`:
  - `None`, `Invalid`, `Overflow`, `DivByZero`
- `ok()` / `error()` / `operator bool()`

## Rounding
Parsing and division/multiplication use:
- `Trunc`  (toward zero)
- `HalfUp` (ties round away from zero)

## Operators
- `+ - * /` between same `P,S`
- mixed with integers
- `to_string()` prints exactly `S` fractional digits (pads with zeros)

## Precision rule
Any operation that produces a value requiring >= `10^P` magnitude is `Overflow`.
