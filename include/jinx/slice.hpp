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
