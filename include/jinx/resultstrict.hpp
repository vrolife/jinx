/*
Copyright (C) 2022  pom@vro.life

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
