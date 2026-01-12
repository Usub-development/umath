# umath numeric

Two fixed/decimal numeric types:

## `Numeric128<P, S>`
A fast fixed-scale decimal backed by a signed 128-bit integer.

- `P` — total precision (digits), `1..38`
- `S` — fixed scale (fraction digits), `0..P`
- Stored as `raw = value * 10^S`
- Supports parsing, formatting, `+ - * /`, comparisons, and conversions to `long double`
- Errors are tracked inside the value (`Err::None/Invalid/Overflow/DivByZero`)

Example:
```cpp
using N = unumber::numeric::Numeric128<38, 4>;

N a("12.34005");
N b("2.5000");
auto c = a * b;          // rounded half-up
std::cout << c << "\n";  // 30.8501
````

## `Numeric`

A PostgreSQL-like arbitrary-precision decimal with a dynamic scale.

* Magnitude is stored in base `1e9` limbs (`std::vector<uint32_t>`)
* `scale` is the number of fractional digits
* Limits match PostgreSQL defaults:

    * up to **131072** digits before the decimal point
    * up to **16383** digits after the decimal point
* Supports parsing, formatting, `+ - * /`, rescaling with rounding (`Trunc` / `HalfUp`)

Example:

```cpp
using unumber::numeric::Numeric;

Numeric a("1.23");
Numeric b("2.5");

auto s = a + b;                              // 3.73
auto m = Numeric::mul(a, b, 2, Rounding::HalfUp); // 3.08
auto d = Numeric::div(a, b, 4, Rounding::Trunc);  // 0.4920
```

## Rounding

* `Rounding::Trunc`  — toward zero
* `Rounding::HalfUp` — rounds away from zero on a 0.5 tie

## Tests

Unit tests use GoogleTest and include randomized cross-checks against builtin `__int128` reference math when available.