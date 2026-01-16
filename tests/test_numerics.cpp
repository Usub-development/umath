#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <random>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "umath/Numeric.h"
#include "umath/ExtendedInt.h"

using usub::umath::uint128;
using usub::umath::int128;
using usub::umath::uint256;
using usub::umath::int256;

using usub::umath::Err;
using usub::umath::Numeric;
using usub::umath::Numeric128;
using usub::umath::Numeric256;
using usub::umath::Rounding;

#if defined(__SIZEOF_INT128__)
using i128w = __int128;
using u128w = unsigned __int128;

static i128w iabs(i128w v) { return v < 0 ? -v : v; }

static i128w pow10_i128(int k) {
    i128w p = 1;
    for (int i = 0; i < k; ++i) p *= 10;
    return p;
}

static std::string to_string_i128(i128w v) {
    if (v == 0) return "0";
    bool neg = v < 0;
    u128w x = static_cast<u128w>(neg ? -v : v);
    std::string s;
    while (x != 0) {
        unsigned digit = static_cast<unsigned>(x % 10);
        s.push_back(static_cast<char>('0' + digit));
        x /= 10;
    }
    if (neg) s.push_back('-');
    std::reverse(s.begin(), s.end());
    return s;
}

static std::string to_fixed_string_i128(i128w scaled, int scale) {
    if (scaled == 0) {
        if (scale == 0) return "0";
        std::string z = "0.";
        z.append(static_cast<std::size_t>(scale), '0');
        return z;
    }

    bool neg = scaled < 0;
    u128w mag = static_cast<u128w>(neg ? -scaled : scaled);

    std::string digits;
    while (mag != 0) {
        unsigned d = static_cast<unsigned>(mag % 10);
        digits.push_back(static_cast<char>('0' + d));
        mag /= 10;
    }
    std::reverse(digits.begin(), digits.end());

    if (scale == 0) return neg ? ("-" + digits) : digits;

    if (static_cast<int>(digits.size()) <= scale) {
        std::string res;
        if (neg) res.push_back('-');
        res += "0.";
        res.append(static_cast<std::size_t>(scale - static_cast<int>(digits.size())), '0');
        res += digits;
        return res;
    }

    std::size_t p = digits.size() - static_cast<std::size_t>(scale);
    std::string res;
    if (neg) res.push_back('-');
    res.append(digits.begin(), digits.begin() + static_cast<std::ptrdiff_t>(p));
    res.push_back('.');
    res.append(digits.begin() + static_cast<std::ptrdiff_t>(p), digits.end());
    return res;
}

static i128w div_round_halfup(i128w num, i128w div) {
    if (div <= 0) std::abort();
    bool neg = (num < 0);
    i128w a = neg ? -num : num;
    i128w q = a / div;
    i128w r = a % div;
    if (r * 2 >= div) q += 1;
    return neg ? -q : q;
}
#endif

TEST(Numeric128, ParseToStringBasics) {
    using N = Numeric128<38, 4>;

    N a("0");
    EXPECT_TRUE(a.ok());
    EXPECT_EQ(a.to_string(), "0.0000");

    N b("12.34");
    EXPECT_TRUE(b.ok());
    EXPECT_EQ(b.to_string(), "12.3400");

    N c("-12.34009", Rounding::Trunc);
    EXPECT_TRUE(c.ok());
    EXPECT_EQ(c.to_string(), "-12.3400");

    N d("-12.34005", Rounding::HalfUp);
    EXPECT_TRUE(d.ok());
    EXPECT_EQ(d.to_string(), "-12.3401");

    N e(".5");
    EXPECT_TRUE(e.ok());
    EXPECT_EQ(e.to_string(), "0.5000");

    N f("1.");
    EXPECT_TRUE(f.ok());
    EXPECT_EQ(f.to_string(), "1.0000");
}

