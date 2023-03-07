/*
MIT License

Copyright (c) 2023 pom@vro.life

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#ifndef __resultvariant_hpp__
#define __resultvariant_hpp__

#include <cstdlib>

#include <jinx/variant.hpp>
#include <jinx/overload.hpp>

namespace jinx {
namespace detail {
    struct ResultStrictLazyDetector { };

    template<typename T, typename... Args>
    struct HasLazyDetector {
        template<typename C, typename = decltype(std::declval<C>().operator()(std::declval<Args>()...))>
        static std::true_type test(int);

        template<typename C>
        static std::false_type test(...);

        static constexpr bool value = decltype(test<T>(0))::value;
    };
}

template<typename... Types>
class ResultStrict {
    Variant<Types...> _value{};

public:
    template<typename T>
    ResultStrict(T&& value)  // NOLINT
    : _value(std::forward<T>(value)) { }

    JINX_DEFAULT_COPY_DEFAULT_MOVE(ResultStrict);

    ~ResultStrict() noexcept(false) {
#if !defined(NDEBUG) && !defined(JINX_RESULT_NO_CHECK)
#if defined(JINX_RESULT_THROW)
        if (not _value.empty()) {
            throw std::logic_error("Unahndled result");  // NOLINT
        }
#else
        jinx_assert(_value.empty() && "Unahndled result");
#endif
#endif
    }

    template<typename... Ts>
    JINX_NO_DISCARD
    ResultBool operator()(Ts&&... args)
    {
        auto fun = overload(std::forward<Ts>(args)...);
        return (*this)(fun);
    }

    template<typename... Ts, typename F=detail::__overload_impl<Ts...>>
    JINX_NO_DISCARD
    ResultBool operator()(F&& fun)
    {
#if !defined(NDEBUG) && !defined(JINX_RESULT_NO_CHECK)
        const bool lazy = detail::HasLazyDetector<decltype(fun), detail::ResultStrictLazyDetector>::value;
#if defined(JINX_RESULT_THROW)
        if (lazy) {
            _value.reset();
            throw std::logic_error("Unahndled result");  // NOLINT
        }
#else
        static_assert(not lazy, "Unahndled result");
#endif
#endif
        
        auto result = _value(std::forward<F>(fun));
        _value.reset();
        return result;
    }
};


} // namespace jinx

#endif
