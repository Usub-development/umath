#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <random>
#include <string>

#include "umath/ExtendedInt.h"

using usub::umath::uint128;
using usub::umath::int128;
using usub::umath::uint256;
using usub::umath::int256;

static uint128 U128(std::uint64_t hi, std::uint64_t lo) { return uint128{hi, lo}; }
static int128 S128(std::int64_t hi, std::uint64_t lo) { return int128{hi, lo}; }

static uint256 U256(uint128 hi, uint128 lo) { return uint256{hi, lo}; }
static uint256 U256(std::uint64_t v) { return uint256{v}; }

static int256 S256(uint128 hi, uint128 lo) { return int256{hi, lo}; }
static int256 S256(std::int64_t v) { return int256{v}; }

#if defined(__SIZEOF_INT128__)
using u128w = unsigned __int128;
using i128w = __int128;

static u128w to_wide(uint128 v) {
    return (static_cast<u128w>(v.high()) << 64) | static_cast<u128w>(v.low());
}

static uint128 from_wide(u128w w) {
    return uint128{static_cast<std::uint64_t>(w >> 64), static_cast<std::uint64_t>(w)};
}

static i128w to_wide(int128 v) {
    return (static_cast<i128w>(static_cast<std::int64_t>(v.high())) << 64) | static_cast<i128w>(v.low());
}

static int128 from_wide(i128w w) {
    return int128{
        static_cast<std::int64_t>(static_cast<std::uint64_t>(w >> 64)),
        static_cast<std::uint64_t>(w)
    };
}

static u128w to_wide_u256_low(uint256 v) {
    const uint128 lo = v.low();
    return (static_cast<u128w>(lo.high()) << 64) | static_cast<u128w>(lo.low());
}

static uint256 from_wide_u256(u128w w) {
    return uint256{uint128{0, 0}, from_wide(w)};
}

static i128w to_wide_i256_low(int256 v) {
    const uint128 hi = v.high();
    const uint128 lo = v.low();
    (void) hi;
    const i128w w = (static_cast<i128w>(static_cast<std::int64_t>(lo.high())) << 64) | static_cast<i128w>(lo.low());
    return w;
}

static int256 from_wide_i256(i128w w) {
    const int128 s = from_wide(w);
    const bool neg = (s.high() < 0);
    const uint128 hi = neg ? uint128{~0ull, ~0ull} : uint128{0, 0};
    const uint128 lo = uint128{static_cast<std::uint64_t>(s.high()), s.low()};
    return int256{hi, lo};
}
#endif

TEST(UInt256, ConstructionAndParts) {
    uint256 z{};
    EXPECT_EQ(z.high(), (uint128{0, 0}));
    EXPECT_EQ(z.low(), (uint128{0, 0}));
    EXPECT_EQ(z.to_string(), "0");

    uint256 a{123u};
    EXPECT_EQ(a.high(), (uint128{0, 0}));
    EXPECT_EQ(a.low(), (uint128{0, 123}));
    EXPECT_EQ(a.to_string(), "123");

    uint256 b{-1};
    auto umax = (std::numeric_limits<uint256>::max)();
    EXPECT_EQ(b, umax);
}

TEST(Int256, ConstructionAndParts) {
    int256 z{};
    EXPECT_EQ(z.high(), (uint128{0, 0}));
    EXPECT_EQ(z.low(), (uint128{0, 0}));
    EXPECT_EQ(z.to_string(), "0");

    int256 p{123};
    EXPECT_EQ(p.to_string(), "123");

    int256 n{-123};
    EXPECT_EQ(n.to_string(), "-123");

    int256 m{-1};
    EXPECT_EQ(m.to_string(), "-1");
}

TEST(UInt256, AddSubBasic) {
    uint256 x = U256(uint128{0, 0}, uint128{0, 10});
    uint256 y = U256(uint128{0, 0}, uint128{0, 20});

    EXPECT_EQ(x + y, U256(uint128{0, 0}, uint128{0, 30}));
    EXPECT_EQ(y - x, U256(uint128{0, 0}, uint128{0, 10}));
    EXPECT_EQ(x - x, U256(uint128{0, 0}, uint128{0, 0}));

    uint256 a = U256(uint128{0, 0}, uint128{~0ull, ~0ull});
    uint256 b = U256(uint128{0, 0}, uint128{0, 1});
    uint256 s = a + b;
    EXPECT_EQ(s.low(), (uint128{0, 0}));
    EXPECT_EQ(s.high(), (uint128{0, 1}));
}

TEST(UInt256, MulDivModBasic) {
    uint256 x{10u};
    uint256 y{20u};

    EXPECT_EQ(x * y, uint256{200u});
    EXPECT_EQ(y / x, uint256{2u});
    EXPECT_EQ(y % x, uint256{0u});
}