TEST(Numeric128, InvalidAndDivByZero) {
    using N = Numeric128<20, 2>;

    N bad("12a.3");
    EXPECT_FALSE(bad.ok());
    EXPECT_EQ(bad.error(), Err::Invalid);

    N z("0");
    N one("1");
    auto q = one / z;
    EXPECT_FALSE(q.ok());
    EXPECT_EQ(q.error(), Err::DivByZero);

    EXPECT_FALSE((bad == one));
}

TEST(Numeric128, OverflowByPrecision) {
    using N = Numeric128<6, 0>;

    N ok("999999");
    EXPECT_TRUE(ok.ok());

    N ov("1000000");
    EXPECT_FALSE(ov.ok());
    EXPECT_EQ(ov.error(), Err::Overflow);

    N ov2(std::numeric_limits<std::int64_t>::max());
    EXPECT_FALSE(ov2.ok());
    EXPECT_EQ(ov2.error(), Err::Overflow);
}

TEST(Numeric128, AddSubMulDivSamples) {
    using N = Numeric128<38, 4>;

    N a("1.2300");
    N b("2.5000");

    auto s = a + b;
    EXPECT_TRUE(s.ok());
    EXPECT_EQ(s.to_string(), "3.7300");

    auto d = b - a;
    EXPECT_TRUE(d.ok());
    EXPECT_EQ(d.to_string(), "1.2700");

    auto m = a * b;
    EXPECT_TRUE(m.ok());
    EXPECT_EQ(m.to_string(), "3.0750");

    auto q = b / a;
    EXPECT_TRUE(q.ok());
    EXPECT_EQ(q.to_string(), "2.0325");
}

TEST(Numeric128, IntegralOperatorsAndFloatConversion) {
    using N = Numeric128<38, 4>;

    N a("10.0000");
    auto b = a + 2;
    EXPECT_TRUE(b.ok());
    EXPECT_EQ(b.to_string(), "12.0000");

    auto c = 3 + a;
    EXPECT_TRUE(c.ok());
    EXPECT_EQ(c.to_string(), "13.0000");

    auto d = a / 2;
    EXPECT_TRUE(d.ok());
    EXPECT_EQ(d.to_string(), "5.0000");

    long double x = static_cast<long double>(a);
    EXPECT_NEAR(static_cast<double>(x), 10.0, 1e-12);

    auto y = a / 1.0;
    (void) y;
    EXPECT_NEAR(static_cast<double>(y), 10.0, 1e-12);
}

#if defined(__SIZEOF_INT128__)
TEST(Numeric128, RandomAgainstBuiltinScaled) {
    using N = Numeric128<38, 6>;
    constexpr int S = 6;

    std::mt19937_64 rng(123456);
    std::uniform_int_distribution<std::int64_t> dist(-1'000'000'000LL, 1'000'000'000LL);

    for (int i = 0; i < 20000; ++i) {
        std::int64_t ai = dist(rng);
        std::int64_t bi = dist(rng);
        if (bi == 0) bi = 1;

        N a(ai);
        N b(bi);

        ASSERT_TRUE(a.ok());
        ASSERT_TRUE(b.ok());

        i128w A = static_cast<i128w>(ai) * pow10_i128(S);
        i128w B = static_cast<i128w>(bi) * pow10_i128(S);

        auto add = a + b;
        ASSERT_TRUE(add.ok());

        auto sub = a - b;
        ASSERT_TRUE(sub.ok());

        i128w prod = (A * B);
        i128w qmul = div_round_halfup(prod, pow10_i128(S));
        auto mul = N::mul(a, b, Rounding::HalfUp);
        ASSERT_TRUE(mul.ok());
        EXPECT_EQ(mul.to_string(), to_fixed_string_i128(qmul, S));

        i128w num = A * pow10_i128(S);
        i128w qdiv = div_round_halfup(num, iabs(B));
        if (B < 0) qdiv = -qdiv;
        auto div = N::div(a, b, Rounding::HalfUp);
        ASSERT_TRUE(div.ok());
        EXPECT_EQ(div.to_string(), to_fixed_string_i128(qdiv, S));
    }
}
#endif

