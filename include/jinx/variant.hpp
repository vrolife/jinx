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
#ifndef __jinx_variant_hpp__
#define __jinx_variant_hpp__

#include <cstddef>
#include <type_traits>
#include <typeinfo>
#include <utility>
#include <array>

#include <jinx/assert.hpp>
#include <jinx/meta.hpp>
#include <jinx/result.hpp>
#include <jinx/macros.hpp>

namespace jinx {
namespace detail {

template<typename S, typename T, typename... Ts>
struct __lookup_table;

template<typename S, typename T>
struct alignas(void*) __lookup_table<S, T> {
    alignas(void*)
    typename S::Signature _fun__missing_TypeList = &S::template apply<T>;
};

template<typename S, typename T, typename... Ts>
struct alignas(void*) __lookup_table {
    alignas(void*)
    typename S::Signature _fun__missing_TypeList = &S::template apply<T>;

    alignas(void*)
    __lookup_table<S, Ts...> _next;
};

template<typename S, typename... TypeList>
inline
typename S::Signature __lookup_table_apply(const int type) {
    static const __lookup_table<S, TypeList...> table{};
    static_assert(sizeof(table) == sizeof...(TypeList) * sizeof(typename S::Signature), "incompatible lookup table");
    jinx_assert(table._fun__missing_TypeList);
    return reinterpret_cast<typename S::Signature const*>(&table)[type];
}

template<typename F>
struct __variant_stub_call {
    typedef void(*Signature)(void*, F& fun);
    template<typename T>
    static void apply(void* mem, F& fun) {
        fun(*reinterpret_cast<T*>(mem));
    }
};

struct __variant_stub_free {
    typedef void(*Signature)(void*);
    template<typename T>
    inline
    static void apply(void* mem) {
        reinterpret_cast<T*>(mem)->~T();
    }
};

struct __variant_stub_construct_copy {
    typedef void(*Signature)(void*, const void*);
    template<typename T>
    inline
    static void apply(void* mem, const void* other) {
        new(mem)T(*reinterpret_cast<const T*>(other));
    }
};

struct __variant_stub_construct_move {
    typedef void(*Signature)(void*, void*);
    template<typename T>
    inline
    static void apply(void* mem, void* other) {
        new(mem)T(std::move(*reinterpret_cast<T*>(other)));
    }
};

struct __variant_stub_compare {
    typedef bool(*Signature)(void*, const void*);
    template<typename T>
    inline
    static bool apply(void* mem, const void* other) {
        return *reinterpret_cast<T*>(mem) == *reinterpret_cast<const T*>(other);
    }
};

} // namespace detail

template<typename... Types>
class VariantBase
{
protected:
    alignas(Types...)
    char _memory[meta::union_size<Types...>::size]{};
#ifdef __JINX_DEBUG_VARIANT_VOLATILE_TYPE
    volatile
#endif
    int _type{-1};

public:

    bool empty() const {
       return this->_type == -1;
    }

    template<typename T>
    JINX_NO_DISCARD
    Result<T*> as_pointer() 
    {
        auto type = meta::index_of<T, Types...>::value;
        if (type != this->_type) {
            return nullptr;
        }
        return reinterpret_cast<T*>(&this->_memory[0]);
    }

    template<typename T>
    JINX_NO_DISCARD
    Result<const T*> as_pointer() const 
    {
        auto type = meta::index_of<T, Types...>::value;
        if (type != this->_type) {
            return nullptr;
        }
        return reinterpret_cast<const T*>(&this->_memory[0]);
    }

    int get_type() const {
        return this->_type;
    }

    template<typename T>
    struct TypeIndex : public meta::index_of<T, Types...> { };
};

template<typename... Types>
class VariantRecursive : public VariantBase<Types...>
{
    // free
    template<typename T, typename... TS>
    inline
    void _free(const int vtype) {
        auto type = meta::index_of<T, Types...>::value;
        /* 
            Use JINX_UNLIKELY to generate parallel branch instruction
            May be slower on MCU/old processor
        */
        if (JINX_UNLIKELY(type == vtype)) {
            reinterpret_cast<T*>(&this->_memory[0])->~T();
            return;
        }
        return _free<TS...>(vtype);
    }

