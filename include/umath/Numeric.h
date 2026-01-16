//
// Created by kirill on 1/12/26.
//

#ifndef UNUMBER_NUMERIC_FIXED_H
#define UNUMBER_NUMERIC_FIXED_H

#include <ranges>
#include <algorithm>
#include <bit>
#include <cstdint>
#include <expected>
#include <limits>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>
#include <concepts>
#include <type_traits>

#include "ExtendedInt.h"

namespace usub::umath {
    using usub::umath::uint128;
    using usub::umath::int128;

    enum class Err : std::uint8_t {
        None,
        Invalid,
        Overflow,
        DivByZero,
    };

    enum class Rounding : std::uint8_t {
        Trunc,
        HalfUp,
    };

    namespace detail {
        constexpr uint128 pow10_u(unsigned k) {
            uint128 r{0, 1};
            for (unsigned i = 0; i < k; ++i) {
                r *= uint128{0, 10};
            }
            return r;
        }

        constexpr int128 pow10_i(unsigned k) {
            return int128(pow10_u(k));
        }

        inline uint128 abs_u(int128 v) noexcept {
            if (v.high() < 0) {
                int128 nv = -v;
                return {static_cast<std::uint64_t>(nv.high()), nv.low()};
            }
            return {static_cast<std::uint64_t>(v.high()), v.low()};
        }

        inline int msb_u(uint128 v) noexcept {
            if (v.high() != 0) {
                return 64 + (63 - std::countl_zero(v.high()));
            }
            if (v.low() != 0) {
                return 63 - std::countl_zero(v.low());
            }
            return -1;
        }

        inline bool fits_precision(int128 raw, int P) noexcept {
            if (P <= 0) return false;
            if (raw.high() == 0 && raw.low() == 0) return true;

            uint128 a = abs_u(raw);
            uint128 lim = pow10_u(static_cast<unsigned>(P));
            return a < lim;
        }

        inline bool safe_mul_128(int128 a, int128 b) noexcept {
            uint128 ua = abs_u(a);
            uint128 ub = abs_u(b);
            int ma = msb_u(ua);
            int mb = msb_u(ub);
            if (ma < 0 || mb < 0) return true;
            return (ma + mb) < 127;
        }

        inline uint128 div_u(uint128 num, uint128 den, uint128 &rem) {
            rem = num % den;
            return num / den;
        }

        inline int128 apply_sign(uint128 mag, bool neg) {
            int128 r(mag);
            return neg ? -r : r;
        }

        inline int msb_u256(uint256 v) noexcept {
            const uint128 hi = v.high();
            if (hi.high() != 0) return 192 + (63 - std::countl_zero(hi.high()));
            if (hi.low() != 0) return 128 + (63 - std::countl_zero(hi.low()));
            const uint128 lo = v.low();
            if (lo.high() != 0) return 64 + (63 - std::countl_zero(lo.high()));
            if (lo.low() != 0) return (63 - std::countl_zero(lo.low()));
            return -1;
        }

        constexpr uint256 pow10_u256(unsigned k) {
            uint256 r{0U};
            r = uint256{1U};
            for (unsigned i = 0; i < k; ++i) r *= uint256{10U};
            return r;
        }

        inline uint256 abs_u256(int256 v) noexcept {
            const uint256 u(v.high(), v.low());
            if (!v.is_negative()) return u;
            return uint256{0U} - u;
        }

        inline bool fits_precision_i256(int256 raw, int P) noexcept {
            if (P <= 0) return false;
            if (raw.high() == uint128{0, 0} && raw.low() == uint128{0, 0}) return true;
            const uint256 a = abs_u256(raw);
            const uint256 lim = pow10_u256(static_cast<unsigned>(P));
            return a < lim;
        }

        inline bool safe_mul_256(uint256 a, uint256 b) noexcept {
            const int ma = msb_u256(a);
            const int mb = msb_u256(b);
            if (ma < 0 || mb < 0) return true;
            return (ma + mb) < 256;
        }

        inline uint256 div_u256(uint256 num, uint256 den, uint256 &rem) noexcept {
            rem = num % den;
            return num / den;
        }

        inline int256 apply_sign_u256(uint256 mag, bool neg) noexcept {
            int256 r(mag.high(), mag.low());
            return neg ? -r : r;
        }

        inline bool safe_mul_i256(int256 a, int256 b) noexcept {
            const uint256 ua = abs_u256(a);
            const uint256 ub = abs_u256(b);
            const int ma = msb_u256(ua);
            const int mb = msb_u256(ub);
            if (ma < 0 || mb < 0) return true;
            return (ma + mb) < 255;
        }
    } // namespace detail

    template<int P, int S>
    class Numeric128 {
    public:
        static_assert(P >= 1);
        static_assert(S >= 0);
        static_assert(S <= P);
        static_assert(P <= 38);

        using self = Numeric128<P, S>;
        using checked_t = std::expected<self, Err>;

        static constexpr int precision = P;
        static constexpr int scale = S;

        constexpr Numeric128() noexcept = default;

        explicit Numeric128(std::int64_t v) noexcept { init_from_int64(v); }
        explicit Numeric128(std::string_view s, Rounding r = Rounding::HalfUp) noexcept { init_parse(s, r); }

        [[nodiscard]] bool ok() const noexcept { return err_ == Err::None; }
        [[nodiscard]] Err error() const noexcept { return err_; }
        explicit operator bool() const noexcept { return ok(); }

        [[nodiscard]] checked_t checked() const noexcept {
            if (!ok()) return std::unexpected(err_);
            return *this;
        }

        static checked_t from_raw_checked(int128 r) noexcept {
            self out;
            out.init_from_raw(r);
            return out.checked();
        }

        static checked_t from_int64_checked(std::int64_t v) noexcept {
            self out;
            out.init_from_int64(v);
            return out.checked();
        }

        static checked_t parse_checked(std::string_view s, Rounding r = Rounding::HalfUp) noexcept {
            self out;
            out.init_parse(s, r);
            return out.checked();
        }

        [[nodiscard]] int128 raw() const noexcept { return raw_; }

        [[nodiscard]] std::string to_string() const {
            if (!ok()) {
                return "<err>";
            }

            bool neg = raw_.high() < 0;
            uint128 mag = detail::abs_u(raw_);

            uint128 div = detail::pow10_u(static_cast<unsigned>(S));
            uint128 int_mag = mag / div;
            uint128 frac_mag = mag % div;

            std::string is = uint128(int_mag).to_string();
            if constexpr (S == 0) {
                return neg ? ("-" + is) : is;
            }

            std::string fs = uint128(frac_mag).to_string();
            if (fs.size() < static_cast<std::size_t>(S)) {
                fs.insert(fs.begin(), static_cast<std::size_t>(S) - fs.size(), '0');
            }

            std::string out;
            out.reserve(is.size() + 1 + fs.size() + (neg ? 1 : 0));
            if (neg) out.push_back('-');
            out += is;
            out.push_back('.');
            out += fs;
            return out;
        }

        friend inline std::ostream &operator<<(std::ostream &os, const self &v) {
            return os << v.to_string();
        }

        friend bool operator==(const self &a, const self &b) noexcept {
            if (!a.ok() || !b.ok()) return false;
            return a.raw_ == b.raw_;
        }

        friend auto operator<=>(const self &a, const self &b) noexcept {
            return a.raw_ <=> b.raw_;
        }

        friend self operator+(self v) noexcept { return v; }

        friend self operator-(self v) noexcept {
            if (!v.ok()) return v;
            self out;
            out.init_from_raw(-v.raw_);
            return out;
        }

        friend self operator+(const self &a, const self &b) noexcept { return add(a, b); }
        friend self operator-(const self &a, const self &b) noexcept { return sub(a, b); }
        friend self operator*(const self &a, const self &b) noexcept { return mul(a, b, Rounding::HalfUp); }
        friend self operator/(const self &a, const self &b) noexcept { return div(a, b, Rounding::HalfUp); }

        friend self operator+(const self &a, std::int64_t b) noexcept { return a + self(b); }
        friend self operator-(const self &a, std::int64_t b) noexcept { return a - self(b); }
        friend self operator*(const self &a, std::int64_t b) noexcept { return a * self(b); }
        friend self operator/(const self &a, std::int64_t b) noexcept { return a / self(b); }