TEST(Numeric256, ParseToStringBasics) {
    using N = Numeric256<76, 4>;

    N a("0");
    EXPECT_TRUE(a.ok());
    EXPECT_EQ(a.to_string(), "0.0000");

    N b("12.34");
    EXPECT_TRUE(b.ok());
    EXPECT_EQ(b.to_string(), "12.3400");

    N c("-12.34009", Rounding::Trunc);
    EXPECT_TRUE(c.ok());
    EXPECT_EQ(c.to_string(), "-12.3400");

    N d("-12.34005", Rounding::HalfUp);
    EXPECT_TRUE(d.ok());
    EXPECT_EQ(d.to_string(), "-12.3401");

    N e(".5");
    EXPECT_TRUE(e.ok());
    EXPECT_EQ(e.to_string(), "0.5000");

    N f("1.");
    EXPECT_TRUE(f.ok());
    EXPECT_EQ(f.to_string(), "1.0000");

    N g("-0");
    EXPECT_TRUE(g.ok());
    EXPECT_EQ(g.to_string(), "0.0000");
}

TEST(Numeric256, InvalidAndDivByZero) {
    using N = Numeric256<76, 2>;

    N bad("12a.3");
    EXPECT_FALSE(bad.ok());
    EXPECT_EQ(bad.error(), Err::Invalid);

    N z("0");
    N one("1");
    auto q = one / z;
    EXPECT_FALSE(q.ok());
    EXPECT_EQ(q.error(), Err::DivByZero);

    EXPECT_FALSE((bad == one));
}

TEST(Numeric256, Int64MinConstructor) {
    using N = Numeric256<76, 0>;

    const std::int64_t v = (std::numeric_limits<std::int64_t>::min)();
    N a(v);
    ASSERT_TRUE(a.ok());
    EXPECT_EQ(a.to_string(), std::to_string(v));

    using N4 = Numeric256<76, 4>;
    N4 b(v);
    ASSERT_TRUE(b.ok());
    EXPECT_EQ(b.to_string(), std::to_string(v) + ".0000");
}

TEST(Numeric256, AddSubMulDivSamples) {
    using N = Numeric256<76, 4>;

    N a("1.2300");
    N b("2.5000");

    auto s = a + b;
    EXPECT_TRUE(s.ok());
    EXPECT_EQ(s.to_string(), "3.7300");

    auto d = b - a;
    EXPECT_TRUE(d.ok());
    EXPECT_EQ(d.to_string(), "1.2700");

    auto m = a * b;
    EXPECT_TRUE(m.ok());
    EXPECT_EQ(m.to_string(), "3.0750");

    auto q = b / a;
    EXPECT_TRUE(q.ok());
    EXPECT_EQ(q.to_string(), "2.0325");
}

TEST(Numeric256, IntegralOperators) {
    using N = Numeric256<76, 4>;

    N a("10.0000");
    auto b = a + 2;
    EXPECT_TRUE(b.ok());
    EXPECT_EQ(b.to_string(), "12.0000");

    auto c = 3 + a;
    EXPECT_TRUE(c.ok());
    EXPECT_EQ(c.to_string(), "13.0000");

    auto d = a / 2;
    EXPECT_TRUE(d.ok());
    EXPECT_EQ(d.to_string(), "5.0000");
}

