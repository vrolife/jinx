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
