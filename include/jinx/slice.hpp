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
#ifndef __jinx_slice_hpp__
#define __jinx_slice_hpp__

#include <cstddef>
#include <cstring>

namespace jinx {

class SliceMutable {
public:
    typedef void* ValueType;
    typedef char* iterator;
    
    void* _data;
    size_t _size;

    SliceMutable() : _data(nullptr), _size(0) { }

    SliceMutable(void* ptr, size_t size)
    : _data(ptr), _size(size) { }

    inline char* begin() const noexcept { return reinterpret_cast<char*>(_data); }
    inline char* end() const noexcept { return begin() + _size; }
    inline void* data() const noexcept { return _data; }
    inline size_t size() const noexcept { return _size; }

    template<size_t N>
    bool operator ==(const char (&str)[N]) const noexcept {
        return _size == (N - 1) and memcmp(_data, &str[0], N - 1) == 0;
    }

    template<size_t N>
    bool operator !=(const char (&str)[N]) const noexcept {
        return _size != (N - 1) or memcmp(_data, &str[0], N - 1);
    }
};

class SliceConst {
public:
    typedef const void* ValueType;
    typedef const char* iterator;

    const void* _data;
    size_t _size;

    SliceConst() : _data(nullptr), _size(0) { }

    SliceConst(const void* ptr, size_t size)
    : _data(ptr), _size(size) { }

    SliceConst(const SliceMutable& buf) // NOLINT(hicpp-explicit-conversions)
    : _data(buf.data()), _size(buf.size()) { }

    inline const char* begin() const noexcept { return reinterpret_cast<const char*>(_data); }
    inline const char* end() const noexcept { return begin() + _size; }
    inline const void* data() const noexcept { return _data; }
    inline size_t size() const noexcept { return _size; }

    template<size_t N>
    bool operator ==(const char (&str)[N]) const noexcept {
        return _size == (N - 1) and memcmp(_data, &str[0], N - 1) == 0;
    }

    template<size_t N>
    bool operator !=(const char (&str)[N]) const noexcept {
        return _size != (N - 1) or memcmp(_data, &str[0], N - 1);
    }
};

// struct iovec
// TODO winsock2 WSABUF
static_assert(sizeof(SliceConst) == (sizeof(void*) + sizeof(size_t)), "Incompatible with struct iovec");
static_assert(sizeof(SliceMutable) == (sizeof(void*) + sizeof(size_t)), "Incompatible with struct iovec");

typedef SliceMutable SliceRead;
typedef SliceConst SliceWrite;

} // namespace jinx

#endif