#if defined(__SIZEOF_INT128__)
TEST(Numeric256, RandomAgainstI128Scaled_SmallRange) {
    using N = Numeric256<38, 6>;
    constexpr int S = 6;

    std::mt19937_64 rng(222333);
    std::uniform_int_distribution<std::int64_t> dist(-1'000'000LL, 1'000'000LL);

    for (int i = 0; i < 25000; ++i) {
        std::int64_t ai = dist(rng);
        std::int64_t bi = dist(rng);
        if (bi == 0) bi = 1;

        N a(ai);
        N b(bi);
        ASSERT_TRUE(a.ok());
        ASSERT_TRUE(b.ok());

        i128w A = static_cast<i128w>(ai) * pow10_i128(S);
        i128w B = static_cast<i128w>(bi) * pow10_i128(S);

        auto add = a + b;
        ASSERT_TRUE(add.ok());
        EXPECT_EQ(add.to_string(), to_fixed_string_i128(A + B, S));

        auto sub = a - b;
        ASSERT_TRUE(sub.ok());
        EXPECT_EQ(sub.to_string(), to_fixed_string_i128(A - B, S));

        i128w prod = A * B;
        i128w qmul = div_round_halfup(prod, pow10_i128(S));
        auto mul = N::mul(a, b, Rounding::HalfUp);
        ASSERT_TRUE(mul.ok());
        EXPECT_EQ(mul.to_string(), to_fixed_string_i128(qmul, S));

        i128w num = A * pow10_i128(S);
        i128w qdiv = div_round_halfup(num, iabs(B));
        if (B < 0) qdiv = -qdiv;

        auto div = N::div(a, b, Rounding::HalfUp);
        ASSERT_TRUE(div.ok());
        EXPECT_EQ(div.to_string(), to_fixed_string_i128(qdiv, S));
    }
}
#endif

TEST(Numeric, ParseToStringBasics) {
    Numeric a("0");
    EXPECT_TRUE(a.ok());
    EXPECT_EQ(a.to_string(), "0");

    Numeric b("12.34");
    EXPECT_TRUE(b.ok());
    EXPECT_EQ(b.to_string(), "12.34");

    Numeric c(".5");
    EXPECT_TRUE(c.ok());
    EXPECT_EQ(c.to_string(), "0.5");

    Numeric d("1.");
    EXPECT_TRUE(d.ok());
    EXPECT_EQ(d.to_string(), "1");

    Numeric e("-0.000");
    EXPECT_TRUE(e.ok());
    EXPECT_EQ(e.to_string(), "0");
    EXPECT_FALSE(e.negative());
    EXPECT_EQ(e.scale(), 0);
}

TEST(Numeric, InvalidAndDivByZero) {
    Numeric bad("1..2");
    EXPECT_FALSE(bad.ok());
    EXPECT_EQ(bad.error(), Err::Invalid);

    Numeric z("0");
    Numeric one("1");
    auto q = one / z;
    EXPECT_FALSE(q.ok());
    EXPECT_EQ(q.error(), Err::DivByZero);

    EXPECT_FALSE((bad == one));
}

TEST(Numeric, ScaleLimitOverflow) {
    std::string s = "0.";
    s.append(static_cast<std::size_t>(Numeric::max_frac_digits), '0');
    s.push_back('1');
    Numeric ov(s);
    EXPECT_FALSE(ov.ok());
    EXPECT_EQ(ov.error(), Err::Overflow);
}

TEST(Numeric, RescaleTruncAndHalfUp) {
    Numeric a("1.2345");
    ASSERT_TRUE(a.ok());
    EXPECT_EQ(a.scale(), 4);

    Numeric t = a;
    ASSERT_TRUE(t.rescale(2, Rounding::Trunc));
    EXPECT_EQ(t.to_string(), "1.23");
    EXPECT_EQ(t.scale(), 2);

    Numeric h("1.2350");
    ASSERT_TRUE(h.ok());
    ASSERT_TRUE(h.rescale(2, Rounding::HalfUp));
    EXPECT_EQ(h.to_string(), "1.24");

    Numeric n("-1.2350");
    ASSERT_TRUE(n.ok());
    ASSERT_TRUE(n.rescale(2, Rounding::HalfUp));
    EXPECT_EQ(n.to_string(), "-1.24");
}

TEST(Numeric, AddSubMulDivSamples) {
    Numeric a("1.23");
    Numeric b("2.5");

    auto s = a + b;
    EXPECT_TRUE(s.ok());
    EXPECT_EQ(s.to_string(), "3.73");

    auto d = b - a;
    EXPECT_TRUE(d.ok());
    EXPECT_EQ(d.to_string(), "1.27");

    auto m = Numeric::mul(a, b, 2, Rounding::HalfUp);
    EXPECT_TRUE(m.ok());
    EXPECT_EQ(m.to_string(), "3.08");

    auto q = Numeric::div(a, b, 4, Rounding::Trunc);
    EXPECT_TRUE(q.ok());
    EXPECT_EQ(q.to_string(), "0.4920");
}