TEST(UInt256, Shifts) {
    uint256 one{1u};

    EXPECT_EQ(one << 0, one);
    EXPECT_EQ(one << 1, uint256{2u});

    uint256 sh64 = one << 64;
    EXPECT_EQ(sh64.high(), (uint128{0, 0}));
    EXPECT_EQ(sh64.low(), (uint128{1, 0}));

    uint256 sh128 = one << 128;
    EXPECT_EQ(sh128.high(), (uint128{0, 1}));
    EXPECT_EQ(sh128.low(), (uint128{0, 0}));

    uint256 sh255 = one << 255;
    EXPECT_EQ(sh255.high(), (uint128{0x8000'0000'0000'0000ull, 0}));
    EXPECT_EQ(sh255.low(), (uint128{0, 0}));

    EXPECT_EQ((sh128 >> 128), one);
    EXPECT_EQ((sh255 >> 255), one);

    EXPECT_EQ((one << 256), U256(uint128{0, 0}, uint128{0, 0}));
    EXPECT_EQ((one >> 256), U256(uint128{0, 0}, uint128{0, 0}));
}

TEST(UInt256, BitOps) {
    uint256 a = U256(uint128{~0ull, ~0ull}, uint128{0, 0});
    uint256 b = U256(uint128{0, 0}, uint128{~0ull, ~0ull});

    auto umax = (std::numeric_limits<uint256>::max)();

    EXPECT_EQ((a | b), umax);
    EXPECT_EQ((a & b), U256(uint128{0, 0}, uint128{0, 0}));
    EXPECT_EQ((a ^ b), umax);
    EXPECT_EQ((~U256(0u)), umax);
}

TEST(UInt256, OverflowIsModulo) {
    auto umax = (std::numeric_limits<uint256>::max)();
    EXPECT_EQ(umax + uint256{1u}, uint256{0u});
    EXPECT_EQ(uint256{0u} - uint256{1u}, umax);
}

TEST(UInt256, ToStringSamples) {
    EXPECT_EQ(uint256{0u}.to_string(), "0");
    EXPECT_EQ(uint256{1u}.to_string(), "1");
    EXPECT_EQ(uint256{10u}.to_string(), "10");

    uint256 two128 = U256(uint128{0, 1}, uint128{0, 0});
    EXPECT_EQ(two128.to_string(), "340282366920938463463374607431768211456");

    auto umax = (std::numeric_limits<uint256>::max)();
    EXPECT_EQ(umax.to_string(),
              "115792089237316195423570985008687907853269984665640564039457584007913129639935");
}

TEST(Int256, BasicArith) {
    int256 p{100};
    int256 n{-30};

    EXPECT_EQ((p + n).to_string(), "70");
    EXPECT_EQ((p - n).to_string(), "130");
    EXPECT_EQ((p * n).to_string(), "-3000");

    EXPECT_EQ((int256{-42} / int256{2}).to_string(), "-21");
    EXPECT_EQ((int256{-42} % int256{5}).to_string(), "-2");
}

TEST(Int256, ArithmeticShiftRight) {
    for (int k: {0, 1, 7, 63, 64, 100, 127, 128, 200, 255}) {
        EXPECT_EQ((int256{-1} >> k).to_string(), "-1");
    }
    EXPECT_EQ((int256{-2} >> 1).to_string(), "-1");
    EXPECT_EQ((int256{-3} >> 1).to_string(), "-2");
}

TEST(Int256, UnaryMinus) {
    EXPECT_EQ((-int256{0}).to_string(), "0");
    EXPECT_EQ((-int256{1}).to_string(), "-1");
    EXPECT_EQ((-int256{-1}).to_string(), "1");
}

TEST(UInt256, DivModIdentityRandom) {
    std::mt19937_64 rng(777);

    for (int i = 0; i < 5000; ++i) {
        uint256 a = U256(U128(rng(), rng()), U128(rng(), rng()));
        uint256 b = U256(U128(rng(), rng()), U128(rng(), rng()));
        if (b == uint256{0u}) b = uint256{1u};

        uint256 q = a / b;
        uint256 r = a % b;

        EXPECT_EQ(b * q + r, a);

        EXPECT_TRUE(r < b);
    }
}

TEST(Int256, DivModIdentityRandom) {
    std::mt19937_64 rng(888);

    for (int i = 0; i < 5000; ++i) {
        uint256 ua = U256(U128(rng(), rng()), U128(rng(), rng()));
        uint256 ub = U256(U128(rng(), rng()), U128(rng(), rng()));
        if (ub == uint256{0u}) ub = uint256{1u};

        int256 a{ua};
        int256 b{ub};

        if ((rng() & 1ull) != 0) a = -a;
        if ((rng() & 1ull) != 0) b = -b;
        if (b.to_string() == "0") b = int256{1};

        int256 q = a / b;
        int256 r = a % b;

        EXPECT_EQ(b * q + r, a);

        uint256 ar = (r < int256{0}) ? uint256{static_cast<uint256>(-r)} : uint256{static_cast<uint256>(r)};
        uint256 ab = (b < int256{0}) ? uint256{static_cast<uint256>(-b)} : uint256{static_cast<uint256>(b)};
        EXPECT_TRUE(ar < ab);
    }
}

#if defined(__SIZEOF_INT128__)
TEST(UInt256, RandomAgainstBuiltinWithin128_NoOverflow) {
    std::mt19937_64 rng(12345);

    for (int i = 0; i < 30000; ++i) {
        const u128w wa = (static_cast<u128w>(rng()) << 64) | static_cast<u128w>(rng());
        const u128w wb = (static_cast<u128w>(rng()) << 64) | static_cast<u128w>(rng());

        const uint256 a = from_wide_u256(wa);
        const uint256 b = from_wide_u256(wb);

        const u128w ws = wa + wb;
        if (ws >= wa) {
            const uint256 s = a + b;
            EXPECT_EQ(s.high(), (uint128{0, 0}));
            EXPECT_EQ(to_wide_u256_low(s), ws);
        }

        if (wa >= wb) {
            const uint256 d = a - b;
            EXPECT_EQ(d.high(), (uint128{0, 0}));
            EXPECT_EQ(to_wide_u256_low(d), wa - wb);
        }

        const std::uint64_t x = rng();
        const std::uint64_t y = rng();
        const u128w wp = static_cast<u128w>(x) * static_cast<u128w>(y);
        const uint256 px = uint256{x};
        const uint256 py = uint256{y};
        const uint256 p = px * py;
        EXPECT_EQ(p.high(), (uint128{0, 0}));
        EXPECT_EQ(to_wide_u256_low(p), wp);

        if (wb != 0) {
            const uint256 q = a / b;
            const uint256 r = a % b;
            EXPECT_EQ(q.high(), (uint128{0, 0}));
            EXPECT_EQ(r.high(), (uint128{0, 0}));
            EXPECT_EQ(to_wide_u256_low(q), wa / wb);
            EXPECT_EQ(to_wide_u256_low(r), wa % wb);
        }

        const int sh = static_cast<int>(rng() % 128);

        const uint256 sl = a << sh;
        const uint256 sr = a >> sh;

        const u128w wh = (sh == 0) ? 0 : (wa >> (128 - sh));
        EXPECT_EQ(sl.high(), from_wide(wh));
        EXPECT_EQ(to_wide_u256_low(sl), (wa << sh));

        EXPECT_EQ(sr.high(), (uint128{0, 0}));
        EXPECT_EQ(to_wide_u256_low(sr), (wa >> sh));
    }
}

TEST(Int256, RandomAgainstBuiltinWithin128) {
    std::mt19937_64 rng(54321);

    for (int i = 0; i < 30000; ++i) {
        const std::int64_t ahi = static_cast<std::int64_t>(rng() % 2048) - 1024;
        const std::int64_t bhi = static_cast<std::int64_t>(rng() % 2048) - 1024;
        const std::uint64_t alo = rng();
        const std::uint64_t blo = rng();

        const int128 a128{ahi, alo};
        const int128 b128{bhi, blo};

        const i128w wa = to_wide(a128);
        const i128w wb = to_wide(b128);

        const int256 a = from_wide_i256(wa);
        const int256 b = from_wide_i256(wb);

        const i128w ws = wa + wb;
        const i128w wd = wa - wb;
        const i128w wm = wa * wb;

        EXPECT_EQ(to_wide_i256_low(a + b), ws);
        EXPECT_EQ(to_wide_i256_low(a - b), wd);
        EXPECT_EQ(to_wide_i256_low(a * b), wm);

        if (wb != 0) {
            const i128w wq = wa / wb;
            const i128w wr = wa % wb;
            EXPECT_EQ(to_wide_i256_low(a / b), wq);
            EXPECT_EQ(to_wide_i256_low(a % b), wr);
        }

        const int sh = static_cast<int>(rng() % 128);
        const i128w wshr = (sh >= 128) ? (wa < 0 ? -1 : 0) : (wa >> sh);
        EXPECT_EQ(to_wide_i256_low(a >> sh), wshr);
    }
}
#endif