        friend self operator+(std::int64_t a, const self &b) noexcept { return self(a) + b; }
        friend self operator-(std::int64_t a, const self &b) noexcept { return self(a) - b; }
        friend self operator*(std::int64_t a, const self &b) noexcept { return self(a) * b; }

        self &operator+=(const self &b) noexcept {
            *this = *this + b;
            return *this;
        }

        self &operator-=(const self &b) noexcept {
            *this = *this - b;
            return *this;
        }

        self &operator*=(const self &b) noexcept {
            *this = *this * b;
            return *this;
        }

        self &operator/=(const self &b) noexcept {
            *this = *this / b;
            return *this;
        }

        self &operator+=(std::int64_t b) noexcept { return (*this += self(b)); }
        self &operator-=(std::int64_t b) noexcept { return (*this -= self(b)); }
        self &operator*=(std::int64_t b) noexcept { return (*this *= self(b)); }
        self &operator/=(std::int64_t b) noexcept { return (*this /= self(b)); }

        template<std::integral I>
            requires (!std::same_as<std::remove_cvref_t<I>, bool>)
        friend self operator+(const self &a, I b) noexcept { return a + from_integral_(b); }

        template<std::integral I>
            requires (!std::same_as<std::remove_cvref_t<I>, bool>)
        friend self operator-(const self &a, I b) noexcept { return a - from_integral_(b); }

        template<std::integral I>
            requires (!std::same_as<std::remove_cvref_t<I>, bool>)
        friend self operator*(const self &a, I b) noexcept { return a * from_integral_(b); }

        template<std::integral I>
            requires (!std::same_as<std::remove_cvref_t<I>, bool>)
        friend self operator/(const self &a, I b) noexcept { return a / from_integral_(b); }

        template<std::integral I>
            requires (!std::same_as<std::remove_cvref_t<I>, bool>)
        friend self operator+(I a, const self &b) noexcept { return from_integral_(a) + b; }

        template<std::integral I>
            requires (!std::same_as<std::remove_cvref_t<I>, bool>)
        friend self operator-(I a, const self &b) noexcept { return from_integral_(a) - b; }

        template<std::integral I>
            requires (!std::same_as<std::remove_cvref_t<I>, bool>)
        friend self operator*(I a, const self &b) noexcept { return from_integral_(a) * b; }

        template<std::integral I>
            requires (!std::same_as<std::remove_cvref_t<I>, bool>)
        friend self operator/(I a, const self &b) noexcept { return from_integral_(a) / b; }

        template<std::integral I>
            requires (!std::same_as<std::remove_cvref_t<I>, bool>)
        self &operator+=(I b) noexcept { return (*this += from_integral_(b)); }

        template<std::integral I>
            requires (!std::same_as<std::remove_cvref_t<I>, bool>)
        self &operator-=(I b) noexcept { return (*this -= from_integral_(b)); }

        template<std::integral I>
            requires (!std::same_as<std::remove_cvref_t<I>, bool>)
        self &operator*=(I b) noexcept { return (*this *= from_integral_(b)); }

        template<std::integral I>
            requires (!std::same_as<std::remove_cvref_t<I>, bool>)
        self &operator/=(I b) noexcept { return (*this /= from_integral_(b)); }

        static self mul(const self &a, const self &b, Rounding rnd) noexcept {
            if (!a.ok()) return a;
            if (!b.ok()) return b;

            self out;

            if (!detail::safe_mul_128(a.raw_, b.raw_)) {
                out.err_ = Err::Overflow;
                return out;
            }

            int128 prod = a.raw_ * b.raw_;
            int128 divv = detail::pow10_i(static_cast<unsigned>(S));

            int128 q = prod / divv;
            int128 r = prod % divv;

            if (rnd == Rounding::HalfUp) {
                uint128 ar = detail::abs_u(r);
                uint128 ad = detail::abs_u(divv);
                uint128 two_ar = ar + ar;
                if (two_ar >= ad) {
                    q += (prod.high() < 0 ? int128{-1} : int128{1});
                }
            }

            out.init_from_raw(q);
            return out;
        }

        static self div(const self &a, const self &b, Rounding rnd) noexcept {
            if (!a.ok()) return a;
            if (!b.ok()) return b;

            self out;

            if (b.raw_ == int128{0}) {
                out.err_ = Err::DivByZero;
                return out;
            }

            int128 mult = detail::pow10_i(static_cast<unsigned>(S));
            if (!detail::safe_mul_128(a.raw_, mult)) {
                out.err_ = Err::Overflow;
                return out;
            }

            int128 num = a.raw_ * mult;
            int128 q = num / b.raw_;
            int128 r = num % b.raw_;

            if (rnd == Rounding::HalfUp) {
                uint128 ar = detail::abs_u(r);
                uint128 ab = detail::abs_u(b.raw_);
                uint128 two_ar = ar + ar;
                if (two_ar >= ab) {
                    bool neg = (num.high() < 0) ^ (b.raw_.high() < 0);
                    q += neg ? int128{-1} : int128{1};
                }
            }

            out.init_from_raw(q);
            return out;
        }

        static self add(const self &a, const self &b) noexcept {
            if (!a.ok()) return a;
            if (!b.ok()) return b;

            self out;
            out.init_from_raw(a.raw_ + b.raw_);
            return out;
        }

        static self sub(const self &a, const self &b) noexcept {
            if (!a.ok()) return a;
            if (!b.ok()) return b;

            self out;
            out.init_from_raw(a.raw_ - b.raw_);
            return out;
        }

        [[nodiscard]] long double to_long_double() const noexcept {
            if (!ok()) return 0.0L;

            bool neg = raw_.high() < 0;
            uint128 mag = detail::abs_u(raw_);

            auto v = static_cast<long double>(mag.high());
            v = v * 18446744073709551616.0L + static_cast<long double>(mag.low());

            int s = S;
            while (s > 0) {
                const int step = (s > 18) ? 18 : s;
                v /= pow10_ld_(step);
                s -= step;
            }

            return neg ? -v : v;
        }

        operator long double() const noexcept {
            return to_long_double();
        }

        template<std::floating_point F>
        friend long double operator/(const self &a, F b) noexcept {
            return static_cast<long double>(a) / static_cast<long double>(b);
        }

        template<std::floating_point F>
        friend long double operator/(F a, const self &b) noexcept {
            return static_cast<long double>(a) / static_cast<long double>(b);
        }

    private:
        int128 raw_{};
        Err err_{Err::None};

        void init_error(Err e) noexcept {
            raw_ = int128{0};
            err_ = e;
        }

        void init_from_raw(int128 r) noexcept {
            if (!detail::fits_precision(r, P)) {
                init_error(Err::Overflow);
                return;
            }
            raw_ = r;
            err_ = Err::None;
        }

        void init_from_int64(std::int64_t v) noexcept {
            int128 r = int128(v) * detail::pow10_i(static_cast<unsigned>(S));
            init_from_raw(r);
        }

        void init_parse(std::string_view s, Rounding rnd) noexcept {
            if (s.empty()) {
                init_error(Err::Invalid);
                return;
            }

            bool neg = false;
            if (s.front() == '+') s.remove_prefix(1);
            else if (s.front() == '-') {
                neg = true;
                s.remove_prefix(1);
            }
            if (s.empty()) {
                init_error(Err::Invalid);
                return;
            }

            uint128 int_part{0, 0};
            uint128 frac_part{0, 0};
            unsigned frac_len = 0;
            bool seen_dot = false;

            auto is_digit = [](char c) { return c >= '0' && c <= '9'; };

            for (char c: s) {
                if (c == '.') {
                    if (seen_dot) {
                        init_error(Err::Invalid);
                        return;
                    }
                    seen_dot = true;
                    continue;
                }
                if (!is_digit(c)) {
                    init_error(Err::Invalid);
                    return;
                }

                uint128 d{0, static_cast<std::uint64_t>(c - '0')};
                if (!seen_dot) {
                    int_part = int_part * uint128{0, 10} + d;
                } else {
                    if (frac_len < 64) {
                        frac_part = frac_part * uint128{0, 10} + d;
                        ++frac_len;
                    } else {
                        ++frac_len;
                    }
                }
            }

            uint128 raw_mag = int_part * detail::pow10_u(static_cast<unsigned>(S));

            bool round_up = false;
            if (S == 0) {
                if (frac_len > 0 && rnd == Rounding::HalfUp) {
                    if (frac_len <= 64) {
                        uint128 p = detail::pow10_u(frac_len - 1);
                        uint128 first = frac_part / p;
                        round_up = (first.low() >= 5);
                    } else {
                        init_error(Err::Invalid);
                        return;
                    }
                }
            } else {
                if (frac_len == 0) {
                } else if (frac_len <= static_cast<unsigned>(S)) {
                    uint128 mult = detail::pow10_u(static_cast<unsigned>(S) - frac_len);
                    raw_mag += frac_part * mult;
                } else {
                    unsigned cut = frac_len - static_cast<unsigned>(S);
                    if (frac_len > 64) {
                        init_error(Err::Invalid);
                        return;
                    }

                    uint128 divv = detail::pow10_u(cut);
                    uint128 q{};
                    uint128 r{};
                    q = detail::div_u(frac_part, divv, r);

                    if (rnd == Rounding::HalfUp) {
                        uint128 two_r = r + r;
                        round_up = (two_r >= divv);
                    }
                    raw_mag += q;
                }
            }

            if (round_up) raw_mag += uint128{0, 1};

            int128 raw_signed = detail::apply_sign(raw_mag, neg);
            init_from_raw(raw_signed);
        }

