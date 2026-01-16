#ifndef UNUMBER_INT128_H
#define UNUMBER_INT128_H

#include <bit>
#include <cstdint>
#include <compare>
#include <limits>
#include <ostream>
#include <string>
#include <utility>
#include <algorithm>
#include <type_traits>

namespace usub::umath {
    class uint128 {
    public:
        constexpr uint128() noexcept = default;

        constexpr uint128(uint64_t hi, uint64_t lo) noexcept
            : hi_(hi), lo_(lo) {
        }

        template<typename T,
            std::enable_if_t<std::is_integral_v<T> && std::is_unsigned_v<T>, int> = 0>
        constexpr uint128(T v) noexcept
            : hi_(0),
              lo_(static_cast<uint64_t>(v)) {
        }

        template<typename T,
            std::enable_if_t<std::is_integral_v<T> && std::is_signed_v<T>, int> = 0>
        constexpr uint128(T v) noexcept
            : hi_(v < 0 ? ~uint64_t{0} : 0),
              lo_(static_cast<uint64_t>(v)) {
        }

        [[nodiscard]] constexpr uint64_t high() const noexcept { return hi_; }
        [[nodiscard]] constexpr uint64_t low() const noexcept { return lo_; }

        explicit constexpr operator uint64_t() const noexcept { return lo_; }

        explicit constexpr operator int64_t() const noexcept {
            return static_cast<int64_t>(lo_);
        }

        [[nodiscard]] constexpr auto operator<=>(const uint128 &) const noexcept = default;

        friend constexpr uint128 operator+(uint128 a, uint128 b) noexcept {
#if defined(__SIZEOF_INT128__)
            auto wa = to_wide(a);
            auto wb = to_wide(b);
            auto wr = wa + wb;
            return from_wide(wr);
#else
            uint64_t lo = a.lo_ + b.lo_;
            const uint64_t carry = lo < a.lo_ ? 1u : 0u;
            uint64_t hi = a.hi_ + b.hi_ + carry;
            return uint128(hi, lo);
#endif
        }

        friend constexpr uint128 operator-(uint128 a, uint128 b) noexcept {
#if defined(__SIZEOF_INT128__)
            auto wa = to_wide(a);
            auto wb = to_wide(b);
            auto wr = wa - wb;
            return from_wide(wr);
#else
            const uint64_t borrow = a.lo_ < b.lo_ ? 1u : 0u;
            uint64_t lo = a.lo_ - b.lo_;
            uint64_t hi = a.hi_ - b.hi_ - borrow;
            return uint128(hi, lo);
#endif
        }

        friend inline uint128 operator*(uint128 a, uint128 b) noexcept {
#if defined(__SIZEOF_INT128__)
            auto wa = to_wide(a);
            auto wb = to_wide(b);
            auto wr = wa * wb;
            return from_wide(wr);
#else
            uint64_t a_lo = a.lo_;
            uint64_t a_hi = a.hi_;
            uint64_t b_lo = b.lo_;
            uint64_t b_hi = b.hi_;

            uint64_t lo_hi, lo_lo;
            mul_64_64_128(a_lo, b_lo, lo_hi, lo_lo);

            uint64_t cross1_hi, cross1_lo;
            mul_64_64_128(a_hi, b_lo, cross1_hi, cross1_lo);

            uint64_t cross2_hi, cross2_lo;
            mul_64_64_128(a_lo, b_hi, cross2_hi, cross2_lo);

            uint64_t hi = a_hi * b_hi;

            uint64_t tmp_lo = lo_hi;
            uint64_t carry = 0;

            tmp_lo += cross1_lo;
            if (tmp_lo < lo_hi) {
                ++carry;
            }

            tmp_lo += cross2_lo;
            if (tmp_lo < cross2_lo) {
                ++carry;
            }

            hi += cross1_hi + cross2_hi + carry;

            return uint128(hi, lo_lo);
#endif
        }

        friend inline uint128 operator/(uint128 a, uint128 b) noexcept {
#if defined(__SIZEOF_INT128__)
            auto wa = to_wide(a);
            auto wb = to_wide(b);
            auto wr = wa / wb;
            return from_wide(wr);
#else
            uint128 q{};
            uint128 r{};
            div_mod(a, b, q, r);
            return q;
#endif
        }

        friend inline uint128 operator%(uint128 a, uint128 b) noexcept {
#if defined(__SIZEOF_INT128__)
            auto wa = to_wide(a);
            auto wb = to_wide(b);
            auto wr = wa % wb;
            return from_wide(wr);
#else
            uint128 q{};
            uint128 r{};
            div_mod(a, b, q, r);
            return r;
#endif
        }

        constexpr uint128 &operator+=(uint128 other) noexcept {
            *this = *this + other;
            return *this;
        }

        constexpr uint128 &operator-=(uint128 other) noexcept {
            *this = *this - other;
            return *this;
        }

        uint128 &operator*=(uint128 other) noexcept {
            *this = *this * other;
            return *this;
        }

        uint128 &operator/=(uint128 other) noexcept {
            *this = *this / other;
            return *this;
        }

        uint128 &operator%=(uint128 other) noexcept {
            *this = *this % other;
            return *this;
        }

        constexpr uint128 &operator++() noexcept {
            *this += uint128(0, 1);
            return *this;
        }

        constexpr uint128 operator++(int) noexcept {
            uint128 tmp(*this);
            ++(*this);
            return tmp;
        }

        constexpr uint128 &operator--() noexcept {
            *this -= uint128(0, 1);
            return *this;
        }

        constexpr uint128 operator--(int) noexcept {
            uint128 tmp(*this);
            --(*this);
            return tmp;
        }

        friend constexpr uint128 operator~(uint128 x) noexcept {
            return {~x.hi_, ~x.lo_};
        }

        friend constexpr uint128 operator&(uint128 a, uint128 b) noexcept {
            return {a.hi_ & b.hi_, a.lo_ & b.lo_};
        }

        friend constexpr uint128 operator|(uint128 a, uint128 b) noexcept {
            return {a.hi_ | b.hi_, a.lo_ | b.lo_};
        }

        friend constexpr uint128 operator^(uint128 a, uint128 b) noexcept {
            return {a.hi_ ^ b.hi_, a.lo_ ^ b.lo_};
        }

        constexpr uint128 &operator&=(uint128 other) noexcept {
            hi_ &= other.hi_;
            lo_ &= other.lo_;
            return *this;
        }

        constexpr uint128 &operator|=(uint128 other) noexcept {
            hi_ |= other.hi_;
            lo_ |= other.lo_;
            return *this;
        }

        constexpr uint128 &operator^=(uint128 other) noexcept {
            hi_ ^= other.hi_;
            lo_ ^= other.lo_;
            return *this;
        }

        friend constexpr uint128 operator<<(uint128 v, int shift) noexcept {
            if (shift <= 0) {
                return v;
            }
            if (shift >= 128) {
                return {0, 0};
            }
            if (shift >= 64) {
                uint64_t hi = v.lo_ << (shift - 64);
                return {hi, 0};
            }
            uint64_t hi = (v.hi_ << shift) | (v.lo_ >> (64 - shift));
            uint64_t lo = v.lo_ << shift;
            return {hi, lo};
        }