    template<typename... TS>
    inline
    typename std::enable_if<sizeof...(TS) == 0>::
    type _free(const int vtype) { }

    // copy
    template<typename T, typename... TS>
    inline
    void _construct_copy(const VariantRecursive& other) {
        auto type = meta::index_of<T, Types...>::value;
        if (JINX_UNLIKELY(type == other._type)) {
            new(this->_memory)T(*reinterpret_cast<const T*>(other._memory));
            this->_type = other._type;
            return;
        }
        return _construct_copy<TS...>(other);
    }

    template<typename... TS>
    inline
    typename std::enable_if<sizeof...(TS) == 0>::
    type _construct_copy(const VariantRecursive& other) { }

    // move
    template<typename T, typename... TS>
    inline
    void _construct_move(VariantRecursive&& other) {
        auto type = meta::index_of<T, Types...>::value;
        if (JINX_UNLIKELY(type == other._type)) {
            new(this->_memory)T(std::move(*reinterpret_cast<T*>(other._memory)));
            this->_type = other._type;
            other._type = -1;
            return;
        }
        return _construct_move<TS...>(std::move(other));
    }

    template<typename... TS>
    inline
    typename std::enable_if<sizeof...(TS) == 0>::
    type _construct_move(VariantRecursive&& other) { }

    // compare
    template<typename T, typename... TS>
    inline
    bool _compare(const VariantRecursive& other) {
        auto type = meta::index_of<T, Types...>::value;
        if (JINX_UNLIKELY(type == other._type)) {
            return *reinterpret_cast<T*>(this->_memory) == *reinterpret_cast<const T*>(other._memory);
        }
        return _compare<TS...>(other);
    }

    template<typename... TS>
    inline
    typename std::enable_if<sizeof...(TS) == 0, bool>::
    type _compare(const VariantRecursive& other)
    { 
        return false;
    }

    // call
    template<typename F, typename T, typename... TS>
    inline
    void _call(F&& fun, const int vtype) {
        auto type = meta::index_of<T, Types...>::value;
        if (JINX_UNLIKELY(type == vtype)) {
            fun(*reinterpret_cast<T*>(this->_memory));
        }
        return _call<F, TS...>(std::forward<F>(fun), vtype);
    }

    template<typename F, typename... TS>
    inline
    typename std::enable_if<sizeof...(TS) == 0>::
    type _call(F&& fun, const int vtype) { }

public:
    VariantRecursive() = default;

    template<typename T>
    explicit VariantRecursive(T&& value) {
        this->template emplace<T>(std::forward<T>(value));
    }

    VariantRecursive(const VariantRecursive& other) {
        if (other.empty()) {
            this->_type = -1;
            return;
        }
        _construct_copy<Types...>(other);
    }

    VariantRecursive(VariantRecursive&& other)  noexcept {
        if (other.empty()) {
            this->_type = -1;
            return;
        }
        _construct_move<Types...>(std::move(other));
    }

    ~VariantRecursive() { reset(); }

    VariantRecursive& operator =(const VariantRecursive& other) {
        reset();
        if (not other.empty()) {
            _construct_copy<Types...>(other);
        }
        return *this;
    }

    VariantRecursive& operator =(VariantRecursive&& other)  noexcept {
        reset();
        if (not other.empty()) {
            _construct_move<Types...>(std::move(other));
        }
        return *this;
    }

    bool operator ==(const VariantRecursive& other) {
        return this->_type == other._type and this->_type == -1 or _compare<Types...>(other);
    }

    bool operator !=(const VariantRecursive& other) {
        return !(*this == other);
    }

    template<typename F>
    JINX_NO_DISCARD
    ResultBool operator()(F&& fun) {
        if (this->empty()) {
            return false;
        }
        _call<F, Types...>(std::forward<F>(fun), this->_type);

        return true;
    }