        template<std::integral I>
            requires (!std::same_as<std::remove_cvref_t<I>, bool>)
        static self from_integral_(I v) noexcept {
            using V = std::remove_cvref_t<I>;

            if constexpr (std::signed_integral<V>) {
                if constexpr (sizeof(V) > sizeof(std::int64_t)) {
                    if (v < static_cast<V>(std::numeric_limits<std::int64_t>::min()) ||
                        v > static_cast<V>(std::numeric_limits<std::int64_t>::max())) {
                        self out;
                        out.init_error(Err::Overflow);
                        return out;
                    }
                }
                return self(static_cast<std::int64_t>(v));
            } else {
                if constexpr (sizeof(V) > sizeof(std::int64_t)) {
                    if (v > static_cast<V>(std::numeric_limits<std::int64_t>::max())) {
                        self out;
                        out.init_error(Err::Overflow);
                        return out;
                    }
                } else {
                    if (static_cast<std::uint64_t>(v) >
                        static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
                        self out;
                        out.init_error(Err::Overflow);
                        return out;
                    }
                }
                return self(static_cast<std::int64_t>(static_cast<std::uint64_t>(v)));
            }
        }

        static long double pow10_ld_(int k) noexcept {
            switch (k) {
                case 0: return 1.0L;
                case 1: return 10.0L;
                case 2: return 100.0L;
                case 3: return 1000.0L;
                case 4: return 10000.0L;
                case 5: return 100000.0L;
                case 6: return 1000000.0L;
                case 7: return 10000000.0L;
                case 8: return 100000000.0L;
                case 9: return 1000000000.0L;
                case 10: return 10000000000.0L;
                case 11: return 100000000000.0L;
                case 12: return 1000000000000.0L;
                case 13: return 10000000000000.0L;
                case 14: return 100000000000000.0L;
                case 15: return 1000000000000000.0L;
                case 16: return 10000000000000000.0L;
                case 17: return 100000000000000000.0L;
                case 18: return 1000000000000000000.0L;
                default: return 1.0L;
            }
        }
    };

    template<int P, int S>
    class Numeric256 {
    public:
        static_assert(P >= 1);
        static_assert(S >= 0);
        static_assert(S <= P);
        static_assert(P <= 76);

        using self = Numeric256<P, S>;
        using checked_t = std::expected<self, Err>;

        static constexpr int precision = P;
        static constexpr int scale = S;

        constexpr Numeric256() noexcept = default;

        explicit Numeric256(std::int64_t v) noexcept { init_from_int64(v); }
        explicit Numeric256(std::string_view s, Rounding r = Rounding::HalfUp) noexcept { init_parse(s, r); }

        [[nodiscard]] bool ok() const noexcept { return err_ == Err::None; }
        [[nodiscard]] Err error() const noexcept { return err_; }
        explicit operator bool() const noexcept { return ok(); }

        [[nodiscard]] checked_t checked() const noexcept {
            if (!ok()) return std::unexpected(err_);
            return *this;
        }

        static checked_t from_raw_checked(int256 r) noexcept {
            self out;
            out.init_from_raw(r);
            return out.checked();
        }

        static checked_t from_int64_checked(std::int64_t v) noexcept {
            self out;
            out.init_from_int64(v);
            return out.checked();
        }

        static checked_t parse_checked(std::string_view s, Rounding r = Rounding::HalfUp) noexcept {
            self out;
            out.init_parse(s, r);
            return out.checked();
        }

        [[nodiscard]] int256 raw() const noexcept { return raw_; }

        [[nodiscard]] std::string to_string() const {
            if (!ok()) return "<err>";

            const bool neg = raw_.is_negative();
            const uint256 mag = detail::abs_u256(raw_);

            if (mag == uint256{0U}) {
                if constexpr (S == 0) return "0";
                std::string z = "0.";
                z.append(static_cast<std::size_t>(S), '0');
                return z;
            }

            const uint256 div = detail::pow10_u256(static_cast<unsigned>(S));
            const uint256 int_mag = mag / div;
            const uint256 frac_mag = mag % div;

            std::string is = int_mag.to_string();
            if constexpr (S == 0) {
                return neg ? ("-" + is) : is;
            }

            std::string fs = frac_mag.to_string();
            if (fs.size() < static_cast<std::size_t>(S)) {
                fs.insert(fs.begin(), static_cast<std::size_t>(S) - fs.size(), '0');
            }

            std::string out;
            out.reserve(is.size() + 1 + fs.size() + (neg ? 1U : 0U));
            if (neg) out.push_back('-');
            out += is;
            out.push_back('.');
            out += fs;
            return out;
        }

        friend inline std::ostream &operator<<(std::ostream &os, const self &v) {
            return os << v.to_string();
        }

        friend bool operator==(const self &a, const self &b) noexcept {
            if (!a.ok() || !b.ok()) return false;
            return a.raw_ == b.raw_;
        }

        friend auto operator<=>(const self &a, const self &b) noexcept {
            return a.raw_ <=> b.raw_;
        }

        friend self operator+(self v) noexcept { return v; }

        friend self operator-(self v) noexcept {
            if (!v.ok()) return v;
            self out;
            out.init_from_raw(-v.raw_);
            return out;
        }

        friend self operator+(const self &a, const self &b) noexcept { return add(a, b); }
        friend self operator-(const self &a, const self &b) noexcept { return sub(a, b); }
        friend self operator*(const self &a, const self &b) noexcept { return mul(a, b, Rounding::HalfUp); }
        friend self operator/(const self &a, const self &b) noexcept { return div(a, b, Rounding::HalfUp); }

        self &operator+=(const self &b) noexcept {
            *this = *this + b;
            return *this;
        }

        self &operator-=(const self &b) noexcept {
            *this = *this - b;
            return *this;
        }

        self &operator*=(const self &b) noexcept {
            *this = *this * b;
            return *this;
        }

        self &operator/=(const self &b) noexcept {
            *this = *this / b;
            return *this;
        }

        template<std::integral I>
            requires (!std::same_as<std::remove_cvref_t<I>, bool>)
        friend self operator+(const self &a, I b) noexcept { return a + from_integral_(b); }

        template<std::integral I>
            requires (!std::same_as<std::remove_cvref_t<I>, bool>)
        friend self operator-(const self &a, I b) noexcept { return a - from_integral_(b); }

        template<std::integral I>
            requires (!std::same_as<std::remove_cvref_t<I>, bool>)
        friend self operator*(const self &a, I b) noexcept { return a * from_integral_(b); }

        template<std::integral I>
            requires (!std::same_as<std::remove_cvref_t<I>, bool>)
        friend self operator/(const self &a, I b) noexcept { return a / from_integral_(b); }

        template<std::integral I>
            requires (!std::same_as<std::remove_cvref_t<I>, bool>)
        friend self operator+(I a, const self &b) noexcept { return from_integral_(a) + b; }

        template<std::integral I>
            requires (!std::same_as<std::remove_cvref_t<I>, bool>)
        friend self operator-(I a, const self &b) noexcept { return from_integral_(a) - b; }