TEST(Numeric, IntegralOperatorsAndFloatConversion) {
    Numeric a("10.25");
    ASSERT_TRUE(a.ok());

    auto b = a + 2;
    EXPECT_TRUE(b.ok());
    EXPECT_EQ(b.to_string(), "12.25");

    auto c = 3 + a;
    EXPECT_TRUE(c.ok());
    EXPECT_EQ(c.to_string(), "13.25");

    auto d = a / 2;
    EXPECT_TRUE(d.ok());
    EXPECT_EQ(d.to_string(), "5.13");

    long double x = static_cast<long double>(a);
    EXPECT_NEAR(static_cast<double>(x), 10.25, 1e-12);

    auto y = a / 1.0;
    (void) y;
    EXPECT_NEAR(static_cast<double>(y), 10.25, 1e-12);
}

#if defined(__SIZEOF_INT128__)
TEST(Numeric, RandomAgainstI128ReferenceSmallScales) {
    std::mt19937_64 rng(98765);
    std::uniform_int_distribution<int> scale_dist(0, 6);
    std::uniform_int_distribution<std::int64_t> val_dist(-1'000'000'000LL, 1'000'000'000LL);

    for (int i = 0; i < 15000; ++i) {
        int sa = scale_dist(rng);
        int sb = scale_dist(rng);
        int ts = scale_dist(rng);

        std::int64_t va = val_dist(rng);
        std::int64_t vb = val_dist(rng);
        if (vb == 0) vb = 1;

        i128w A = static_cast<i128w>(va) * pow10_i128(sa);
        i128w B = static_cast<i128w>(vb) * pow10_i128(sb);

        std::string as = to_fixed_string_i128(A, sa);
        std::string bs = to_fixed_string_i128(B, sb);

        Numeric a(as);
        Numeric b(bs);

        ASSERT_TRUE(a.ok());
        ASSERT_TRUE(b.ok());

        int rs = std::max(sa, sb);
        i128w Ar = A * pow10_i128(rs - sa);
        i128w Br = B * pow10_i128(rs - sb);

        auto add = Numeric::add(a, b);
        ASSERT_TRUE(add.ok());
        EXPECT_EQ(add.to_string(), to_fixed_string_i128(Ar + Br, rs));

        auto sub = Numeric::sub(a, b);
        ASSERT_TRUE(sub.ok());
        EXPECT_EQ(sub.to_string(), to_fixed_string_i128(Ar - Br, rs));

        i128w P = A * B;
        int ps = sa + sb;

        if (ts >= ps) {
            i128w Pts = P * pow10_i128(ts - ps);

            auto mtr = Numeric::mul(a, b, ts, Rounding::Trunc);
            ASSERT_TRUE(mtr.ok());
            EXPECT_EQ(mtr.to_string(), to_fixed_string_i128(Pts, ts));

            auto mhu = Numeric::mul(a, b, ts, Rounding::HalfUp);
            ASSERT_TRUE(mhu.ok());
            EXPECT_EQ(mhu.to_string(), to_fixed_string_i128(Pts, ts));
        } else {
            i128w div = pow10_i128(ps - ts);
            i128w qtr = (P >= 0) ? (P / div) : -((-P) / div);

            auto mtr = Numeric::mul(a, b, ts, Rounding::Trunc);
            ASSERT_TRUE(mtr.ok());
            EXPECT_EQ(mtr.to_string(), to_fixed_string_i128(qtr, ts));

            i128w qhu = div_round_halfup(P, div);
            auto mhu = Numeric::mul(a, b, ts, Rounding::HalfUp);
            ASSERT_TRUE(mhu.ok());
            EXPECT_EQ(mhu.to_string(), to_fixed_string_i128(qhu, ts));
        } {
            i128w num_tr = A * pow10_i128(ts + sb);
            i128w den_tr = B * pow10_i128(sa);
            i128w qtr = num_tr / den_tr;

            auto dtr = Numeric::div(a, b, ts, Rounding::Trunc);
            ASSERT_TRUE(dtr.ok());
            const std::string exp_tr = (qtr == 0) ? "0" : to_fixed_string_i128(qtr, ts);
            EXPECT_EQ(dtr.to_string(), exp_tr);

            i128w num_ex = A * pow10_i128(ts + 1 + sb);
            i128w qex = num_ex / den_tr;
            i128w last = qex % 10;
            i128w baseq = qex / 10;
            if (last < 0) last = -last;
            if (last >= 5) baseq += (qex < 0) ? -1 : 1;

            auto dhu = Numeric::div(a, b, ts, Rounding::HalfUp);
            ASSERT_TRUE(dhu.ok());
            const std::string exp_hu = (baseq == 0) ? "0" : to_fixed_string_i128(baseq, ts);
            EXPECT_EQ(dhu.to_string(), exp_hu);
        }
    }
}

