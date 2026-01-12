//
// Created by kirill on 1/12/26.
//

#include <iostream>
#include <umath/Numeric.h>

using money_18_2 = usub::umath::Numeric128<18, 2>;
using dec_36_18 = usub::umath::Numeric128<36, 18>;
using usub::umath::Err;
using usub::umath::Rounding;

int main() { {
        money_18_2 a("123.45");
        money_18_2 b(10);

        auto c = a * 3 - b / 2;
        if (!c) {
            // c.error()
        }
        std::cout << c << "\n";
    } {
        using usub::umath::Numeric;
        using usub::umath::Rounding;

        auto a = Numeric("1.2345");
        auto b = Numeric("3");

        auto p = Numeric::mul(a, b, 2, Rounding::HalfUp); // 3.70
        auto q = Numeric::div(a, b, 4, Rounding::Trunc); // 0.4115

        std::cout << p << std::endl;
        std::cout << q << std::endl;
    }
}