        template<std::integral I>
            requires (!std::same_as<std::remove_cvref_t<I>, bool>)
        friend self operator*(I a, const self &b) noexcept { return from_integral_(a) * b; }

        template<std::integral I>
            requires (!std::same_as<std::remove_cvref_t<I>, bool>)
        friend self operator/(I a, const self &b) noexcept { return from_integral_(a) / b; }

        template<std::integral I>
            requires (!std::same_as<std::remove_cvref_t<I>, bool>)
        self &operator+=(I b) noexcept { return (*this += from_integral_(b)); }

        template<std::integral I>
            requires (!std::same_as<std::remove_cvref_t<I>, bool>)
        self &operator-=(I b) noexcept { return (*this -= from_integral_(b)); }

        template<std::integral I>
            requires (!std::same_as<std::remove_cvref_t<I>, bool>)
        self &operator*=(I b) noexcept { return (*this *= from_integral_(b)); }

        template<std::integral I>
            requires (!std::same_as<std::remove_cvref_t<I>, bool>)
        self &operator/=(I b) noexcept { return (*this /= from_integral_(b)); }

        static self add(const self &a, const self &b) noexcept {
            if (!a.ok()) return a;
            if (!b.ok()) return b;

            self out;

            const bool an = a.raw_.is_negative();
            const bool bn = b.raw_.is_negative();
            const uint256 ua = detail::abs_u256(a.raw_);
            const uint256 ub = detail::abs_u256(b.raw_);

            uint256 mag{0U};
            bool neg = false;

            if (ua == uint256{0U}) {
                out.init_from_raw(b.raw_);
                return out;
            }
            if (ub == uint256{0U}) {
                out.init_from_raw(a.raw_);
                return out;
            }

            if (an == bn) {
                const uint256 sum = ua + ub;
                if (sum < ua) {
                    out.init_error(Err::Overflow);
                    return out;
                }
                mag = sum;
                neg = an;
            } else {
                if (ua == ub) {
                    out.raw_ = int256(uint128{0, 0}, uint128{0, 0});
                    out.err_ = Err::None;
                    return out;
                }
                if (ua > ub) {
                    mag = ua - ub;
                    neg = an;
                } else {
                    mag = ub - ua;
                    neg = bn;
                }
            }

            out.init_from_raw(detail::apply_sign_u256(mag, neg));
            return out;
        }

        static self sub(const self &a, const self &b) noexcept {
            if (!a.ok()) return a;
            if (!b.ok()) return b;
            self nb = b;
            nb.raw_ = -nb.raw_;
            return add(a, nb);
        }

        static self mul(const self &a, const self &b, Rounding rnd) noexcept {
            if (!a.ok()) return a;
            if (!b.ok()) return b;

            self out;

            if (!detail::safe_mul_i256(a.raw_, b.raw_)) {
                out.init_error(Err::Overflow);
                return out;
            }

            const bool neg = a.raw_.is_negative() ^ b.raw_.is_negative();
            const uint256 ua = detail::abs_u256(a.raw_);
            const uint256 ub = detail::abs_u256(b.raw_);

            const uint256 prod = ua * ub;
            const uint256 divv = detail::pow10_u256(static_cast<unsigned>(S));

            uint256 r{};
            uint256 q = detail::div_u256(prod, divv, r);

            if (rnd == Rounding::HalfUp) {
                const uint256 two_r = r + r;
                if (two_r >= divv) {
                    const uint256 qq = q + uint256{1U};
                    if (qq < q) {
                        out.init_error(Err::Overflow);
                        return out;
                    }
                    q = qq;
                }
            }

            out.init_from_raw(detail::apply_sign_u256(q, neg));
            return out;
        }

        static self div(const self &a, const self &b, Rounding rnd) noexcept {
            if (!a.ok()) return a;
            if (!b.ok()) return b;

            self out;

            if (b.raw_ == int256(uint128{0, 0}, uint128{0, 0})) {
                out.init_error(Err::DivByZero);
                return out;
            }

            const bool neg = a.raw_.is_negative() ^ b.raw_.is_negative();
            const uint256 ua = detail::abs_u256(a.raw_);
            const uint256 ub = detail::abs_u256(b.raw_);

            const uint256 mult = detail::pow10_u256(static_cast<unsigned>(S));
            if (!detail::safe_mul_256(ua, mult)) {
                out.init_error(Err::Overflow);
                return out;
            }

            const uint256 num = ua * mult;
            uint256 r{};
            uint256 q = detail::div_u256(num, ub, r);

            if (rnd == Rounding::HalfUp) {
                const uint256 two_r = r + r;
                if (two_r >= ub) {
                    const uint256 qq = q + uint256{1U};
                    if (qq < q) {
                        out.init_error(Err::Overflow);
                        return out;
                    }
                    q = qq;
                }
            }

            out.init_from_raw(detail::apply_sign_u256(q, neg));
            return out;
        }

        [[nodiscard]] long double to_long_double() const noexcept {
            if (!ok()) return 0.0L;

            const bool neg = raw_.is_negative();
            const uint256 mag = detail::abs_u256(raw_);
            if (mag == uint256{0U}) return 0.0L;

            const uint128 hi = mag.high();
            const uint128 lo = mag.low();

            const long double two64 = 18446744073709551616.0L;

            long double v = 0.0L;
            v = v * two64 + static_cast<long double>(hi.high());
            v = v * two64 + static_cast<long double>(hi.low());
            v = v * two64 + static_cast<long double>(lo.high());
            v = v * two64 + static_cast<long double>(lo.low());

            int s = S;
            while (s > 0) {
                const int step = (s > 18) ? 18 : s;
                v /= pow10_ld_(step);
                s -= step;
            }

            return neg ? -v : v;
        }

        operator long double() const noexcept { return to_long_double(); }

    private:
        int256 raw_{};
        Err err_{Err::None};

        void init_error(Err e) noexcept {
            raw_ = int256(uint128{0, 0}, uint128{0, 0});
            err_ = e;
        }

        void init_from_raw(int256 r) noexcept {
            if (!detail::fits_precision_i256(r, P)) {
                init_error(Err::Overflow);
                return;
            }
            raw_ = r;
            err_ = Err::None;
        }

        void init_from_int64(std::int64_t v) noexcept {
            const bool neg = (v < 0);

            const std::uint64_t av = neg
                                         ? (static_cast<std::uint64_t>(~static_cast<std::uint64_t>(v)) + 1ULL)
                                         : static_cast<std::uint64_t>(v);

            const uint256 mult = detail::pow10_u256(static_cast<unsigned>(S));
            const uint256 uav{av};

            if (!detail::safe_mul_256(uav, mult)) {
                init_error(Err::Overflow);
                return;
            }

            const uint256 mag = uav * mult;
            init_from_raw(detail::apply_sign_u256(mag, neg));
        }

