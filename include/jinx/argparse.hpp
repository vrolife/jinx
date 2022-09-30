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
#ifndef __jinx_argparse_hpp__
#define __jinx_argparse_hpp__

#include <stdexcept>
#include <sstream>
#include <array>
#include <iomanip>
#include <iostream>
#include <unordered_map>
#include <regex>

#include <jinx/assert.hpp>
#include <jinx/record.hpp>

namespace jinx {
namespace argparse {
using namespace jinx::record;

enum ArgumentType {
    TypeInvalid,
    TypeSize,
    TypeFloat,
    TypeBool,
    // TypeTime,
    TypeInteger,
    TypeString,
    // TypeAddress,
    TypeStringList
};

class Argument {
    std::string _name;
    std::string _desc;
    ArgumentType _type;
    bool _secret{false};

    size_t _size_default{0};
    size_t _size_min{0};
    size_t _size_max{0};

    int _int_default{0};
    int _int_min{0};
    int _int_max{0};

    float _float_default{0};
    float _float_min{0};
    float _float_max{0};

    std::string _string_default{};
    size_t _string_min{0};
    size_t _string_max{0};

    bool _bool_default{false};

    std::vector<std::string> _string_list_default{};
    size_t _string_list_min{0};
    size_t _string_list_max{0};

public:
    Argument(const std::string& name, size_t default_value, size_t min, size_t max, const std::string& desc, bool secret=false)
    : _name(name), _type(TypeSize), _size_default(default_value), _size_min(min), _size_max(max), _desc(desc), _secret(secret)
    { }

    Argument(const std::string& name, float default_value, float min, float max, const std::string& desc, bool secret=false)
    : _name(name), _type(TypeFloat), _float_default(default_value), _float_min(min), _float_max(max), _desc(desc), _secret(secret)
    { }

    Argument(const std::string& name, int default_value, int min, int max, const std::string& desc, bool secret=false)
    : _name(name), _type(TypeInteger), _int_default(default_value), _int_min(min), _int_max(max), _desc(desc), _secret(secret)
    { }

    Argument(const std::string& name, const std::string& default_value, size_t min, size_t max, const std::string& desc, bool secret=false)
    : _name(name), _type(TypeString), _string_default(default_value), _string_min(min), _string_max(max), _desc(desc), _secret(secret)
    { }
    
    Argument(const std::string& name, bool default_value, const std::string& desc, bool secret=false)
    : _name(name), _type(TypeBool), _bool_default(default_value), _desc(desc), _secret(secret)
    { }

    Argument(const std::string& name, std::vector<std::string>&& default_value, size_t min, size_t max, const std::string& desc, bool secret=false)
    : _name(name), _type(TypeStringList), _string_list_default(default_value), _string_min(min), _string_max(max), _desc(desc), _secret(secret)
    { }

    const std::string& get_name() const { return _name; }
    const std::string& get_description() const { return _desc; }

    void validate(RecordCategory& category) const;
    void apply(RecordCategory& category, const std::string& value) const;

private:
    size_t get_default(size_t* ignore) const {
        return _size_default;
    }

    int get_default(int* ignore) const {
        return _int_default;
    }

    bool get_default(bool* ignore) const {
        return _bool_default;
    }

    float get_default(float* ignore) const {
        return _float_default;
    }

    const std::string& get_default(std::string* ignore) const {
        return _string_default;
    }

    std::vector<std::string> get_default(std::vector<std::string>* ignore) const {
        return _string_list_default;
    }

    template<typename O, typename T, typename V>
    inline
    void _commit(RecordCategory& category, V&& value) const {
        if (_secret) {
            auto* record = category.create<RecordSecret<O>>(_name, _desc, this->get_default(reinterpret_cast<typename std::decay<T>::type*>(0)));
            record->commit(std::forward<V>(value));
        } else {
            auto* record = category.create<RecordImmediate<O>>(_name, _desc, this->get_default(reinterpret_cast<typename std::decay<T>::type*>(0)));
            record->commit(std::forward<V>(value));
        }
    }

    template<typename O, typename T>
    typename std::enable_if<std::is_arithmetic<O>::value, void>::
    type _apply(RecordCategory& category, T&& value) const {
        _commit<O, O>(category, std::forward<T>(value));
    }

    template<typename O, typename T>
    typename std::enable_if<!std::is_arithmetic<O>::value, void>::
    type _apply(RecordCategory& category, T&& value) const {
        _commit<O, const O&>(category, std::forward<T>(value));
    }

    template<typename O, typename T>
    inline
    const O& _get(RecordCategory& category) const {
        if (_secret) {
            auto* record = category.create<RecordSecret<O>>(_name, _desc, this->get_default(reinterpret_cast<typename std::decay<T>::type*>(0)));
            return record->get_value();
        }
    
        auto* record = category.create<RecordImmediate<O>>(_name, _desc, this->get_default(reinterpret_cast<typename std::decay<T>::type*>(0)));
        return record->get_value();
    }

    template<typename O>
    typename std::enable_if<std::is_arithmetic<O>::value, const O&>::
    type _get(RecordCategory& category) const {
        return _get<O, O>(category);
    }

    template<typename O>
    typename std::enable_if<!std::is_arithmetic<O>::value, const O&>::
    type _get(RecordCategory& category) const {
        return _get<O, const O&>(category);
    }

    template<typename O>
    void _check_range(const std::string& name, O val, O min, O max) const
    {
        if (val < min or val > max) {
            std::ostringstream oss;
            oss << "the value of " << name << " is " << val << ". min is " << min << ". max is " << max;
            throw std::out_of_range(oss.str());
        }
    }
};

void print_help(const Argument* arguments, size_t count, std::ostream& error_stream=std::cerr);

void apply_options(
    std::unordered_map<std::string, std::string>& dict, 
    RecordCategory& category, 
    const Argument* arguments, size_t count,
    std::ostream& error_stream=std::cerr);

void parse_argv(
    int argc, const char* argv[], 
    RecordCategory& category, 
    const Argument* arguments, size_t count,
    std::ostream& error_stream=std::cerr);

} // namespace argparse
} // namespace jinx

#endif
