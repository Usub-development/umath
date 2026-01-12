#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <random>
#include <ranges>
#include <string>
#include <vector>

#include "umath/Int128.h"

using usub::umath::uint128;
using usub::umath::int128;

static uint128 U(uint64_t hi, uint64_t lo) { return uint128{hi, lo}; }
static int128 S(int64_t hi, uint64_t lo) { return int128{hi, lo}; }

#if defined(__SIZEOF_INT128__)
using u128w = unsigned __int128;
using i128w = __int128;

static u128w to_wide(uint128 v) {
    return (static_cast<u128w>(v.high()) << 64) | static_cast<u128w>(v.low());
}

static uint128 from_wide(u128w w) {
    return uint128{static_cast<uint64_t>(w >> 64), static_cast<uint64_t>(w)};
}

static i128w to_wide(int128 v) {
    return (static_cast<i128w>(static_cast<int64_t>(v.high())) << 64) | static_cast<i128w>(v.low());
}

static int128 from_wide(i128w w) {
    return int128{static_cast<int64_t>(static_cast<uint64_t>(w >> 64)), static_cast<uint64_t>(w)};
}
#endif

TEST(UInt128, ConstructionAndParts) {
    uint128 z{};
    EXPECT_EQ(z.high(), 0u);
    EXPECT_EQ(z.low(), 0u);
    EXPECT_EQ(z.to_string(), "0");

    uint128 a{123u};
    EXPECT_EQ(a.high(), 0u);
    EXPECT_EQ(a.low(), 123u);
    EXPECT_EQ(a.to_string(), "123");

    uint128 b{-1};
    auto umax = (std::numeric_limits<uint128>::max)();
    EXPECT_EQ(b, umax);
}

TEST(UInt128, AddSubBasic) {
    uint128 x{0, 10};
    uint128 y{0, 20};

    EXPECT_EQ(x + y, U(0, 30));
    EXPECT_EQ(y - x, U(0, 10));
    EXPECT_EQ(x - x, U(0, 0));
}

TEST(UInt128, MulDivModBasic) {
    uint128 x{0, 10};
    uint128 y{0, 20};

    EXPECT_EQ(x * y, U(0, 200));
    EXPECT_EQ(y / x, U(0, 2));
    EXPECT_EQ(y % x, U(0, 0));
}

TEST(UInt128, Shifts) {
    uint128 one{0, 1};

    EXPECT_EQ(one << 0, one);
    EXPECT_EQ(one << 1, U(0, 2));
    EXPECT_EQ(one << 64, U(1, 0));

    auto sh100 = one << 100;
    EXPECT_EQ(sh100.high(), (1ull << 36));
    EXPECT_EQ(sh100.low(), 0ull);

    EXPECT_EQ((sh100 >> 100), one);
}

TEST(UInt128, BitOps) {
    uint128 a{0xFFFF'FFFF'FFFF'FFFFull, 0};
    uint128 b{0, 0xFFFF'FFFF'FFFF'FFFFull};

    EXPECT_EQ((a | b), (std::numeric_limits<uint128>::max)());
    EXPECT_EQ((a & b), U(0, 0));
    EXPECT_EQ((a ^ b), (std::numeric_limits<uint128>::max)());
}

TEST(UInt128, OverflowIsModulo) {
    auto umax = (std::numeric_limits<uint128>::max)();
    EXPECT_EQ(umax + U(0, 1), U(0, 0));
    EXPECT_EQ(U(0, 0) - U(0, 1), umax);
}

TEST(UInt128, ToStringSamples) {
    EXPECT_EQ(U(0, 0).to_string(), "0");
    EXPECT_EQ(U(0, 1).to_string(), "1");
    EXPECT_EQ(U(0, 10).to_string(), "10");
    EXPECT_EQ(U(1, 0).to_string(), "18446744073709551616");

    auto umax = (std::numeric_limits<uint128>::max)();
    EXPECT_EQ(umax.to_string(), "340282366920938463463374607431768211455");
}

TEST(Int128, BasicArith) {
    int128 p{100};
    int128 n{-30};

    EXPECT_EQ((p + n).to_string(), "70");
    EXPECT_EQ((p - n).to_string(), "130");
    EXPECT_EQ((p * n).to_string(), "-3000");

    EXPECT_EQ((int128{-42} / int128{2}).to_string(), "-21");
    EXPECT_EQ((int128{-42} % int128{5}).to_string(), "-2");
}

TEST(Int128, ArithmeticShiftRight) {
    for (int k: {0, 1, 7, 63, 64, 100, 127}) {
        EXPECT_EQ((int128{-1} >> k).to_string(), "-1");
    }
    EXPECT_EQ((int128{-2} >> 1).to_string(), "-1");
    EXPECT_EQ((int128{-3} >> 1).to_string(), "-2");
}

TEST(Int128, UnaryMinus) {
    EXPECT_EQ((-int128{0}).to_string(), "0");
    EXPECT_EQ((-int128{1}).to_string(), "-1");
    EXPECT_EQ((-int128{-1}).to_string(), "1");
}

#if defined(__SIZEOF_INT128__)
TEST(UInt128, RandomAgainstBuiltin) {
    std::mt19937_64 rng(123);
    for (int i = 0; i < 20000; ++i) {
        uint64_t ah = rng(), al = rng();
        uint64_t bh = rng(), bl = rng();

        uint128 a{ah, al};
        uint128 b{bh, bl};

        u128w wa = (static_cast<u128w>(ah) << 64) | al;
        u128w wb = (static_cast<u128w>(bh) << 64) | bl;

        EXPECT_EQ(to_wide(a + b), wa + wb);
        EXPECT_EQ(to_wide(a - b), wa - wb);
        EXPECT_EQ(to_wide(a * b), wa * wb);

        if (bh != 0 || bl != 0) {
            EXPECT_EQ(to_wide(a / b), wa / wb);
            EXPECT_EQ(to_wide(a % b), wa % wb);
        }

        int sh = static_cast<int>(rng() % 128);
        EXPECT_EQ(to_wide(a << sh), (sh >= 128 ? 0 : (wa << sh)));
        EXPECT_EQ(to_wide(a >> sh), (sh >= 128 ? 0 : (wa >> sh)));
    }
}

TEST(Int128, RandomAgainstBuiltin) {
    std::mt19937_64 rng(456);
    for (int i = 0; i < 20000; ++i) {
        uint64_t alo = rng();
        uint64_t blo = rng();

        int64_t ahi = static_cast<int64_t>(rng() % 1024) - 512;
        int64_t bhi = static_cast<int64_t>(rng() % 1024) - 512;

        int128 a{ahi, alo};
        int128 b{bhi, blo};

        i128w wa = (static_cast<i128w>(ahi) << 64) | static_cast<i128w>(alo);
        i128w wb = (static_cast<i128w>(bhi) << 64) | static_cast<i128w>(blo);

        EXPECT_EQ(to_wide(a + b), wa + wb);
        EXPECT_EQ(to_wide(a - b), wa - wb);
        EXPECT_EQ(to_wide(a * b), wa * wb);

        if (!(bhi == 0 && blo == 0)) {
            EXPECT_EQ(to_wide(a / b), wa / wb);
            EXPECT_EQ(to_wide(a % b), wa % wb);
        }

        int sh = static_cast<int>(rng() % 128);
        i128w wshr = (sh >= 128) ? (wa < 0 ? -1 : 0) : (wa >> sh);
        EXPECT_EQ(to_wide(a >> sh), wshr);
    }
}
#endif