        friend constexpr uint128 operator>>(uint128 v, int shift) noexcept {
            if (shift <= 0) {
                return v;
            }
            if (shift >= 128) {
                return {0, 0};
            }
            if (shift >= 64) {
                uint64_t lo = v.hi_ >> (shift - 64);
                return {0, lo};
            }
            uint64_t hi = v.hi_ >> shift;
            uint64_t lo = (v.lo_ >> shift) | (v.hi_ << (64 - shift));
            return {hi, lo};
        }

        constexpr uint128 &operator<<=(int shift) noexcept {
            *this = *this << shift;
            return *this;
        }

        constexpr uint128 &operator>>=(int shift) noexcept {
            *this = *this >> shift;
            return *this;
        }

        [[nodiscard]] std::string to_string() const {
            if (hi_ == 0 && lo_ == 0) {
                return "0";
            }

            uint128 tmp = *this;
            std::string res;
            res.reserve(40);

            while (tmp.hi_ != 0 || tmp.lo_ != 0) {
                uint64_t digit = (tmp % uint128(0, 10)).low();
                res.push_back(static_cast<char>('0' + digit));
                tmp /= uint128(0, 10);
            }

            std::ranges::reverse(res);
            return res;
        }

    private:
        uint64_t hi_{0};
        uint64_t lo_{0};

#if defined(__SIZEOF_INT128__)
        using wide_type = unsigned __int128;

        static constexpr wide_type to_wide(uint128 v) noexcept {
            return (static_cast<wide_type>(v.hi_) << 64) |
                   static_cast<wide_type>(v.lo_);
        }

        static constexpr uint128 from_wide(wide_type w) noexcept {
            auto lo = static_cast<uint64_t>(w);
            auto hi = static_cast<uint64_t>(w >> 64);
            return {hi, lo};
        }
#endif

        static inline void mul_64_64_128(uint64_t x, uint64_t y,
                                         uint64_t &hi, uint64_t &lo) noexcept {
            const uint64_t x0 = x & 0xffffffffu;
            const uint64_t x1 = x >> 32;
            const uint64_t y0 = y & 0xffffffffu;
            const uint64_t y1 = y >> 32;

            const uint64_t p0 = x0 * y0;
            const uint64_t p1 = x0 * y1;
            const uint64_t p2 = x1 * y0;
            const uint64_t p3 = x1 * y1;

            uint64_t mid = (p0 >> 32) + (p1 & 0xffffffffu) + (p2 & 0xffffffffu);
            hi = p3 + (p1 >> 32) + (p2 >> 32) + (mid >> 32);
            lo = (mid << 32) | (p0 & 0xffffffffu);
        }

        static inline void div_mod(uint128 dividend,
                                   uint128 divisor,
                                   uint128 &q,
                                   uint128 &r) noexcept {
            if (divisor > dividend) {
                q = uint128(0, 0);
                r = dividend;
                return;
            }
            if (divisor == dividend) {
                q = uint128(0, 1);
                r = uint128(0, 0);
                return;
            }

            int shift = msb(dividend) - msb(divisor);
            uint128 denom = divisor << shift;
            uint128 quotient(0, 0);

            while (shift >= 0) {
                quotient <<= 1;
                if (dividend >= denom) {
                    dividend -= denom;
                    quotient.lo_ |= 1u;
                }
                denom >>= 1;
                --shift;
            }

            q = quotient;
            r = dividend;
        }

        static inline int msb(uint128 v) noexcept {
            if (v.hi_ != 0) {
                return 64 + (63 - std::countl_zero(v.hi_));
            }
            return 63 - std::countl_zero(v.lo_);
        }
    };

    inline std::ostream &operator<<(std::ostream &os, const uint128 &v) {
        return os << v.to_string();
    }

    class int128 {
    public:
        constexpr int128() noexcept = default;

        constexpr int128(int v) noexcept
            : int128(static_cast<int64_t>(v)) {
        }

        constexpr int128(unsigned int v) noexcept
            : int128(static_cast<uint64_t>(v)) {
        }

        constexpr int128(int64_t v) noexcept
            : hi_(static_cast<uint64_t>(v < 0 ? -1 : 0)),
              lo_(static_cast<uint64_t>(v)) {
        }

        constexpr int128(uint64_t v) noexcept
            : hi_(0),
              lo_(v) {
        }

        constexpr int128(int64_t hi, uint64_t lo) noexcept
            : hi_(static_cast<uint64_t>(hi)),
              lo_(lo) {
        }

        explicit constexpr int128(uint128 u) noexcept
            : hi_(u.high()),
              lo_(u.low()) {
        }

        [[nodiscard]] constexpr int64_t high() const noexcept {
            return static_cast<int64_t>(hi_);
        }

        [[nodiscard]] constexpr uint64_t low() const noexcept {
            return lo_;
        }

        explicit constexpr operator int64_t() const noexcept {
            return static_cast<int64_t>(lo_);
        }

        explicit constexpr operator uint64_t() const noexcept {
            return lo_;
        }

        [[nodiscard]] constexpr auto operator<=>(const int128 &) const noexcept = default;

        friend constexpr int128 operator+(int128 a, int128 b) noexcept {
#if defined(__SIZEOF_INT128__)
            auto wa = to_wide(a);
            auto wb = to_wide(b);
            auto wr = wa + wb;
            return from_wide(wr);
#else
            uint128 ua(a.hi_, a.lo_);
            uint128 ub(b.hi_, b.lo_);
            uint128 ur = ua + ub;
            return int128(static_cast<int64_t>(ur.high()), ur.low());
#endif
        }

        friend constexpr int128 operator-(int128 a, int128 b) noexcept {
#if defined(__SIZEOF_INT128__)
            auto wa = to_wide(a);
            auto wb = to_wide(b);
            auto wr = wa - wb;
            return from_wide(wr);
#else
            uint128 ua(a.hi_, a.lo_);
            uint128 ub(b.hi_, b.lo_);
            uint128 ur = ua - ub;
            return int128(static_cast<int64_t>(ur.high()), ur.low());
#endif
        }

        friend inline int128 operator*(int128 a, int128 b) noexcept {
#if defined(__SIZEOF_INT128__)
            auto wa = to_wide(a);
            auto wb = to_wide(b);
            auto wr = wa * wb;
            return from_wide(wr);
#else
            uint128 ua(a.hi_, a.lo_);
            uint128 ub(b.hi_, b.lo_);
            uint128 ur = ua * ub;
            return int128(static_cast<int64_t>(ur.high()), ur.low());
#endif
        }

        friend inline int128 operator/(int128 a, int128 b) noexcept {
#if defined(__SIZEOF_INT128__)
            auto wa = to_wide(a);
            auto wb = to_wide(b);
            auto wr = wa / wb;
            return from_wide(wr);
#else
            bool neg = (a.high() < 0) ^ (b.high() < 0);
            uint128 ua = unsigned_abs(a);
            uint128 ub = unsigned_abs(b);
            uint128 uq{};
            uint128 ur{};
            uint128::div_mod(ua, ub, uq, ur);
            if (neg) {
                uint128 tmp = uint128(0, 0) - uq;
                return int128(static_cast<int64_t>(tmp.high()), tmp.low());
            }
            return int128(static_cast<int64_t>(uq.high()), uq.low());
#endif
        }