        void init_parse(std::string_view s, Rounding rnd) noexcept {
            if (s.empty()) {
                init_error(Err::Invalid);
                return;
            }

            bool neg = false;
            if (s.front() == '+') s.remove_prefix(1);
            else if (s.front() == '-') {
                neg = true;
                s.remove_prefix(1);
            }
            if (s.empty()) {
                init_error(Err::Invalid);
                return;
            }

            uint256 int_part{0U};

            uint256 frac_keep{0U};
            unsigned frac_keep_len = 0;
            unsigned frac_len_total = 0;
            bool seen_dot = false;
            bool any = false;

            auto is_digit = [](char c) { return c >= '0' && c <= '9'; };

            for (char c: s) {
                if (c == '.') {
                    if (seen_dot) {
                        init_error(Err::Invalid);
                        return;
                    }
                    seen_dot = true;
                    continue;
                }
                if (!is_digit(c)) {
                    init_error(Err::Invalid);
                    return;
                }

                any = true;
                const auto d = static_cast<std::uint32_t>(c - '0');

                if (!seen_dot) {
                    if (!detail::safe_mul_256(int_part, uint256{10U})) {
                        init_error(Err::Overflow);
                        return;
                    }
                    uint256 next = int_part * uint256{10U} + uint256{d};
                    if (next < int_part) {
                        init_error(Err::Overflow);
                        return;
                    }
                    int_part = next;
                } else {
                    ++frac_len_total;
                    if (frac_keep_len < static_cast<unsigned>(S + 1)) {
                        if (!detail::safe_mul_256(frac_keep, uint256{10U})) {
                            init_error(Err::Overflow);
                            return;
                        }
                        uint256 next = frac_keep * uint256{10U} + uint256{d};
                        if (next < frac_keep) {
                            init_error(Err::Overflow);
                            return;
                        }
                        frac_keep = next;
                        ++frac_keep_len;
                    }
                }
            }

            if (!any) {
                init_error(Err::Invalid);
                return;
            }

            const uint256 p10s = detail::pow10_u256(static_cast<unsigned>(S));
            if (!detail::safe_mul_256(int_part, p10s)) {
                init_error(Err::Overflow);
                return;
            }
            uint256 raw_mag = int_part * p10s;

            bool round_up = false;

            if constexpr (S == 0) {
                if (frac_len_total > 0 && rnd == Rounding::HalfUp) {
                    if (frac_keep_len != 0) {
                        uint256 divv = detail::pow10_u256(frac_keep_len - 1);
                        uint256 first = frac_keep / divv;
                        round_up = (static_cast<unsigned>(static_cast<std::uint64_t>(first)) >= 5U);
                    }
                }
            } else {
                if (frac_len_total == 0) {
                } else if (frac_len_total <= static_cast<unsigned>(S)) {
                    const unsigned add_zeros = static_cast<unsigned>(S) - frac_len_total;
                    const uint256 mult = detail::pow10_u256(add_zeros);
                    if (!detail::safe_mul_256(frac_keep, mult)) {
                        init_error(Err::Overflow);
                        return;
                    }
                    const uint256 add = frac_keep * mult;
                    const uint256 sum = raw_mag + add;
                    if (sum < raw_mag) {
                        init_error(Err::Overflow);
                        return;
                    }
                    raw_mag = sum;
                } else {
                    if (frac_keep_len < static_cast<unsigned>(S + 1)) {
                        init_error(Err::Invalid);
                        return;
                    }

                    uint256 rem{};
                    uint256 q = detail::div_u256(frac_keep, uint256{10U}, rem);

                    if (rnd == Rounding::HalfUp) {
                        const auto guard64 = static_cast<std::uint64_t>(rem);
                        round_up = (guard64 >= 5U);
                    }

                    const uint256 sum = raw_mag + q;
                    if (sum < raw_mag) {
                        init_error(Err::Overflow);
                        return;
                    }
                    raw_mag = sum;
                }
            }

            if (round_up) {
                const uint256 next = raw_mag + uint256{1U};
                if (next < raw_mag) {
                    init_error(Err::Overflow);
                    return;
                }
                raw_mag = next;
            }

            init_from_raw(detail::apply_sign_u256(raw_mag, neg));
        }

        template<std::integral I>
            requires (!std::same_as<std::remove_cvref_t<I>, bool>)
        static self from_integral_(I v) noexcept {
            using V = std::remove_cvref_t<I>;

            if constexpr (std::signed_integral<V>) {
                if constexpr (sizeof(V) > sizeof(std::int64_t)) {
                    if (v < static_cast<V>(std::numeric_limits<std::int64_t>::min()) ||
                        v > static_cast<V>(std::numeric_limits<std::int64_t>::max())) {
                        self out;
                        out.init_error(Err::Overflow);
                        return out;
                    }
                }
                return self(static_cast<std::int64_t>(v));
            } else {
                if constexpr (sizeof(V) > sizeof(std::int64_t)) {
                    if (v > static_cast<V>(std::numeric_limits<std::int64_t>::max())) {
                        self out;
                        out.init_error(Err::Overflow);
                        return out;
                    }
                } else {
                    if (static_cast<std::uint64_t>(v) >
                        static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
                        self out;
                        out.init_error(Err::Overflow);
                        return out;
                    }
                }
                return self(static_cast<std::int64_t>(static_cast<std::uint64_t>(v)));
            }
        }

        static long double pow10_ld_(int k) noexcept {
            switch (k) {
                case 0: return 1.0L;
                case 1: return 10.0L;
                case 2: return 100.0L;
                case 3: return 1000.0L;
                case 4: return 10000.0L;
                case 5: return 100000.0L;
                case 6: return 1000000.0L;
                case 7: return 10000000.0L;
                case 8: return 100000000.0L;
                case 9: return 1000000000.0L;
                case 10: return 10000000000.0L;
                case 11: return 100000000000.0L;
                case 12: return 1000000000000.0L;
                case 13: return 10000000000000.0L;
                case 14: return 100000000000000.0L;
                case 15: return 1000000000000000.0L;
                case 16: return 10000000000000000.0L;
                case 17: return 100000000000000000.0L;
                case 18: return 1000000000000000000.0L;
                default: return 1.0L;
            }
        }
    };

    class Numeric {
    public:
        using self = Numeric;
        using checked_t = std::expected<self, Err>;

        static constexpr int max_int_digits = 131072;
        static constexpr int max_frac_digits = 16383;

        Numeric() = default;

        explicit Numeric(std::int64_t v) noexcept { init_from_int64(v); }
        explicit Numeric(std::string_view s, Rounding rnd = Rounding::HalfUp) noexcept { init_parse(s, rnd); }

        [[nodiscard]] bool ok() const noexcept { return err_ == Err::None; }
        [[nodiscard]] Err error() const noexcept { return err_; }
        explicit operator bool() const noexcept { return ok(); }

        [[nodiscard]] int scale() const noexcept { return scale_; }
        [[nodiscard]] bool negative() const noexcept { return neg_ && !is_zero_(); }

        static checked_t parse_checked(std::string_view s, Rounding rnd = Rounding::HalfUp) noexcept {
            self out;
            out.init_parse(s, rnd);
            if (!out.ok()) return std::unexpected(out.err_);
            return out;
        }

        static checked_t from_int64_checked(std::int64_t v) noexcept {
            self out;
            out.init_from_int64(v);
            if (!out.ok()) return std::unexpected(out.err_);
            return out;
        }

        [[nodiscard]] std::string to_string() const {
            if (!ok()) return "<err>";
            if (is_zero_()) return "0";

            std::string digits = mag_to_decimal_();

            if (scale_ == 0) {
                if (neg_) digits.insert(digits.begin(), '-');
                return digits;
            }

            if (static_cast<int>(digits.size()) <= scale_) {
                std::string res;
                res.reserve((neg_ ? 1U : 0U) + 2U + static_cast<std::size_t>(scale_) - digits.size() + digits.size());
                if (neg_) res.push_back('-');
                res += "0.";
                res.append(static_cast<std::size_t>(scale_ - static_cast<int>(digits.size())), '0');
                res += digits;
                return res;
            }

            const std::size_t p = digits.size() - static_cast<std::size_t>(scale_);
            std::string res;
            res.reserve((neg_ ? 1U : 0U) + digits.size() + 1U);
            if (neg_) res.push_back('-');
            res.append(digits.begin(), digits.begin() + static_cast<std::ptrdiff_t>(p));
            res.push_back('.');
            res.append(digits.begin() + static_cast<std::ptrdiff_t>(p), digits.end());
            return res;
        }

        friend inline std::ostream &operator<<(std::ostream &os, const self &v) {
            return os << v.to_string();
        }

        friend bool operator==(const self &a, const self &b) noexcept {
            if (!a.ok() || !b.ok()) return false;
            if (a.is_zero_() && b.is_zero_()) return true;
            return a.neg_ == b.neg_ && a.scale_ == b.scale_ && a.mag_ == b.mag_;
        }

        friend self operator+(const self &a, const self &b) noexcept { return add(a, b); }
        friend self operator-(const self &a, const self &b) noexcept { return sub(a, b); }

        friend self operator*(const self &a, const self &b) noexcept {
            return mul(a, b, std::max(a.scale_, b.scale_), Rounding::HalfUp);
        }

        friend self operator/(const self &a, const self &b) noexcept {
            return div(a, b, std::max(a.scale_, b.scale_), Rounding::HalfUp);
        }

        self &operator+=(const self &b) noexcept {
            *this = *this + b;
            return *this;
        }

        self &operator-=(const self &b) noexcept {
            *this = *this - b;
            return *this;
        }

        self &operator*=(const self &b) noexcept {
            *this = *this * b;
            return *this;
        }

