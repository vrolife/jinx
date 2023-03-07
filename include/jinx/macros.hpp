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