        friend inline int128 operator%(int128 a, int128 b) noexcept {
#if defined(__SIZEOF_INT128__)
            auto wa = to_wide(a);
            auto wb = to_wide(b);
            auto wr = wa % wb;
            return from_wide(wr);
#else
            bool neg = (a.high() < 0);
            uint128 ua = unsigned_abs(a);
            uint128 ub = unsigned_abs(b);
            uint128 uq{};
            uint128 ur{};
            uint128::div_mod(ua, ub, uq, ur);
            if (neg) {
                uint128 tmp = uint128(0, 0) - ur;
                return int128(static_cast<int64_t>(tmp.high()), tmp.low());
            }
            return int128(static_cast<int64_t>(ur.high()), ur.low());
#endif
        }

        constexpr int128 &operator+=(int128 other) noexcept {
            *this = *this + other;
            return *this;
        }

        constexpr int128 &operator-=(int128 other) noexcept {
            *this = *this - other;
            return *this;
        }

        int128 &operator*=(int128 other) noexcept {
            *this = *this * other;
            return *this;
        }

        int128 &operator/=(int128 other) noexcept {
            *this = *this / other;
            return *this;
        }

        int128 &operator%=(int128 other) noexcept {
            *this = *this % other;
            return *this;
        }

        constexpr int128 operator-() const noexcept {
#if defined(__SIZEOF_INT128__)
            return from_wide(-to_wide(*this));
#else
            uint128 u(hi_, lo_);
            uint128 neg = uint128(0, 0) - u;
            return int128(static_cast<int64_t>(neg.high()), neg.low());
#endif
        }

        constexpr int128 &operator++() noexcept {
            *this += int128(1);
            return *this;
        }

        constexpr int128 operator++(int) noexcept {
            int128 tmp(*this);
            ++(*this);
            return tmp;
        }

        constexpr int128 &operator--() noexcept {
            *this -= int128(1);
            return *this;
        }

        constexpr int128 operator--(int) noexcept {
            int128 tmp(*this);
            --(*this);
            return tmp;
        }

        friend constexpr int128 operator~(int128 v) noexcept {
            return {static_cast<int64_t>(~v.hi_), ~v.lo_};
        }

        friend constexpr int128 operator&(int128 a, int128 b) noexcept {
            return {static_cast<int64_t>(a.hi_ & b.hi_), a.lo_ & b.lo_};
        }

        friend constexpr int128 operator|(int128 a, int128 b) noexcept {
            return {static_cast<int64_t>(a.hi_ | b.hi_), a.lo_ | b.lo_};
        }

        friend constexpr int128 operator^(int128 a, int128 b) noexcept {
            return {static_cast<int64_t>(a.hi_ ^ b.hi_), a.lo_ ^ b.lo_};
        }

        constexpr int128 &operator&=(int128 other) noexcept {
            hi_ &= other.hi_;
            lo_ &= other.lo_;
            return *this;
        }

        constexpr int128 &operator|=(int128 other) noexcept {
            hi_ |= other.hi_;
            lo_ |= other.lo_;
            return *this;
        }

        constexpr int128 &operator^=(int128 other) noexcept {
            hi_ ^= other.hi_;
            lo_ ^= other.lo_;
            return *this;
        }

        friend constexpr int128 operator<<(int128 v, int shift) noexcept {
            return int128(uint128(v.hi_, v.lo_) << shift);
        }

        friend constexpr int128 operator>>(int128 v, int shift) noexcept {
#if defined(__SIZEOF_INT128__)
            auto w = to_wide(v);
            if (shift <= 0) {
                return v;
            }
            if (shift >= 128) {
                w = (w < 0) ? static_cast<wide_type>(-1) : static_cast<wide_type>(0);
            } else {
                w >>= shift;
            }
            return from_wide(w);
#else
            bool neg = v.high() < 0;
            uint128 u(v.hi_, v.lo_);
            if (neg) {
                u = uint128(0, 0) - u;
            }
            u >>= shift;
            if (neg) {
                uint128 tmp = uint128(0, 0) - u;
                return int128(static_cast<int64_t>(tmp.high()), tmp.low());
            }
            return int128(static_cast<int64_t>(u.high()), u.low());
#endif
        }

        constexpr int128 &operator<<=(int shift) noexcept {
            *this = *this << shift;
            return *this;
        }

        constexpr int128 &operator>>=(int shift) noexcept {
            *this = *this >> shift;
            return *this;
        }

        [[nodiscard]] std::string to_string() const {
            if (hi_ == 0 && lo_ == 0) {
                return "0";
            }
            bool neg = (static_cast<int64_t>(hi_) < 0);
            int128 tmp = *this;
            if (neg) {
                tmp = -tmp;
            }

            std::string res;
            res.reserve(40);

            while (tmp.hi_ != 0 || tmp.lo_ != 0) {
                uint128 u(tmp.hi_, tmp.lo_);
                uint128 q{};
                uint128 r{};
#if defined(__SIZEOF_INT128__)
                uint128 d(0, 10);
                q = u / d;
                r = u % d;
#else
                uint128 d(0, 10);
                uint128::div_mod(u, d, q, r);
#endif
                uint64_t digit = r.low();
                res.push_back(static_cast<char>('0' + digit));
                tmp = int128(static_cast<int64_t>(q.high()), q.low());
            }

            if (neg) {
                res.push_back('-');
            }

            std::ranges::reverse(res);
            return res;
        }

    private:
        uint64_t hi_{0};
        uint64_t lo_{0};

#if defined(__SIZEOF_INT128__)
        using wide_type = __int128;

        static constexpr wide_type to_wide(int128 v) noexcept {
            return (static_cast<wide_type>(static_cast<int64_t>(v.hi_)) << 64) |
                   static_cast<wide_type>(v.lo_);
        }

        static constexpr int128 from_wide(wide_type w) noexcept {
            auto lo = static_cast<uint64_t>(w);
            auto hi = static_cast<uint64_t>(w >> 64);
            return {static_cast<int64_t>(hi), lo};
        }
#endif

        static inline uint128 unsigned_abs(int128 v) noexcept {
            if (v.high() < 0) {
                uint128 u(v.hi_, v.lo_);
                uint128 neg = uint128(0, 0) - u;
                return neg;
            }
            return {v.hi_, v.lo_};
        }
    };

    inline std::ostream &operator<<(std::ostream &os, const int128 &v) {
        return os << v.to_string();
    }

    class int256;

    class uint256 {
    public:
        friend class int256;

        constexpr uint256() noexcept = default;

        constexpr uint256(uint128 hi, uint128 lo) noexcept
            : hi_(hi), lo_(lo) {
        }

        constexpr uint256(uint128 v) noexcept
            : hi_(0, 0), lo_(v) {
        }

        template<typename T,
            std::enable_if_t<std::is_integral_v<T> && std::is_unsigned_v<T>, int> = 0>
        constexpr uint256(T v) noexcept
            : hi_(0, 0),
              lo_(static_cast<std::uint64_t>(v)) {
        }

