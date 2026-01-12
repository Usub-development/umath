# umath

C++23 numeric building blocks:

- **uint128 / int128**: portable 128-bit integers (uses `__int128` when available, otherwise pure C++).
- **Numeric128<P,S>**: fixed-scale decimal backed by `int128`, with precision `P` (<= 38) and compile-time scale `S`.
- **Numeric**: arbitrary-precision decimal using base-1e9 limbs, with PostgreSQL-like limits:
  - up to **131072** digits before the decimal point
  - up to **16383** digits after the decimal point

## Design
- No exceptions in core types (errors are stored as `Err`).
- Rounding modes: `Trunc`, `HalfUp`.
- Deterministic `to_string()` / parsing.

## Quick example
```cpp
#include <iostream>
#include "umath/NumericFixed.h"

using usub::umath::Numeric;
using usub::umath::Numeric128;
using usub::umath::Rounding;

int main() {
  Numeric128<38,4> a("12.34005", Rounding::HalfUp);
  Numeric128<38,4> b("2.5000");
  std::cout << (a * b).to_string() << "\n"; // 30.8501

  Numeric x("1.23");
  Numeric y("2.5");
  std::cout << (x + y).to_string() << "\n"; // 3.73
}
```
