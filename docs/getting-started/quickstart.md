# Quickstart

## 128-bit integers
```cpp
#include "umath/Int128.h"

using usub::umath::uint128;
using usub::umath::int128;

uint128 a = 42;
int128  b = -7;

auto c = a * uint128(0, 10);
auto d = b / int128(2);
```

## Fixed-scale decimal (`Numeric128<P,S>`)
```cpp
#include "umath/NumericFixed.h"
using usub::umath::Numeric128;
using usub::umath::Rounding;

using Money = Numeric128<38, 2>;

Money p("12.345", Rounding::HalfUp); // 12.35
Money q("2.00");

auto s = p + q;
auto m = p * q;
```

## Arbitrary decimal (`Numeric`)
```cpp
#include "umath/NumericFixed.h"
using usub::umath::Numeric;

Numeric a("1.23");
Numeric b("2.5");

auto s = a + b; // 3.73
```