        template<typename T,
            std::enable_if_t<std::is_integral_v<T> && std::is_signed_v<T>, int> = 0>
        constexpr uint256(T v) noexcept
            : hi_(0, 0),
              lo_(0, 0) {
            const bool neg = (v < 0);
            const auto lo64 = static_cast<std::uint64_t>(v);

            hi_ = neg ? uint128{~0ull, ~0ull} : uint128{0ull, 0ull};
            lo_ = neg ? uint128{~0ull, lo64} : uint128{0ull, lo64};
        }

        explicit constexpr uint256(const int256 &v) noexcept;

        [[nodiscard]] constexpr uint128 high() const noexcept { return hi_; }
        [[nodiscard]] constexpr uint128 low() const noexcept { return lo_; }

        explicit constexpr operator uint64_t() const noexcept { return lo_.low(); }

        explicit constexpr operator int64_t() const noexcept {
            return static_cast<int64_t>(lo_.low());
        }

        [[nodiscard]] constexpr auto operator<=>(const uint256 &) const noexcept = default;

        friend constexpr uint256 operator+(uint256 a, uint256 b) noexcept {
            const uint128 lo = a.lo_ + b.lo_;
            const bool carry = (lo < a.lo_);
            const uint128 hi = a.hi_ + b.hi_ + uint128(0, static_cast<std::uint64_t>(carry ? 1U : 0U));
            return {hi, lo};
        }

        friend constexpr uint256 operator-(uint256 a, uint256 b) noexcept {
            const bool borrow = (a.lo_ < b.lo_);
            const uint128 lo = a.lo_ - b.lo_;
            const uint128 hi = a.hi_ - b.hi_ - uint128(0, static_cast<std::uint64_t>(borrow ? 1U : 0U));
            return {hi, lo};
        }

        friend inline uint256 operator*(uint256 a, uint256 b) noexcept {
            using u64 = std::uint64_t;

            const u64 A[4] = {a.lo_.low(), a.lo_.high(), a.hi_.low(), a.hi_.high()};
            const u64 B[4] = {b.lo_.low(), b.lo_.high(), b.hi_.low(), b.hi_.high()};
            u64 R[4] = {0, 0, 0, 0};

#if defined(__SIZEOF_INT128__)
            using u128 = unsigned __int128;

            auto add_prod = [&](int k, u64 x, u64 y, u128 &carry) noexcept {
                const u128 prod = static_cast<u128>(x) * static_cast<u128>(y);

                u128 acc = prod;
                acc += static_cast<u128>(R[k]);
                const bool of1 = acc < prod;

                const u128 acc2 = acc + carry;
                const bool of2 = acc2 < acc;

                acc = acc2;
                R[k] = static_cast<u64>(acc);

                u128 c = (acc >> 64);
                if (of1 || of2) {
                    c += (static_cast<u128>(1) << 64);
                }
                carry = c;
            };

            for (int i = 0; i < 4; ++i) {
                u128 carry = 0;
                for (int j = 0; j < 4 - i; ++j) {
                    add_prod(i + j, A[i], B[j], carry);
                }
            }

            return {uint128(R[3], R[2]), uint128(R[1], R[0])};
#else
            auto add64 = [](std::uint64_t &x, std::uint64_t y, std::uint64_t &carry) noexcept {
                const std::uint64_t old = x;
                x += y;
                if (x < old) ++carry;
            };

            auto add128_to_limbs = [&](std::uint64_t add_hi, std::uint64_t add_lo,
                                       std::uint64_t &r_lo, std::uint64_t &r_hi, std::uint64_t &r_ex) noexcept {
                std::uint64_t c = 0;
                add64(r_lo, add_lo, c);
                add64(r_hi, add_hi, c);
                r_ex += c;
            };

            std::uint64_t r0 = 0, r1 = 0, r2 = 0, r3 = 0;
            std::uint64_t ex1 = 0, ex2 = 0, ex3 = 0;

            {
                std::uint64_t hi, lo;
                mul_64_64_128(a0, b0, hi, lo);
                r0 = lo;
                r1 = hi;
            }

            {
                std::uint64_t hi, lo;
                mul_64_64_128(a0, b1, hi, lo);
                add128_to_limbs(hi, lo, r1, r2, ex2);

                mul_64_64_128(a1, b0, hi, lo);
                add128_to_limbs(hi, lo, r1, r2, ex2);
            }

            {
                std::uint64_t hi, lo;
                mul_64_64_128(a0, b2, hi, lo);
                add128_to_limbs(hi, lo, r2, r3, ex3);

                mul_64_64_128(a1, b1, hi, lo);
                add128_to_limbs(hi, lo, r2, r3, ex3);

                mul_64_64_128(a2, b0, hi, lo);
                add128_to_limbs(hi, lo, r2, r3, ex3);

                std::uint64_t c = 0;
                add64(r2, ex2, c);
                add64(r3, 0, c);
                (void) c;
            }

            {
                std::uint64_t hi, lo;
                mul_64_64_128(a0, b3, hi, lo);
                add128_to_limbs(hi, lo, r3, ex1, ex1);
                mul_64_64_128(a1, b2, hi, lo);
                add128_to_limbs(hi, lo, r3, ex1, ex1);
                mul_64_64_128(a2, b1, hi, lo);
                add128_to_limbs(hi, lo, r3, ex1, ex1);
                mul_64_64_128(a3, b0, hi, lo);
                add128_to_limbs(hi, lo, r3, ex1, ex1);

                std::uint64_t c = 0;
                add64(r3, ex3, c);
                (void) c;
            }

            return {uint128(r3, r2), uint128(r1, r0)};
#endif
        }


        friend inline uint256 operator/(uint256 a, uint256 b) noexcept {
            uint256 q{}, r{};
            div_mod(a, b, q, r);
            return q;
        }

        friend inline uint256 operator%(uint256 a, uint256 b) noexcept {
            uint256 q{}, r{};
            div_mod(a, b, q, r);
            return r;
        }

        constexpr uint256 &operator+=(uint256 other) noexcept {
            *this = *this + other;
            return *this;
        }

        constexpr uint256 &operator-=(uint256 other) noexcept {
            *this = *this - other;
            return *this;
        }

        uint256 &operator*=(uint256 other) noexcept {
            *this = *this * other;
            return *this;
        }

        uint256 &operator/=(uint256 other) noexcept {
            *this = *this / other;
            return *this;
        }

        uint256 &operator%=(uint256 other) noexcept {
            *this = *this % other;
            return *this;
        }

        constexpr uint256 &operator++() noexcept {
            lo_ += uint128(0, 1);
            if (lo_ == uint128(0, 0)) {
                hi_ += uint128(0, 1);
            }
            return *this;
        }

        constexpr uint256 operator++(int) noexcept {
            uint256 tmp(*this);
            ++(*this);
            return tmp;
        }

        constexpr uint256 &operator--() noexcept {
            if (lo_ == uint128(0, 0)) {
                hi_ -= uint128(0, 1);
            }
            lo_ -= uint128(0, 1);
            return *this;
        }

        constexpr uint256 operator--(int) noexcept {
            uint256 tmp(*this);
            --(*this);
            return tmp;
        }

        friend constexpr uint256 operator~(uint256 x) noexcept {
            return {~x.hi_, ~x.lo_};
        }