        self &operator/=(const self &b) noexcept {
            *this = *this / b;
            return *this;
        }

        template<std::integral I>
            requires (!std::same_as<std::remove_cvref_t<I>, bool>)
        friend self operator+(const self &a, I b) noexcept { return a + from_integral_(b); }

        template<std::integral I>
            requires (!std::same_as<std::remove_cvref_t<I>, bool>)
        friend self operator-(const self &a, I b) noexcept { return a - from_integral_(b); }

        template<std::integral I>
            requires (!std::same_as<std::remove_cvref_t<I>, bool>)
        friend self operator*(const self &a, I b) noexcept { return a * from_integral_(b); }

        template<std::integral I>
            requires (!std::same_as<std::remove_cvref_t<I>, bool>)
        friend self operator/(const self &a, I b) noexcept { return a / from_integral_(b); }

        template<std::integral I>
            requires (!std::same_as<std::remove_cvref_t<I>, bool>)
        friend self operator+(I a, const self &b) noexcept { return from_integral_(a) + b; }

        template<std::integral I>
            requires (!std::same_as<std::remove_cvref_t<I>, bool>)
        friend self operator-(I a, const self &b) noexcept { return from_integral_(a) - b; }

        template<std::integral I>
            requires (!std::same_as<std::remove_cvref_t<I>, bool>)
        friend self operator*(I a, const self &b) noexcept { return from_integral_(a) * b; }

        template<std::integral I>
            requires (!std::same_as<std::remove_cvref_t<I>, bool>)
        friend self operator/(I a, const self &b) noexcept { return from_integral_(a) / b; }

        template<std::integral I>
            requires (!std::same_as<std::remove_cvref_t<I>, bool>)
        self &operator+=(I b) noexcept { return (*this += from_integral_(b)); }

        template<std::integral I>
            requires (!std::same_as<std::remove_cvref_t<I>, bool>)
        self &operator-=(I b) noexcept { return (*this -= from_integral_(b)); }

        template<std::integral I>
            requires (!std::same_as<std::remove_cvref_t<I>, bool>)
        self &operator*=(I b) noexcept { return (*this *= from_integral_(b)); }

        template<std::integral I>
            requires (!std::same_as<std::remove_cvref_t<I>, bool>)
        self &operator/=(I b) noexcept { return (*this /= from_integral_(b)); }

        static self add(const self &a, const self &b) noexcept {
            if (!a.ok()) return a;
            if (!b.ok()) return b;

            self x = a;
            self y = b;

            const int res_scale = std::max(x.scale_, y.scale_);
            if (!x.rescale_up_(res_scale) || !y.rescale_up_(res_scale)) {
                self out;
                out.set_error_(Err::Overflow);
                return out;
            }

            self out;
            out.scale_ = res_scale;

            if (x.is_zero_()) return y;
            if (y.is_zero_()) return x;

            if (x.neg_ == y.neg_) {
                out.mag_ = add_abs_(x.mag_, y.mag_);
                out.neg_ = x.neg_;
            } else {
                const int c = cmp_abs_(x.mag_, y.mag_);
                if (c == 0) {
                    out.mag_.clear();
                    out.neg_ = false;
                    out.scale_ = 0;
                } else if (c > 0) {
                    out.mag_ = sub_abs_(x.mag_, y.mag_);
                    out.neg_ = x.neg_;
                } else {
                    out.mag_ = sub_abs_(y.mag_, x.mag_);
                    out.neg_ = y.neg_;
                }
            }

            out.normalize_();
            if (!out.check_limits_()) out.set_error_(Err::Overflow);
            return out;
        }

        static self sub(const self &a, const self &b) noexcept {
            if (!a.ok()) return a;
            if (!b.ok()) return b;
            self nb = b;
            if (!nb.is_zero_()) nb.neg_ = !nb.neg_;
            return add(a, nb);
        }

        static self mul(const self &a, const self &b, int target_scale, Rounding rnd) noexcept {
            if (!a.ok()) return a;
            if (!b.ok()) return b;

            self out;
            if (target_scale < 0 || target_scale > max_frac_digits) {
                out.set_error_(Err::Overflow);
                return out;
            }

            if (a.is_zero_() || b.is_zero_()) {
                out.mag_.clear();
                out.neg_ = false;
                out.scale_ = 0;
                out.err_ = Err::None;
                return out;
            }

            std::vector<std::uint32_t> prod = mul_abs_(a.mag_, b.mag_);
            int prod_scale = a.scale_ + b.scale_;
            bool neg = a.neg_ ^ b.neg_;

            out.mag_ = std::move(prod);
            out.neg_ = neg;
            out.scale_ = prod_scale;
            out.err_ = Err::None;
            out.normalize_();

            if (!out.rescale_(target_scale, rnd)) {
                out.set_error_(Err::Overflow);
                return out;
            }

            if (!out.check_limits_()) out.set_error_(Err::Overflow);
            return out;
        }

        static self div(const self &a, const self &b, int target_scale, Rounding rnd) noexcept {
            if (!a.ok()) return a;
            if (!b.ok()) return b;

            self out;
            if (target_scale < 0 || target_scale > max_frac_digits) {
                out.set_error_(Err::Overflow);
                return out;
            }
            if (b.is_zero_()) {
                out.set_error_(Err::DivByZero);
                return out;
            }
            if (a.is_zero_()) {
                out.mag_.clear();
                out.neg_ = false;
                out.scale_ = 0;
                out.err_ = Err::None;
                return out;
            }

            const int extra = (rnd == Rounding::HalfUp) ? 1 : 0;
            const int k = target_scale + extra + b.scale_ - a.scale_;

            std::vector<std::uint32_t> num = a.mag_;
            std::vector<std::uint32_t> den = b.mag_;

            if (k >= 0) {
                if (!mul_pow10_(num, k)) {
                    out.set_error_(Err::Overflow);
                    return out;
                }
            } else {
                if (!mul_pow10_(den, -k)) {
                    out.set_error_(Err::Overflow);
                    return out;
                }
            }

            auto qr = div_mod_abs_(num, den);
            out.mag_ = std::move(qr.first);
            out.neg_ = a.neg_ ^ b.neg_;
            out.scale_ = target_scale + extra;
            out.err_ = Err::None;

            while (!out.mag_.empty() && out.mag_.back() == 0U) out.mag_.pop_back();
            if (extra == 0) {
                if (out.mag_.empty()) {
                    out.neg_ = false;
                    out.scale_ = 0;
                }
            }

            if (extra == 1) {
                const bool neg_keep = out.neg_;

                std::uint32_t rem10 = 0;
                if (!div_small_(out.mag_, 10U, rem10)) {
                    out.set_error_(Err::Overflow);
                    return out;
                }

                while (!out.mag_.empty() && out.mag_.back() == 0U) out.mag_.pop_back();

                if (rem10 >= 5U) {
                    add_one_(out.mag_);
                    while (!out.mag_.empty() && out.mag_.back() == 0U) out.mag_.pop_back();
                }

                out.scale_ = target_scale;

                if (out.mag_.empty()) {
                    out.neg_ = false;
                    out.scale_ = 0;
                } else {
                    out.neg_ = neg_keep;
                }
            }

            if (!out.check_limits_()) out.set_error_(Err::Overflow);
            return out;
        }

        bool rescale(int new_scale, Rounding rnd) noexcept {
            if (!ok()) return false;
            if (new_scale < 0 || new_scale > max_frac_digits) return false;
            return rescale_(new_scale, rnd);
        }

        [[nodiscard]] long double to_long_double() const noexcept {
            if (!ok() || mag_.empty()) return 0.0L;

            long double v = 0.0L;
            for (std::size_t i = mag_.size(); i-- > 0;) {
                v = v * static_cast<long double>(base) + static_cast<long double>(mag_[i]);
            }

            int s = scale_;
            while (s > 0) {
                const int step = (s > 18) ? 18 : s;
                v /= pow10_ld_(step);
                s -= step;
            }

            return neg_ ? -v : v;
        }

        operator long double() const noexcept {
            return to_long_double();
        }

        template<std::floating_point F>
        friend long double operator/(const self &a, F b) noexcept {
            return static_cast<long double>(a) / static_cast<long double>(b);
        }

        template<std::floating_point F>
        friend long double operator/(F a, const self &b) noexcept {
            return static_cast<long double>(a) / static_cast<long double>(b);
        }