TEST(Numeric, DivHalfUp_DoesNotLoseScaleOrSign_WhenQuotientBecomesZeroThenRounded) {
    Numeric a("-1");
    Numeric b("16");

    auto x = Numeric::div(a, b, 1, Rounding::HalfUp);
    ASSERT_TRUE(x.ok());
    EXPECT_EQ(x.to_string(), "-0.1");
}

TEST(Numeric128, FromRawChecked_MinInt128_IsHandled) {
    using N = Numeric128<38, 0>;

    usub::umath::int128 v = (std::numeric_limits<usub::umath::int128>::min)();

    auto got = N::from_raw_checked(v);
    EXPECT_FALSE(got.has_value());
    EXPECT_EQ(got.error(), Err::Overflow);
}

TEST(Numeric256, RandomAgainstI128Scaled_SmallRange256) {
    using N = Numeric256<38, 6>;
    constexpr int S = 6;

    std::mt19937_64 rng(222333);
    std::uniform_int_distribution<std::int64_t> dist(-1'000'000LL, 1'000'000LL);

    for (int i = 0; i < 20000; ++i) {
        std::int64_t ai = dist(rng);
        std::int64_t bi = dist(rng);
        if (bi == 0) bi = 1;

        N a(ai);
        N b(bi);
        ASSERT_TRUE(a.ok());
        ASSERT_TRUE(b.ok());

        i128w A = static_cast<i128w>(ai) * pow10_i128(S);
        i128w B = static_cast<i128w>(bi) * pow10_i128(S);

        auto add = a + b;
        ASSERT_TRUE(add.ok());
        EXPECT_EQ(add.to_string(), to_fixed_string_i128(A + B, S));

        auto sub = a - b;
        ASSERT_TRUE(sub.ok());
        EXPECT_EQ(sub.to_string(), to_fixed_string_i128(A - B, S));

        i128w prod = A * B;
        i128w qmul = div_round_halfup(prod, pow10_i128(S));
        auto mul = N::mul(a, b, Rounding::HalfUp);
        ASSERT_TRUE(mul.ok());
        EXPECT_EQ(mul.to_string(), to_fixed_string_i128(qmul, S));

        i128w num = A * pow10_i128(S);
        i128w qdiv = div_round_halfup(num, iabs(B));
        if (B < 0) qdiv = -qdiv;

        auto div = N::div(a, b, Rounding::HalfUp);
        ASSERT_TRUE(div.ok());
        EXPECT_EQ(div.to_string(), to_fixed_string_i128(qdiv, S));
    }
}
#endif