        friend constexpr uint256 operator&(uint256 a, uint256 b) noexcept {
            return {a.hi_ & b.hi_, a.lo_ & b.lo_};
        }

        friend constexpr uint256 operator|(uint256 a, uint256 b) noexcept {
            return {a.hi_ | b.hi_, a.lo_ | b.lo_};
        }

        friend constexpr uint256 operator^(uint256 a, uint256 b) noexcept {
            return {a.hi_ ^ b.hi_, a.lo_ ^ b.lo_};
        }

        constexpr uint256 &operator&=(uint256 other) noexcept {
            hi_ &= other.hi_;
            lo_ &= other.lo_;
            return *this;
        }

        constexpr uint256 &operator|=(uint256 other) noexcept {
            hi_ |= other.hi_;
            lo_ |= other.lo_;
            return *this;
        }

        constexpr uint256 &operator^=(uint256 other) noexcept {
            hi_ ^= other.hi_;
            lo_ ^= other.lo_;
            return *this;
        }

        friend constexpr uint256 operator<<(uint256 v, int shift) noexcept {
            if (shift <= 0) return v;
            if (shift >= 256) return {uint128(0, 0), uint128(0, 0)};
            if (shift >= 128) {
                return {v.lo_ << (shift - 128), uint128(0, 0)};
            }
            const uint128 hi = (v.hi_ << shift) | (v.lo_ >> (128 - shift));
            const uint128 lo = (v.lo_ << shift);
            return {hi, lo};
        }

        friend constexpr uint256 operator>>(uint256 v, int shift) noexcept {
            if (shift <= 0) return v;
            if (shift >= 256) return {uint128(0, 0), uint128(0, 0)};
            if (shift >= 128) {
                return {uint128(0, 0), v.hi_ >> (shift - 128)};
            }
            const uint128 hi = (v.hi_ >> shift);
            const uint128 lo = (v.lo_ >> shift) | (v.hi_ << (128 - shift));
            return {hi, lo};
        }

        constexpr uint256 &operator<<=(int shift) noexcept {
            *this = *this << shift;
            return *this;
        }

        constexpr uint256 &operator>>=(int shift) noexcept {
            *this = *this >> shift;
            return *this;
        }

        [[nodiscard]] std::string to_string() const {
            if (hi_ == uint128(0, 0) && lo_ == uint128(0, 0)) {
                return "0";
            }

            uint256 tmp = *this;
            std::string res;
            res.reserve(80);

            const uint256 ten(uint128(0, 0), uint128(0, 10));
            while (tmp.hi_ != uint128(0, 0) || tmp.lo_ != uint128(0, 0)) {
                const uint256 digit = tmp % ten;
                res.push_back(static_cast<char>('0' + static_cast<unsigned>(digit.lo_.low())));
                tmp /= ten;
            }

            std::ranges::reverse(res);
            return res;
        }

    private:
        uint128 hi_{0, 0};
        uint128 lo_{0, 0};

#if defined(__SIZEOF_INT128__)
        static inline void mul_128_128_256(uint128 a, uint128 b, uint128 &hi, uint128 &lo) noexcept {
            const std::uint64_t a0 = a.low();
            const std::uint64_t a1 = a.high();
            const std::uint64_t b0 = b.low();
            const std::uint64_t b1 = b.high();

            unsigned __int128 t0 = static_cast<unsigned __int128>(a0) * static_cast<unsigned __int128>(b0);
            const auto r0 = static_cast<std::uint64_t>(t0);
            unsigned __int128 carry = (t0 >> 64);

            unsigned __int128 t1 =
                    static_cast<unsigned __int128>(a0) * static_cast<unsigned __int128>(b1) +
                    static_cast<unsigned __int128>(a1) * static_cast<unsigned __int128>(b0) +
                    carry;
            const auto r1 = static_cast<std::uint64_t>(t1);
            carry = (t1 >> 64);

            unsigned __int128 t2 =
                    static_cast<unsigned __int128>(a1) * static_cast<unsigned __int128>(b1) +
                    carry;
            const auto r2 = static_cast<std::uint64_t>(t2);
            const auto r3 = static_cast<std::uint64_t>(t2 >> 64);

            lo = uint128(r1, r0);
            hi = uint128(r3, r2);
        }
#else
        static inline void mul_64_64_128(std::uint64_t x, std::uint64_t y,
                                         std::uint64_t &hi, std::uint64_t &lo) noexcept {
            const std::uint64_t x0 = x & 0xffffffffu;
            const std::uint64_t x1 = x >> 32;
            const std::uint64_t y0 = y & 0xffffffffu;
            const std::uint64_t y1 = y >> 32;

            const std::uint64_t p0 = x0 * y0;
            const std::uint64_t p1 = x0 * y1;
            const std::uint64_t p2 = x1 * y0;
            const std::uint64_t p3 = x1 * y1;

            std::uint64_t mid = (p0 >> 32) + (p1 & 0xffffffffu) + (p2 & 0xffffffffu);
            hi = p3 + (p1 >> 32) + (p2 >> 32) + (mid >> 32);
            lo = (mid << 32) | (p0 & 0xffffffffu);
        }

        static inline void add_carry(std::uint64_t &x, std::uint64_t y, std::uint64_t &carry) noexcept {
            const std::uint64_t old = x;
            x += y;
            if (x < old) ++carry;
        }

        static inline void mul_128_128_256(uint128 a, uint128 b, uint128 &hi, uint128 &lo) noexcept {
            const std::uint64_t a0 = a.low();
            const std::uint64_t a1 = a.high();
            const std::uint64_t b0 = b.low();
            const std::uint64_t b1 = b.high();

            std::uint64_t p00_hi, p00_lo;
            std::uint64_t p01_hi, p01_lo;
            std::uint64_t p10_hi, p10_lo;
            std::uint64_t p11_hi, p11_lo;

            mul_64_64_128(a0, b0, p00_hi, p00_lo);
            mul_64_64_128(a0, b1, p01_hi, p01_lo);
            mul_64_64_128(a1, b0, p10_hi, p10_lo);
            mul_64_64_128(a1, b1, p11_hi, p11_lo);

            std::uint64_t r0 = p00_lo;

            std::uint64_t r1 = p00_hi;
            std::uint64_t c = 0;
            add_carry(r1, p01_lo, c);
            add_carry(r1, p10_lo, c);

            std::uint64_t r2 = p01_hi;
            std::uint64_t c2 = 0;
            add_carry(r2, p10_hi, c2);
            add_carry(r2, p11_lo, c2);
            add_carry(r2, c, c2);

            std::uint64_t r3 = p11_hi + c2;

            lo = uint128(r1, r0);
            hi = uint128(r3, r2);
        }
#endif

        static inline int msb128(uint128 v) noexcept {
            if (v.high() != 0) return 64 + (63 - std::countl_zero(v.high()));
            if (v.low() != 0) return 63 - std::countl_zero(v.low());
            return -1;
        }

        static inline int msb(uint256 v) noexcept {
            const int mh = msb128(v.hi_);
            if (mh >= 0) return 128 + mh;
            return msb128(v.lo_);
        }

