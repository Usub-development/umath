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
} // namespace std

#endif // USUB_MP_INT128_H
