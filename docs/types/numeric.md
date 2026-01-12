# Numeric

`Numeric` is an arbitrary-precision decimal (base 1e9 limbs) with dynamic scale.

## Limits
- up to **131072** digits before the decimal point
- up to **16383** digits after the decimal point

## Construction
```cpp
Numeric a("1.23");
Numeric b(int64_t{-7});
```

## Scale
- `scale()` returns the current number of fractional digits.
- `rescale(new_scale, rounding)` changes scale with rounding.

## Operators
- `+ -` align scales automatically.
- `* /` use default scale heuristics unless you call explicit helpers:
  - `Numeric::mul(a,b,target_scale,rounding)`
  - `Numeric::div(a,b,target_scale,rounding)`

## Formatting
`to_string()` prints normalized decimal form.