    private:
        static constexpr std::uint32_t base = 1'000'000'000U;
        static constexpr int base_digits = 9;

        std::vector<std::uint32_t> mag_{};
        int scale_ = 0;
        bool neg_ = false;
        Err err_ = Err::None;

        void set_error_(Err e) noexcept {
            mag_.clear();
            scale_ = 0;
            neg_ = false;
            err_ = e;
        }

        [[nodiscard]] bool is_zero_() const noexcept {
            for (std::uint32_t v: mag_) if (v != 0U) return false;
            return true;
        }

        void normalize_() noexcept {
            while (!mag_.empty() && mag_.back() == 0U) mag_.pop_back();
            if (mag_.empty()) {
                neg_ = false;
                scale_ = 0;
            }
        }

        static int dec_digits_u32_(std::uint32_t x) noexcept {
            if (x >= 100000000U) return 9;
            if (x >= 10000000U) return 8;
            if (x >= 1000000U) return 7;
            if (x >= 100000U) return 6;
            if (x >= 10000U) return 5;
            if (x >= 1000U) return 4;
            if (x >= 100U) return 3;
            if (x >= 10U) return 2;
            return 1;
        }

        [[nodiscard]] int decimal_digits_() const noexcept {
            if (mag_.empty()) return 1;
            return (static_cast<int>(mag_.size()) - 1) * base_digits + dec_digits_u32_(mag_.back());
        }

        [[nodiscard]] bool check_limits_() const noexcept {
            if (scale_ < 0 || scale_ > max_frac_digits) return false;
            if (mag_.empty()) return true;
            const int digs = decimal_digits_();
            const int before = std::max(0, digs - scale_);
            return before <= max_int_digits;
        }

        static std::uint32_t pow10_u32_(int k) noexcept {
            std::uint32_t p = 1U;
            for (int i = 0; i < k; ++i) p *= 10U;
            return p;
        }

        static void add_one_(std::vector<std::uint32_t> &v) {
            std::uint64_t carry = 1;
            for (unsigned int &i: v) {
                const std::uint64_t cur = static_cast<std::uint64_t>(i) + carry;
                i = static_cast<std::uint32_t>(cur % base);
                carry = cur / base;
                if (carry == 0) break;
            }
            if (carry != 0) v.push_back(static_cast<std::uint32_t>(carry));
        }

        static int cmp_abs_(const std::vector<std::uint32_t> &a, const std::vector<std::uint32_t> &b) noexcept {
            if (a.size() != b.size()) return (a.size() < b.size()) ? -1 : 1;
            for (std::size_t i = a.size(); i-- > 0;) {
                if (a[i] != b[i]) return (a[i] < b[i]) ? -1 : 1;
            }
            return 0;
        }

        static std::vector<std::uint32_t> add_abs_(const std::vector<std::uint32_t> &a,
                                                   const std::vector<std::uint32_t> &b) {
            const std::size_t n = std::max(a.size(), b.size());
            std::vector<std::uint32_t> r;
            r.resize(n);

            std::uint64_t carry = 0;
            for (std::size_t i = 0; i < n; ++i) {
                const std::uint64_t av = (i < a.size()) ? a[i] : 0U;
                const std::uint64_t bv = (i < b.size()) ? b[i] : 0U;
                const std::uint64_t s = av + bv + carry;
                r[i] = static_cast<std::uint32_t>(s % base);
                carry = s / base;
            }
            if (carry != 0) r.push_back(static_cast<std::uint32_t>(carry));
            while (!r.empty() && r.back() == 0U) r.pop_back();
            return r;
        }

        static std::vector<std::uint32_t> sub_abs_(const std::vector<std::uint32_t> &a,
                                                   const std::vector<std::uint32_t> &b) {
            std::vector<std::uint32_t> r = a;
            std::int64_t carry = 0;
            for (std::size_t i = 0; i < r.size(); ++i) {
                const auto av = static_cast<std::int64_t>(r[i]);
                const std::int64_t bv = (i < b.size()) ? static_cast<std::int64_t>(b[i]) : 0;
                std::int64_t cur = av - bv - carry;
                if (cur < 0) {
                    cur += static_cast<std::int64_t>(base);
                    carry = 1;
                } else {
                    carry = 0;
                }
                r[i] = static_cast<std::uint32_t>(cur);
            }
            while (!r.empty() && r.back() == 0U) r.pop_back();
            return r;
        }

        static std::vector<std::uint32_t> mul_abs_(const std::vector<std::uint32_t> &a,
                                                   const std::vector<std::uint32_t> &b) {
            if (a.empty() || b.empty()) return {};
            std::vector<std::uint64_t> tmp;
            tmp.assign(a.size() + b.size(), 0ULL);

            for (std::size_t i = 0; i < a.size(); ++i) {
                std::uint64_t carry = 0;
                for (std::size_t j = 0; j < b.size(); ++j) {
                    const std::uint64_t cur =
                            tmp[i + j] + static_cast<std::uint64_t>(a[i]) * static_cast<std::uint64_t>(b[j]) + carry;
                    tmp[i + j] = cur % base;
                    carry = cur / base;
                }
                tmp[i + b.size()] += carry;
            }

            std::vector<std::uint32_t> r;
            r.resize(tmp.size());
            for (std::size_t i = 0; i < tmp.size(); ++i) r[i] = static_cast<std::uint32_t>(tmp[i]);
            while (!r.empty() && r.back() == 0U) r.pop_back();
            return r;
        }

        static bool mul_small_(std::vector<std::uint32_t> &v, std::uint32_t m) {
            if (v.empty() || m == 1U) return true;
            if (m == 0U) {
                v.clear();
                return true;
            }

            std::uint64_t carry = 0;
            for (unsigned int &i: v) {
                const std::uint64_t cur = static_cast<std::uint64_t>(i) * m + carry;
                i = static_cast<std::uint32_t>(cur % base);
                carry = cur / base;
            }
            while (carry != 0) {
                v.push_back(static_cast<std::uint32_t>(carry % base));
                carry /= base;
            }
            return true;
        }

        static bool div_small_(std::vector<std::uint32_t> &v, std::uint32_t d, std::uint32_t &rem) {
            if (d == 0U) return false;
            std::uint64_t r = 0;
            for (std::size_t i = v.size(); i-- > 0;) {
                const std::uint64_t cur = v[i] + r * base;
                v[i] = static_cast<std::uint32_t>(cur / d);
                r = cur % d;
            }
            rem = static_cast<std::uint32_t>(r);
            while (!v.empty() && v.back() == 0U) v.pop_back();
            return true;
        }

        static bool mul_pow10_(std::vector<std::uint32_t> &v, int k) {
            if (k < 0) return false;
            if (v.empty() || k == 0) return true;

            const int limb_shift = k / base_digits;
            const int rem = k % base_digits;

            if (limb_shift > 0) {
                std::vector<std::uint32_t> out;
                out.resize(static_cast<std::size_t>(limb_shift), 0U);
                out.insert(out.end(), v.begin(), v.end());
                v.swap(out);
            }

            if (rem > 0) {
                const std::uint32_t m = pow10_u32_(rem);
                return mul_small_(v, m);
            }
            return true;
        }