        static inline void div_mod(uint256 dividend, uint256 divisor, uint256 &q, uint256 &r) noexcept {
            const uint256 zero(uint128(0, 0), uint128(0, 0));

            if (divisor == zero) {
                q = zero;
                r = zero;
                return;
            }
            if (dividend < divisor) {
                q = zero;
                r = dividend;
                return;
            }
            if (dividend == divisor) {
                q = uint256(uint128(0, 0), uint128(0, 1));
                r = zero;
                return;
            }

            int shift = msb(dividend) - msb(divisor);
            uint256 denom = divisor << shift;

            q = zero;

            const uint256 one(uint128(0, 0), uint128(0, 1));

            while (shift >= 0) {
                if (dividend >= denom) {
                    dividend -= denom;
                    q = q | (one << shift);
                }
                denom >>= 1;
                --shift;
            }

            r = dividend;
        }
    };

    inline std::ostream &operator<<(std::ostream &os, const uint256 &v) {
        return os << v.to_string();
    }

    class int256 {
    public:
        constexpr int256() noexcept = default;

        constexpr int256(int v) noexcept
            : int256(static_cast<int64_t>(v)) {
        }

        constexpr int256(unsigned int v) noexcept
            : int256(static_cast<uint64_t>(v)) {
        }

        constexpr int256(int64_t v) noexcept
            : hi_(v < 0 ? uint128{~std::uint64_t{0}, ~std::uint64_t{0}} : uint128{0, 0}),
              lo_(v < 0
                      ? uint128{~0ull, static_cast<std::uint64_t>(v)}
                      : uint128{0ull, static_cast<std::uint64_t>(v)}) {
        }

        constexpr int256(uint64_t v) noexcept
            : hi_(0, 0), lo_(v) {
        }

        constexpr int256(int64_t hi_hi, uint64_t hi_lo, uint64_t lo_hi, uint64_t lo_lo) noexcept
            : hi_(static_cast<std::uint64_t>(hi_hi), hi_lo),
              lo_(lo_hi, lo_lo) {
        }

        constexpr int256(uint128 hi, uint128 lo) noexcept
            : hi_(hi), lo_(lo) {
        }

        explicit constexpr int256(uint256 u) noexcept
            : hi_(u.high()), lo_(u.low()) {
        }

        [[nodiscard]] constexpr int is_negative() const noexcept {
            return (hi_.high() >> 63) != 0;
        }

        [[nodiscard]] constexpr uint128 high() const noexcept { return hi_; }
        [[nodiscard]] constexpr uint128 low() const noexcept { return lo_; }

        explicit constexpr operator int64_t() const noexcept {
            return static_cast<int64_t>(lo_.low());
        }

        explicit constexpr operator uint64_t() const noexcept {
            return lo_.low();
        }

        friend constexpr bool operator==(const int256 &, const int256 &) noexcept = default;

        friend constexpr std::strong_ordering operator<=>(const int256 &a, const int256 &b) noexcept {
            const bool an = a.is_negative();
            const bool bn = b.is_negative();
            if (an != bn) {
                return an ? std::strong_ordering::less : std::strong_ordering::greater;
            }
            const uint256 ua(a.hi_, a.lo_);
            const uint256 ub(b.hi_, b.lo_);
            if (!an) return ua <=> ub;
            const auto ord = ua <=> ub;
            if (ord == std::strong_ordering::less) return std::strong_ordering::greater;
            if (ord == std::strong_ordering::greater) return std::strong_ordering::less;
            return std::strong_ordering::equal;
        }

        friend constexpr int256 operator+(int256 a, int256 b) noexcept {
            const uint256 ua(a.hi_, a.lo_);
            const uint256 ub(b.hi_, b.lo_);
            const uint256 ur = ua + ub;
            return {ur.high(), ur.low()};
        }

        friend constexpr int256 operator-(int256 a, int256 b) noexcept {
            const uint256 ua(a.hi_, a.lo_);
            const uint256 ub(b.hi_, b.lo_);
            const uint256 ur = ua - ub;
            return {ur.high(), ur.low()};
        }

        friend inline int256 operator*(int256 a, int256 b) noexcept {
            const uint256 ua(a.hi_, a.lo_);
            const uint256 ub(b.hi_, b.lo_);
            const uint256 ur = ua * ub;
            return {ur.high(), ur.low()};
        }

        friend inline int256 operator/(int256 a, int256 b) noexcept {
            const bool neg = a.is_negative() ^ b.is_negative();
            const uint256 ua = unsigned_abs(a);
            const uint256 ub = unsigned_abs(b);

            uint256 uq{}, ur{};
            uint256::div_mod(ua, ub, uq, ur);

            if (neg) {
                const uint256 tmp = uint256(uint128(0, 0), uint128(0, 0)) - uq;
                return {tmp.high(), tmp.low()};
            }
            return {uq.high(), uq.low()};
        }

        friend inline int256 operator%(int256 a, int256 b) noexcept {
            const bool neg = a.is_negative();
            const uint256 ua = unsigned_abs(a);
            const uint256 ub = unsigned_abs(b);

            uint256 uq{}, ur{};
            uint256::div_mod(ua, ub, uq, ur);

            if (neg) {
                const uint256 tmp = uint256(uint128(0, 0), uint128(0, 0)) - ur;
                return {tmp.high(), tmp.low()};
            }
            return {ur.high(), ur.low()};
        }

        constexpr int256 &operator+=(int256 other) noexcept {
            *this = *this + other;
            return *this;
        }

        constexpr int256 &operator-=(int256 other) noexcept {
            *this = *this - other;
            return *this;
        }

        int256 &operator*=(int256 other) noexcept {
            *this = *this * other;
            return *this;
        }

        int256 &operator/=(int256 other) noexcept {
            *this = *this / other;
            return *this;
        }

        int256 &operator%=(int256 other) noexcept {
            *this = *this % other;
            return *this;
        }

        constexpr int256 operator-() const noexcept {
            const uint256 u(hi_, lo_);
            const uint256 neg = uint256(uint128(0, 0), uint128(0, 0)) - u;
            return {neg.high(), neg.low()};
        }

        constexpr int256 &operator++() noexcept {
            *this += int256(1);
            return *this;
        }

        constexpr int256 operator++(int) noexcept {
            int256 tmp(*this);
            ++(*this);
            return tmp;
        }

        constexpr int256 &operator--() noexcept {
            *this -= int256(1);
            return *this;
        }

        constexpr int256 operator--(int) noexcept {
            int256 tmp(*this);
            --(*this);
            return tmp;
        }

        friend constexpr int256 operator~(int256 v) noexcept {
            return {~v.hi_, ~v.lo_};
        }

        friend constexpr int256 operator&(int256 a, int256 b) noexcept {
            return {a.hi_ & b.hi_, a.lo_ & b.lo_};
        }

        friend constexpr int256 operator|(int256 a, int256 b) noexcept {
            return {a.hi_ | b.hi_, a.lo_ | b.lo_};
        }

        friend constexpr int256 operator^(int256 a, int256 b) noexcept {
            return {a.hi_ ^ b.hi_, a.lo_ ^ b.lo_};
        }

        constexpr int256 &operator&=(int256 other) noexcept {
            hi_ &= other.hi_;
            lo_ &= other.lo_;
            return *this;
        }

