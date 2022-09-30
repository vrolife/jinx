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
#ifndef __jinx_macros_hpp__
#define __jinx_macros_hpp__

#if __cplusplus < 201103L
#define JINX_NO_DISCARD __attribute__((warn_unused_result))

#elif __cplusplus < 201703L
#ifdef __clang__
#define JINX_NO_DISCARD [[clang::warn_unused_result]]
#else
#define JINX_NO_DISCARD [[gnu::warn_unused_result]]
#endif

#else
#define JINX_NO_DISCARD [[nodiscard]]
#endif

#define JINX_NO_COPY_NO_MOVE(CLASS) \
    CLASS(const CLASS&) = delete; \
    CLASS(CLASS&&) noexcept = delete; \
    CLASS& operator=(const CLASS&) = delete; \
    CLASS& operator=(CLASS&&) noexcept = delete;

#define JINX_NO_COPY(CLASS) \
    CLASS(const CLASS&) = delete; \
    CLASS& operator=(const CLASS&) = delete;

#define JINX_NO_MOVE(CLASS) \
    CLASS(CLASS&&) noexcept = delete; \
    CLASS& operator=(CLASS&&) noexcept = delete;

#define JINX_DEFAULT_COPY_DEFAULT_MOVE(CLASS) \
    CLASS(const CLASS&) = default; \
    CLASS(CLASS&&) noexcept = default; \
    CLASS& operator=(const CLASS&) = default; \
    CLASS& operator=(CLASS&&) noexcept = default;

#define JINX_DEFAULT_COPY(CLASS) \
    CLASS(const CLASS&) = default; \
    CLASS& operator=(const CLASS&) = default;

#define JINX_DEFAULT_MOVE(CLASS) \
    CLASS(CLASS&&) noexcept = default; \
    CLASS& operator=(CLASS&&) noexcept = default;

#define JINX_LIKELY(x)   __builtin_expect(bool(x), true)
#define JINX_UNLIKELY(x) __builtin_expect(bool(x), false)

#endif