        static std::pair<std::vector<std::uint32_t>, std::vector<std::uint32_t> >
        div_mod_abs_(std::vector<std::uint32_t> a, std::vector<std::uint32_t> b) {
            while (!a.empty() && a.back() == 0U) a.pop_back();
            while (!b.empty() && b.back() == 0U) b.pop_back();
            if (b.empty()) return {{}, {}};
            if (a.empty()) return {{}, {}};

            if (cmp_abs_(a, b) < 0) return {{}, a};

            const auto f = static_cast<std::uint32_t>(base / (static_cast<std::uint64_t>(b.back()) + 1ULL));
            if (f != 1U) {
                mul_small_(a, f);
                mul_small_(b, f);
            }

            std::vector<std::uint32_t> q;
            q.assign(a.size(), 0U);

            std::vector<std::uint32_t> r;

            for (std::size_t i = a.size(); i-- > 0;) {
                if (!r.empty() || a[i] != 0U) r.insert(r.begin(), a[i]);
                else r.insert(r.begin(), 0U);
                while (!r.empty() && r.back() == 0U) r.pop_back();

                std::uint64_t s1 = (r.size() > b.size()) ? r[b.size()] : 0ULL;
                std::uint64_t s0 = (r.size() > b.size() - 1) ? r[b.size() - 1] : 0ULL;

                std::uint64_t d = (s1 * base + s0) / b.back();
                if (d >= base) d = base - 1;

                std::vector<std::uint32_t> bd = b;
                if (d != 0) {
                    mul_small_(bd, static_cast<std::uint32_t>(d));
                } else {
                    bd.clear();
                }

                while (!bd.empty() && cmp_abs_(r, bd) < 0) {
                    --d;
                    bd = b;
                    mul_small_(bd, static_cast<std::uint32_t>(d));
                }

                if (!bd.empty()) r = sub_abs_(r, bd);
                q[i] = static_cast<std::uint32_t>(d);
            }

            while (!q.empty() && q.back() == 0U) q.pop_back();

            if (f != 1U && !r.empty()) {
                std::uint32_t rem = 0;
                div_small_(r, f, rem);
            }

            return {q, r};
        }

        bool rescale_up_(int new_scale) noexcept {
            if (new_scale < scale_) return false;
            if (new_scale > max_frac_digits) return false;
            const int add = new_scale - scale_;
            if (add == 0) return true;
            if (!mul_pow10_(mag_, add)) return false;
            scale_ = new_scale;
            normalize_();
            return check_limits_();
        }

        bool rescale_(int new_scale, Rounding rnd) noexcept {
            if (new_scale < 0 || new_scale > max_frac_digits) return false;
            if (new_scale == scale_) return true;

            if (new_scale > scale_) return rescale_up_(new_scale);

            const int cut = scale_ - new_scale;
            const int m = cut / base_digits;
            const int r = cut % base_digits;

            int guard = 0;
            if (rnd == Rounding::HalfUp) {
                if (r == 0) {
                    if (m > 0 && static_cast<std::size_t>(m) <= mag_.size()) {
                        const std::uint32_t limb = mag_[static_cast<std::size_t>(m - 1)];
                        guard = static_cast<int>(limb / 100000000U);
                    } else {
                        guard = 0;
                    }
                } else {
                    if (static_cast<std::size_t>(m) < mag_.size()) {
                        const std::uint32_t limb = mag_[static_cast<std::size_t>(m)];
                        const std::uint32_t p = pow10_u32_(r);
                        const std::uint32_t rem = limb % p;
                        guard = static_cast<int>((rem / pow10_u32_(r - 1)) % 10U);
                    } else {
                        guard = 0;
                    }
                }
            }

            if (m > 0) {
                if (static_cast<std::size_t>(m) >= mag_.size()) {
                    mag_.clear();
                } else {
                    mag_.erase(mag_.begin(), mag_.begin() + static_cast<std::ptrdiff_t>(m));
                }
            }

            if (r > 0 && !mag_.empty()) {
                std::uint32_t rem_small = 0;
                const std::uint32_t d = pow10_u32_(r);
                div_small_(mag_, d, rem_small);
            }

            scale_ = new_scale;

            while (!mag_.empty() && mag_.back() == 0U) mag_.pop_back();

            if (rnd == Rounding::HalfUp && guard >= 5) {
                add_one_(mag_);
                while (!mag_.empty() && mag_.back() == 0U) mag_.pop_back();
            }

            if (mag_.empty()) {
                neg_ = false;
                scale_ = 0;
            }

            return check_limits_();
        }

        void init_from_int64(std::int64_t v) noexcept {
            mag_.clear();
            scale_ = 0;
            neg_ = (v < 0);
            err_ = Err::None;

            auto x = static_cast<std::uint64_t>(neg_ ? -v : v);
            if (x == 0) {
                neg_ = false;
                return;
            }

            while (x != 0) {
                mag_.push_back(static_cast<std::uint32_t>(x % base));
                x /= base;
            }
            normalize_();
            if (!check_limits_()) set_error_(Err::Overflow);
        }

        void init_parse(std::string_view s, Rounding) noexcept {
            mag_.clear();
            scale_ = 0;
            neg_ = false;
            err_ = Err::None;

            if (s.empty()) {
                set_error_(Err::Invalid);
                return;
            }

            if (s.front() == '+') s.remove_prefix(1);
            else if (s.front() == '-') {
                neg_ = true;
                s.remove_prefix(1);
            }
            if (s.empty()) {
                set_error_(Err::Invalid);
                return;
            }

            bool seen_dot = false;
            bool any = false;

            for (char c: s) {
                if (c == '.') {
                    if (seen_dot) {
                        set_error_(Err::Invalid);
                        return;
                    }
                    seen_dot = true;
                    continue;
                }
                if (c < '0' || c > '9') {
                    set_error_(Err::Invalid);
                    return;
                }

                any = true;
                mul10_add_(static_cast<std::uint32_t>(c - '0'));
                if (seen_dot) {
                    ++scale_;
                    if (scale_ > max_frac_digits) {
                        set_error_(Err::Overflow);
                        return;
                    }
                }
            }

            if (!any) {
                set_error_(Err::Invalid);
                return;
            }

            normalize_();
            if (is_zero_()) {
                neg_ = false;
                scale_ = 0;
            }
            if (!check_limits_()) set_error_(Err::Overflow);
        }

        void mul10_add_(std::uint32_t digit) {
            std::uint64_t carry = digit;
            for (unsigned int &i: mag_) {
                const std::uint64_t cur = static_cast<std::uint64_t>(i) * 10ULL + carry;
                i = static_cast<std::uint32_t>(cur % base);
                carry = cur / base;
            }
            if (carry != 0) mag_.push_back(static_cast<std::uint32_t>(carry));
        }

        [[nodiscard]] std::string mag_to_decimal_() const {
            if (mag_.empty()) return "0";
            std::string s = std::to_string(mag_.back());
            for (std::size_t i = mag_.size() - 1; i-- > 0;) {
                std::string block = std::to_string(mag_[i]);
                if (block.size() < static_cast<std::size_t>(base_digits)) {
                    s.append(static_cast<std::size_t>(base_digits) - block.size(), '0');
                }
                s += block;
            }
            return s;
        }

        template<std::integral I>
            requires (!std::same_as<std::remove_cvref_t<I>, bool>)
        static self from_integral_(I v) noexcept {
            using V = std::remove_cvref_t<I>;

            if constexpr (std::signed_integral<V>) {
                if constexpr (sizeof(V) > sizeof(std::int64_t)) {
                    if (v < static_cast<V>(std::numeric_limits<std::int64_t>::min()) ||
                        v > static_cast<V>(std::numeric_limits<std::int64_t>::max())) {
                        self out;
                        out.set_error_(Err::Overflow);
                        return out;
                    }
                }
                return self(static_cast<std::int64_t>(v));
            } else {
                if constexpr (sizeof(V) > sizeof(std::int64_t)) {
                    if (v > static_cast<V>(std::numeric_limits<std::int64_t>::max())) {
                        self out;
                        out.set_error_(Err::Overflow);
                        return out;
                    }
                } else {
                    if (static_cast<std::uint64_t>(v) >
                        static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
                        self out;
                        out.set_error_(Err::Overflow);
                        return out;
                    }
                }
                return self(static_cast<std::int64_t>(static_cast<std::uint64_t>(v)));
            }
        }

        static long double pow10_ld_(int k) noexcept {
            switch (k) {
                case 0: return 1.0L;
                case 1: return 10.0L;
                case 2: return 100.0L;
                case 3: return 1000.0L;
                case 4: return 10000.0L;
                case 5: return 100000.0L;
                case 6: return 1000000.0L;
                case 7: return 10000000.0L;
                case 8: return 100000000.0L;
                case 9: return 1000000000.0L;
                case 10: return 10000000000.0L;
                case 11: return 100000000000.0L;
                case 12: return 1000000000000.0L;
                case 13: return 10000000000000.0L;
                case 14: return 100000000000000.0L;
                case 15: return 1000000000000000.0L;
                case 16: return 10000000000000000.0L;
                case 17: return 100000000000000000.0L;
                case 18: return 1000000000000000000.0L;
                default: return 1.0L;
            }
        }
    };
} // namespace unumber::numeric

#endif // UNUMBER_NUMERIC_FIXED_H