        constexpr int256 &operator|=(int256 other) noexcept {
            hi_ |= other.hi_;
            lo_ |= other.lo_;
            return *this;
        }

        constexpr int256 &operator^=(int256 other) noexcept {
            hi_ ^= other.hi_;
            lo_ ^= other.lo_;
            return *this;
        }

        friend constexpr int256 operator<<(int256 v, int shift) noexcept {
            const uint256 u(v.hi_, v.lo_);
            const uint256 r = u << shift;
            return {r.high(), r.low()};
        }

        friend constexpr int256 operator>>(int256 v, int shift) noexcept {
            if (shift <= 0) return v;
            if (shift >= 256) {
                return v.is_negative()
                           ? int256(uint128{~0ull, ~0ull}, uint128{~0ull, ~0ull})
                           : int256(uint128{0, 0}, uint128{0, 0});
            }

            const bool neg = v.is_negative();
            uint256 u(v.hi_, v.lo_);
            uint256 r = u >> shift;

            if (neg) {
                const uint256 fill = (~uint256(0U)) << (256 - shift);
                r = uint256(r.high(), r.low()) | fill;
            }

            return {r.high(), r.low()};
        }

        constexpr int256 &operator<<=(int shift) noexcept {
            *this = *this << shift;
            return *this;
        }

        constexpr int256 &operator>>=(int shift) noexcept {
            *this = *this >> shift;
            return *this;
        }

        [[nodiscard]] std::string to_string() const {
            if (hi_ == uint128(0, 0) && lo_ == uint128(0, 0)) {
                return "0";
            }

            const bool neg = is_negative();
            uint256 tmp = neg ? unsigned_abs(*this) : uint256(hi_, lo_);

            std::string res;
            res.reserve(80);

            const uint256 ten(uint128(0, 0), uint128(0, 10));
            while (tmp.high() != uint128(0, 0) || tmp.low() != uint128(0, 0)) {
                const uint256 digit = tmp % ten;
                res.push_back(static_cast<char>('0' + static_cast<unsigned>(digit.low().low())));
                tmp /= ten;
            }

            if (neg) res.push_back('-');
            std::ranges::reverse(res);
            return res;
        }

    private:
        uint128 hi_{0, 0};
        uint128 lo_{0, 0};

        static inline uint256 unsigned_abs(int256 v) noexcept {
            if (!v.is_negative()) {
                return {v.hi_, v.lo_};
            }
            const uint256 u(v.hi_, v.lo_);
            return uint256(uint128(0, 0), uint128(0, 0)) - u;
        }
    };

    constexpr uint256::uint256(const int256 &v) noexcept
        : hi_(v.high()), lo_(v.low()) {
    }

    constexpr bool operator==(const uint256 &a, const int256 &b) noexcept {
        if (b.is_negative()) return false;
        return a == uint256(b);
    }

    constexpr bool operator==(const int256 &a, const uint256 &b) noexcept {
        return b == a;
    }

    constexpr std::strong_ordering operator<=>(const uint256 &a, const int256 &b) noexcept {
        if (b.is_negative()) return std::strong_ordering::greater;
        return a <=> uint256(b);
    }

    constexpr std::strong_ordering operator<=>(const int256 &a, const uint256 &b) noexcept {
        const auto ord = (b <=> a);
        if (ord == std::strong_ordering::less) return std::strong_ordering::greater;
        if (ord == std::strong_ordering::greater) return std::strong_ordering::less;
        return std::strong_ordering::equal;
    }

    constexpr uint256 operator+(uint256 a, const int256 &b) noexcept { return a + uint256(b); }
    constexpr uint256 operator-(uint256 a, const int256 &b) noexcept { return a - uint256(b); }
    inline uint256 operator*(uint256 a, const int256 &b) noexcept { return a * uint256(b); }
    inline uint256 operator/(uint256 a, const int256 &b) noexcept { return a / uint256(b); }
    inline uint256 operator%(uint256 a, const int256 &b) noexcept { return a % uint256(b); }

    constexpr uint256 operator&(uint256 a, const int256 &b) noexcept { return a & uint256(b); }
    constexpr uint256 operator|(uint256 a, const int256 &b) noexcept { return a | uint256(b); }
    constexpr uint256 operator^(uint256 a, const int256 &b) noexcept { return a ^ uint256(b); }

    constexpr uint256 &operator+=(uint256 &a, const int256 &b) noexcept {
        a = a + b;
        return a;
    }

    constexpr uint256 &operator-=(uint256 &a, const int256 &b) noexcept {
        a = a - b;
        return a;
    }

    inline uint256 &operator*=(uint256 &a, const int256 &b) noexcept {
        a = a * b;
        return a;
    }

    inline uint256 &operator/=(uint256 &a, const int256 &b) noexcept {
        a = a / b;
        return a;
    }

    inline uint256 &operator%=(uint256 &a, const int256 &b) noexcept {
        a = a % b;
        return a;
    }

    constexpr uint256 &operator&=(uint256 &a, const int256 &b) noexcept {
        a = a & b;
        return a;
    }

    constexpr uint256 &operator|=(uint256 &a, const int256 &b) noexcept {
        a = a | b;
        return a;
    }

    constexpr uint256 &operator^=(uint256 &a, const int256 &b) noexcept {
        a = a ^ b;
        return a;
    }

    inline std::ostream &operator<<(std::ostream &os, const int256 &v) {
        return os << v.to_string();
    }
} // namespace usub::umath

namespace std {
    template<>
    struct numeric_limits<usub::umath::uint128> {
    public:
        static constexpr bool is_specialized = true;
        static constexpr bool is_signed = false;
        static constexpr bool is_integer = true;
        static constexpr bool is_exact = true;
        static constexpr bool has_infinity = false;
        static constexpr bool has_quiet_NaN = false;
        static constexpr bool has_signaling_NaN = false;
        static constexpr float_denorm_style has_denorm = denorm_absent;
        static constexpr bool has_denorm_loss = false;
        static constexpr float_round_style round_style = round_toward_zero;
        static constexpr bool is_iec559 = false;
        static constexpr bool is_bounded = true;
        static constexpr bool is_modulo = true;
        static constexpr int digits = 128;
        static constexpr int digits10 = 38;
        static constexpr int max_digits10 = 0;
        static constexpr int radix = 2;
        static constexpr int min_exponent = 0;
        static constexpr int min_exponent10 = 0;
        static constexpr int max_exponent = 0;
        static constexpr int max_exponent10 = 0;
        static constexpr bool traps = numeric_limits<unsigned long long>::traps;
        static constexpr bool tinyness_before = false;

        static constexpr usub::umath::uint128 (min)() noexcept {
            return {0, 0};
        }

        static constexpr usub::umath::uint128 lowest() noexcept {
            return (min)();
        }

        static constexpr usub::umath::uint128 (max)() noexcept {
            return {
                (std::numeric_limits<unsigned long long>::max)(),
                (std::numeric_limits<unsigned long long>::max)()
            };
        }

        static constexpr usub::umath::uint128 epsilon() noexcept {
            return {0, 0};
        }

        static constexpr usub::umath::uint128 round_error() noexcept {
            return {0, 0};
        }

        static constexpr usub::umath::uint128 infinity() noexcept {
            return {0, 0};
        }

