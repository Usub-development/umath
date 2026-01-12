# uint128

`usub::umath::uint128` is a portable unsigned 128-bit integer.

## Storage
- Two 64-bit words: `(hi, lo)`
- Uses `unsigned __int128` fast-path when available.

## Operations
- `+ - * / %`
- bitwise `~ & | ^`
- shifts `<< >>`
- comparisons and streaming

## Formatting
- `to_string()` outputs base-10 without leading zeros.

## Notes
Division uses a generic `div_mod()` fallback when `__int128` is not available.