TEST(Numeric256, ParseToStringBasics256) {
    using N = Numeric256<76, 4>;

    N a("0");
    EXPECT_TRUE(a.ok());
    EXPECT_EQ(a.to_string(), "0.0000");

    N b("12.34");
    EXPECT_TRUE(b.ok());
    EXPECT_EQ(b.to_string(), "12.3400");

    N c("-12.34009", Rounding::Trunc);
    EXPECT_TRUE(c.ok());
    EXPECT_EQ(c.to_string(), "-12.3400");

    N d("-12.34005", Rounding::HalfUp);
    EXPECT_TRUE(d.ok());
    EXPECT_EQ(d.to_string(), "-12.3401");

    N e(".5");
    EXPECT_TRUE(e.ok());
    EXPECT_EQ(e.to_string(), "0.5000");

    N f("1.");
    EXPECT_TRUE(f.ok());
    EXPECT_EQ(f.to_string(), "1.0000");

    N g("-0");
    EXPECT_TRUE(g.ok());
    EXPECT_EQ(g.to_string(), "0.0000");
}

TEST(Numeric256, InvalidAndDivByZero256) {
    using N = Numeric256<76, 2>;

    N bad("12a.3");
    EXPECT_FALSE(bad.ok());
    EXPECT_EQ(bad.error(), Err::Invalid);

    N z("0");
    N one("1");
    auto q = one / z;
    EXPECT_FALSE(q.ok());
    EXPECT_EQ(q.error(), Err::DivByZero);

    EXPECT_FALSE((bad == one));
}

TEST(Numeric256, Int64MinConstructor256) {
    using N0 = Numeric256<76, 0>;
    using N4 = Numeric256<76, 4>;

    const std::int64_t v = (std::numeric_limits<std::int64_t>::min)();

    N0 a(v);
    ASSERT_TRUE(a.ok());
    EXPECT_EQ(a.to_string(), std::to_string(v));

    N4 b(v);
    ASSERT_TRUE(b.ok());
    EXPECT_EQ(b.to_string(), std::to_string(v) + ".0000");
}

TEST(Numeric256, AddSubMulDivSamples256) {
    using N = Numeric256<76, 4>;

    N a("1.2300");
    N b("2.5000");

    auto s = a + b;
    EXPECT_TRUE(s.ok());
    EXPECT_EQ(s.to_string(), "3.7300");

    auto d = b - a;
    EXPECT_TRUE(d.ok());
    EXPECT_EQ(d.to_string(), "1.2700");

    auto m = a * b;
    EXPECT_TRUE(m.ok());
    EXPECT_EQ(m.to_string(), "3.0750");

    auto q = b / a;
    EXPECT_TRUE(q.ok());
    EXPECT_EQ(q.to_string(), "2.0325");
}

TEST(Numeric256, IntegralOperators256) {
    using N = Numeric256<76, 4>;

    N a("10.0000");

    auto b = a + 2;
    EXPECT_TRUE(b.ok());
    EXPECT_EQ(b.to_string(), "12.0000");

    auto c = 3 + a;
    EXPECT_TRUE(c.ok());
    EXPECT_EQ(c.to_string(), "13.0000");

    auto d = a / 2;
    EXPECT_TRUE(d.ok());
    EXPECT_EQ(d.to_string(), "5.0000");
}

TEST(Numeric256, MulHalfUpRoundsAwayFromZero) {
    using N = Numeric256<76, 2>;

    // 1.005 * 1.00 = 1.005 -> 1.01
    N a("1.005");
    N b("1");
    auto m = N::mul(a, b, Rounding::HalfUp);
    ASSERT_TRUE(m.ok());
    EXPECT_EQ(m.to_string(), "1.01");

    // -1.005 -> -1.01
    N c("-1.005");
    auto mn = N::mul(c, b, Rounding::HalfUp);
    ASSERT_TRUE(mn.ok());
    EXPECT_EQ(mn.to_string(), "-1.01");
}

TEST(Numeric256, DivHalfUpRoundsAwayFromZero) {
    using N = Numeric256<76, 1>;

    N a("1");
    N b("16");
    auto x = N::div(a, b, Rounding::HalfUp);
    ASSERT_TRUE(x.ok());
    EXPECT_EQ(x.to_string(), "0.1");

    N na("-1");
    auto y = N::div(na, b, Rounding::HalfUp);
    ASSERT_TRUE(y.ok());
    EXPECT_EQ(y.to_string(), "-0.1");
}
