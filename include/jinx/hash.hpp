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
#ifndef __jinx_hash_hpp__
#define __jinx_hash_hpp__

#include <cstddef>
#include <cstdint>

namespace jinx {
namespace hash {
namespace detail {

template<typename T>
constexpr
T hash_string(char const* string, T value) noexcept
{
    return (*string != 0) ? hash_string(string+1, (value << 5) + value + *string ) : value;
}

template<typename T>
constexpr
T hash_data(char const* begin, char const* end, T value) noexcept
{
    return (begin != end) ? hash_data(begin+1, end, (value << 5) + value + *begin ) : value;
}

} // namespace detail 

constexpr
uintptr_t hash_data(char const* begin, char const* end, uintptr_t value = 5381) noexcept
{
    return detail::hash_data<uintptr_t>(begin, end, value);
}

constexpr
uintptr_t hash_string(char const* string, uintptr_t value = 5381) noexcept
{
    return detail::hash_string<uintptr_t>(string, value);
}

constexpr
uint64_t hash_l_string(char const* string, uint64_t value = 5381) noexcept
{
    return detail::hash_string<uint64_t>(string, value);
}

constexpr
uint32_t hash32_string(char const* string, uint32_t value = 5381) noexcept
{
    return detail::hash_string<uint32_t>(string, value);
}

constexpr
uint16_t hash16_string(char const* string, uint16_t value = 5381) noexcept
{
    return detail::hash_string<uint16_t>(string, value);
}

constexpr static uintptr_t operator"" _hash(char const* string, size_t size) noexcept {
    return hash_string(string);
}

} // namespace hash
} // namespace jinx

#endif