        static constexpr usub::umath::uint128 quiet_NaN() noexcept {
            return {0, 0};
        }

        static constexpr usub::umath::uint128 signaling_NaN() noexcept {
            return {0, 0};
        }

        static constexpr usub::umath::uint128 denorm_min() noexcept {
            return {0, 0};
        }
    };

    template<>
    struct numeric_limits<usub::umath::int128> {
    public:
        static constexpr bool is_specialized = true;
        static constexpr bool is_signed = true;
        static constexpr bool is_integer = true;
        static constexpr bool is_exact = true;
        static constexpr bool has_infinity = false;
        static constexpr bool has_quiet_NaN = false;
        static constexpr bool has_signaling_NaN = false;
        static constexpr float_denorm_style has_denorm = denorm_absent;
        static constexpr bool has_denorm_loss = false;
        static constexpr float_round_style round_style = round_toward_zero;
        static constexpr bool is_iec559 = false;
        static constexpr bool is_bounded = true;
        static constexpr bool is_modulo = false;
        static constexpr int digits = 127;
        static constexpr int digits10 = 38;
        static constexpr int max_digits10 = 0;
        static constexpr int radix = 2;
        static constexpr int min_exponent = 0;
        static constexpr int min_exponent10 = 0;
        static constexpr int max_exponent = 0;
        static constexpr int max_exponent10 = 0;
        static constexpr bool traps = numeric_limits<long long>::traps;
        static constexpr bool tinyness_before = false;

        static constexpr usub::umath::int128 (min)() noexcept {
            return {
                (std::numeric_limits<long long>::min)(), 0ull
            };
        }

        static constexpr usub::umath::int128 lowest() noexcept {
            return (min)();
        }

        static constexpr usub::umath::int128 (max)() noexcept {
            return {
                (std::numeric_limits<long long>::max)(),
                (std::numeric_limits<unsigned long long>::max)()
            };
        }

        static constexpr usub::umath::int128 epsilon() noexcept {
            return {0};
        }

        static constexpr usub::umath::int128 round_error() noexcept {
            return {0};
        }

        static constexpr usub::umath::int128 infinity() noexcept {
            return {0};
        }

        static constexpr usub::umath::int128 quiet_NaN() noexcept {
            return {0};
        }

        static constexpr usub::umath::int128 signaling_NaN() noexcept {
            return {0};
        }

        static constexpr usub::umath::int128 denorm_min() noexcept {
            return {0};
        }
    };

    template<>
    struct numeric_limits<usub::umath::int256> {
    public:
        static constexpr bool is_specialized = true;
        static constexpr bool is_signed = true;
        static constexpr bool is_integer = true;
        static constexpr bool is_exact = true;
        static constexpr bool has_infinity = false;
        static constexpr bool has_quiet_NaN = false;
        static constexpr bool has_signaling_NaN = false;
        static constexpr float_denorm_style has_denorm = denorm_absent;
        static constexpr bool has_denorm_loss = false;
        static constexpr float_round_style round_style = round_toward_zero;
        static constexpr bool is_iec559 = false;
        static constexpr bool is_bounded = true;
        static constexpr bool is_modulo = false;
        static constexpr int digits = 255;
        static constexpr int digits10 = 76;
        static constexpr int max_digits10 = 0;
        static constexpr int radix = 2;
        static constexpr int min_exponent = 0;
        static constexpr int min_exponent10 = 0;
        static constexpr int max_exponent = 0;
        static constexpr int max_exponent10 = 0;
        static constexpr bool traps = numeric_limits<long long>::traps;
        static constexpr bool tinyness_before = false;

        static constexpr usub::umath::int256 (min)() noexcept {
            return usub::umath::int256(
                usub::umath::uint128{
                    (numeric_limits<unsigned long long>::max)(),
                    (numeric_limits<unsigned long long>::max)()
                },
                usub::umath::uint128{0, 0}
            );
        }

        static constexpr usub::umath::int256 lowest() noexcept {
            return (min)();
        }

        static constexpr usub::umath::int256 (max)() noexcept {
            return usub::umath::int256(
                usub::umath::uint128{
                    (numeric_limits<unsigned long long>::max)() >> 1,
                    (numeric_limits<unsigned long long>::max)()
                },
                usub::umath::uint128{
                    (numeric_limits<unsigned long long>::max)(),
                    (numeric_limits<unsigned long long>::max)()
                }
            );
        }

        static constexpr usub::umath::int256 epsilon() noexcept { return {0}; }
        static constexpr usub::umath::int256 round_error() noexcept { return {0}; }
        static constexpr usub::umath::int256 infinity() noexcept { return {0}; }
        static constexpr usub::umath::int256 quiet_NaN() noexcept { return {0}; }
        static constexpr usub::umath::int256 signaling_NaN() noexcept { return {0}; }
        static constexpr usub::umath::int256 denorm_min() noexcept { return {0}; }
    };

    template<>
    struct numeric_limits<usub::umath::uint256> {
    public:
        static constexpr bool is_specialized = true;
        static constexpr bool is_signed = false;
        static constexpr bool is_integer = true;
        static constexpr bool is_exact = true;
        static constexpr bool has_infinity = false;
        static constexpr bool has_quiet_NaN = false;
        static constexpr bool has_signaling_NaN = false;
        static constexpr float_denorm_style has_denorm = denorm_absent;
        static constexpr bool has_denorm_loss = false;
        static constexpr float_round_style round_style = round_toward_zero;
        static constexpr bool is_iec559 = false;
        static constexpr bool is_bounded = true;
        static constexpr bool is_modulo = true;
        static constexpr int digits = 256;
        static constexpr int digits10 = 76;
        static constexpr int max_digits10 = 0;
        static constexpr int radix = 2;
        static constexpr int min_exponent = 0;
        static constexpr int min_exponent10 = 0;
        static constexpr int max_exponent = 0;
        static constexpr int max_exponent10 = 0;
        static constexpr bool traps = numeric_limits<unsigned long long>::traps;
        static constexpr bool tinyness_before = false;

        static constexpr usub::umath::uint256 (min)() noexcept {
            return {usub::umath::uint128{0, 0}, usub::umath::uint128{0, 0}};
        }

        static constexpr usub::umath::uint256 lowest() noexcept { return (min)(); }

        static constexpr usub::umath::uint256 (max)() noexcept {
            return {
                usub::umath::uint128{
                    (numeric_limits<unsigned long long>::max)(), (numeric_limits<unsigned long long>::max)()
                },
                usub::umath::uint128{
                    (numeric_limits<unsigned long long>::max)(), (numeric_limits<unsigned long long>::max)()
                }
            };
        }

        static constexpr usub::umath::uint256 epsilon() noexcept { return (min)(); }
        static constexpr usub::umath::uint256 round_error() noexcept { return (min)(); }
        static constexpr usub::umath::uint256 infinity() noexcept { return (min)(); }
        static constexpr usub::umath::uint256 quiet_NaN() noexcept { return (min)(); }
        static constexpr usub::umath::uint256 signaling_NaN() noexcept { return (min)(); }
        static constexpr usub::umath::uint256 denorm_min() noexcept { return (min)(); }
    };
} // namespace std

#endif // USUB_MP_INT128_H
