# Compatibility notes

## Compilers
- Works with GCC/Clang/MSVC in C++23 mode.
- `__SIZEOF_INT128__` fast-path used where supported.

## ABI / headers
- Prefer including only the needed headers:
  - `umath/Int128.h`
  - `umath/NumericFixed.h`

## Formatting
`to_string()` is intended to be stable across platforms (same output for same stored value).