    template<typename T, typename... Args>
    T& emplace(Args&&... args) {
        constexpr int type = meta::index_of<T, Types...>::value;
        static_assert(type != -1, "");
        this->reset();
        this->_type = type;
        new(this->_memory)T(std::forward<Args>(args)...);
        return *reinterpret_cast<T*>(&this->_memory[0]);
    }

    void reset() noexcept {
        if (this->_type != -1) {
            _free<Types...>(this->_type);
            this->_type = -1;
        }
    }
};

template<typename... Types>
class VariantLookupTable : public VariantBase<Types...>
{
public:
    VariantLookupTable() = default;

    template<typename T>
    explicit VariantLookupTable(T&& value) {
        this->template emplace<T>(std::forward<T>(value));
    }

    VariantLookupTable(const VariantLookupTable& other) {
        if (other.empty()) {
            this->_type = -1;
            return;
        }
        this->_type = other._type;
        detail::__lookup_table_apply<detail::__variant_stub_construct_copy, Types...>(this->_type)(this->_memory, other._memory);
    }

    VariantLookupTable(VariantLookupTable&& other)  noexcept {
        if (other.empty()) {
            this->_type = -1;
            return;
        }
        this->_type = other._type;
        detail::__lookup_table_apply<detail::__variant_stub_construct_move, Types...>(this->_type)(this->_memory, other._memory);
        other._type = -1;
    }

    ~VariantLookupTable() { reset(); }

    VariantLookupTable& operator =(const VariantLookupTable& other) {
        reset();
        if (not other.empty()) {
            this->_type = other._type;
            detail::__lookup_table_apply<detail::__variant_stub_construct_copy, Types...>(this->_type)(this->_memory, other._memory);
        }
        return *this;
    }

    VariantLookupTable& operator =(VariantLookupTable&& other)  noexcept {
       reset();
        if (not other.empty()) {
            this->_type = other._type;
            detail::__lookup_table_apply<detail::__variant_stub_construct_move, Types...>(this->_type)(this->_memory, other._memory);
            other._type = -1;
        }
        return *this;
    }

    bool operator ==(const VariantLookupTable& other) {
        return this->_type == other._type and 
            (this->_type == -1 or detail::__lookup_table_apply<detail::__variant_stub_compare, Types...>(this->_type)(this->_memory, other._memory));
    }

    bool operator !=(const VariantLookupTable& other) {
        return !(*this == other);
    }

    template<typename F>
    JINX_NO_DISCARD
    ResultBool operator()(F&& fun) {
        if (this->empty()) {
            return false;
        }
        detail::__lookup_table_apply<detail::__variant_stub_call<F>, Types...>(this->_type)(this->_memory, fun);

        return true;
    }

    template<typename T, typename... Args>
    T& emplace(Args&&... args) {
        constexpr int type = meta::index_of<T, Types...>::value;
        static_assert(type != -1, "");
        this->reset();
        this->_type = type;
        new(this->_memory)T(std::forward<Args>(args)...);
        return *reinterpret_cast<T*>(&this->_memory[0]);
    }

    void reset() {
       if (this->_type != -1) {
            detail::__lookup_table_apply<detail::__variant_stub_free, Types...>(this->_type)(this->_memory);
            this->_type = -1;
       }
    }
};

/*
    // see docs/variant_benchmark
*/
#ifndef __JINX_VARIANT_SWITCH
#define __JINX_VARIANT_SWITCH 5
#endif

#if defined(JINX_USE_VARIANT_LOOKUP_TABLE) && JINX_USE_VARIANT_LOOKUP_TABLE

template<typename... TypeList>
using Variant = VariantLookupTable<TypeList...>;

#elif defined(JINX_USE_VARIANT_RECURSIVE) && PLAYLANG_USE_VARIANT_RECURSIVE

template<typename... TypeList>
using Variant = VariantRecursive<TypeList...>;

#else

template<typename... TypeList>
using Variant = typename std::conditional<
    sizeof...(TypeList) <= __JINX_VARIANT_SWITCH, 
        VariantRecursive<TypeList...>, 
        VariantLookupTable<TypeList...>>::type;
#endif
}

#endif
