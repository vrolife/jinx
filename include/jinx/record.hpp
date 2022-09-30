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
#ifndef __jinx_record_hpp__
#define __jinx_record_hpp__

#include <iomanip>
#include <sstream>
#include <map>
#include <vector>

#include <jinx/macros.hpp>
#include <jinx/hash.hpp>

#define JINX_RECORD_VISIT_BASIC(t) virtual bool visit(const std::string& name, t value) { return false; }
#define JINX_RECORD_VISIT_ARRAY(t) virtual bool visit(const std::string& name, t value, size_t n) { return false; }

#define JINX_VISIT_TYPES(W, F) \
    F(W(bool)) \
    F(W(char)) \
    F(W(short)) \
    F(W(int)) \
    F(W(long)) \
    F(W(unsigned char)) \
    F(W(unsigned short)) \
    F(W(unsigned int)) \
    F(W(unsigned long)) \
    F(W(float)) \
    F(W(double)) \
    F(W(const std::string&))

#define JINX_RECORD_TYPE_BASIC(t) t
#define JINX_RECORD_TYPE_ARRAY(t) std::decay<t>::type*

namespace jinx {
namespace record {

struct RecordVisitor
{
    JINX_VISIT_TYPES(JINX_RECORD_TYPE_BASIC, JINX_RECORD_VISIT_BASIC)
    JINX_VISIT_TYPES(JINX_RECORD_TYPE_ARRAY, JINX_RECORD_VISIT_ARRAY)

    virtual bool visit(const std::string& name, const void* typd_id, void* data) {
        return false;
    }
};

template<typename T>
struct RecordVisitorGetValue : public RecordVisitor {
    typename std::decay<T>::type _value{};
    bool visit(const std::string& name, T value) override {
        _value = value;
        return true;
    }
};

class RecordAbstract
{
    std::string _name;
    std::string _description;

public:
    explicit RecordAbstract(const std::string& name) : _name(name) { }
    JINX_NO_COPY_NO_MOVE(RecordAbstract);
    virtual ~RecordAbstract() = default;

    virtual const void* get_type_id() = 0;
    virtual bool visit(RecordVisitor& visitor) = 0;

    const std::string& get_name() const { return _name; }
    const std::string& get_description() const { return _description; }
    void set_description(const std::string& desc) { _description = desc; }
};

template<typename T>
class RecordImmediate : public RecordAbstract
{
    T _value;

public:
    RecordImmediate(const std::string& name, const T& value)
    : RecordAbstract(name), _value(value) { }

    explicit RecordImmediate(const std::string& name)
    : RecordAbstract(name), _value{} { }

    const void* get_type_id() override {
        return reinterpret_cast<void*>(&type_id);
    }

    bool visit(RecordVisitor& visitor) override {
        return visitor.visit(get_name(), _value);
    }

    void commit(const T& value) {
        _value = value;
    }

    const T& get_value() const {
        return _value;
    }

    void* get_data() { return &_value; }

    static T* value_from_data(void* data) { return reinterpret_cast<T*>(data); }
    static void* type_id() { return reinterpret_cast<void*>(&type_id); }
};

template<typename T>
class RecordImmediate<std::vector<T>> : public RecordAbstract
{
    std::vector<T> _value;

public:
    RecordImmediate(const std::string& name, const std::vector<T>& value)
    : RecordAbstract(name), _value(value) { }

    explicit RecordImmediate(const std::string& name)
    : RecordAbstract(name), _value{} { }

    const void* get_type_id() override {
        return reinterpret_cast<void*>(&type_id);
    }

    bool visit(RecordVisitor& visitor) override {
        return visitor.visit(get_name(), _value.data(), _value.size());
    }

    void commit(const T& value) {
        _value.push_back(value);
    }

    const T& get_value() const {
        return _value;
    }

    void* get_data() { return &_value; }

    static T* value_from_data(void* data) { return reinterpret_cast<T*>(data); }
    static void* type_id() { return reinterpret_cast<void*>(&type_id); }
};

template<typename T>
class RecordSecret : public RecordAbstract
{
    T _value;

public:
    static void* type_id() { return reinterpret_cast<void*>(&type_id); }

    RecordSecret(const std::string& name, const T& value)
    : RecordAbstract(name), _value(value) { }

    explicit RecordSecret(const std::string& name)
    : RecordAbstract(name), _value{} { }

    const void* get_type_id() override {
        return reinterpret_cast<void*>(&type_id);
    }

    bool visit(RecordVisitor& visitor) override {
        return false;
    }

    void commit(const T& value) {
        _value = value;
    }

    const T& get_value() const {
        return _value;
    }
};

template<typename T>
class RecordSecret<std::vector<T>> : public RecordAbstract
{
    std::vector<T> _value;

public:
    static void* type_id() { return reinterpret_cast<void*>(&type_id); }

    RecordSecret(const std::string& name, const std::vector<T>& value)
    : RecordAbstract(name), _value(value) { }

    explicit RecordSecret(const std::string& name)
    : RecordAbstract(name), _value{} { }

    const void* get_type_id() override {
        return reinterpret_cast<void*>(&type_id);
    }

    bool visit(RecordVisitor& visitor) override {
        return false;
    }

    void commit(const T& value) {
        _value.push_back(value);
    }

    const T& get_value() const {
        return _value;
    }
};

template<typename T>
class RecordSum : public RecordAbstract
{
    T _value;

public:
    static void* type_id() { return reinterpret_cast<void*>(&type_id); }

    explicit RecordSum(const std::string& name, T&& value)
    : RecordAbstract(name), _value(value) { }

    const void* get_type_id() override {
        return reinterpret_cast<void*>(&type_id);
    }

    bool visit(RecordVisitor& visitor) override {
        return visitor.visit(get_name(), _value);
    }

    void commit(T value) {
        _value += value;
    }
    
    const T& get_value() const {
        return _value;
    }
};

template<typename T>
class RecordHarmonicMean : public RecordAbstract
{
    T _value;
    size_t _n;
public:
    static void* type_id() { return reinterpret_cast<void*>(&type_id); }

    explicit RecordHarmonicMean(const std::string& name, T&& value)
    : RecordAbstract(name), _value(value), _n(0) { }

    const void* get_type_id() override {
        return reinterpret_cast<void*>(&type_id);
    }

    bool visit(RecordVisitor& visitor) override {
        double val = static_cast<double>(_n) / static_cast<double>(_value);
        return visitor.visit(get_name(), static_cast<T>(val));
    }

    void commit(T value) {
        _n += 1;
        _value += 1.0F / value;
    }

    const T& get_value() const {
        return _value;
    }
};

class RecordCategory;

struct RecordCategoryVisitor
{
    virtual bool visit(const std::string& name, RecordCategory&) = 0;
};

class RecordCategory
{
public:
    typedef size_t HashValueType;

private:
    std::map<HashValueType, RecordAbstract*> _records;

public:
    RecordCategory() = default;
    RecordCategory(RecordCategory&&) = default;
    RecordCategory(const RecordCategory&) = delete;
    RecordCategory& operator =(RecordCategory&&) = default;
    RecordCategory& operator =(const RecordCategory&) = delete;
    ~RecordCategory() {
        for (auto& record : _records) {
            delete record.second;
        }
    }

    bool insert(RecordAbstract* record) {
        const auto& name = record->get_name();
        auto hash = jinx::hash::hash_string(name.c_str());
        auto iter = _records.find(hash);
        if (iter != _records.end()) {
            if (JINX_UNLIKELY(record->get_type_id() != iter->second->get_type_id())) {
                std::ostringstream oss;
                oss << "record \"" << name << "\" type conflict";
                throw std::invalid_argument(oss.str());
            }
            return false;
        }

        _records.emplace(hash, record);
        return true;
    }

    template<typename R, typename... Args>
    R* get(const std::string& name, Args&&... args) {
        auto hash = jinx::hash::hash_string(name.c_str());
        auto iter = _records.find(hash);
        if (iter != _records.end()) {
            if (JINX_UNLIKELY(R::type_id() != iter->second->get_type_id())) {
                std::ostringstream oss;
                oss << "record \"" << name << "\" type conflict";
                throw std::invalid_argument(oss.str());
            }
            return static_cast<R*>(iter->second);
        }

        auto* ptr = new R(name, std::forward<Args>(args)...);
        _records.emplace(hash, ptr);
        return ptr;
    }

    template<typename R>
    R* check(const std::string& name) {
        auto hash = jinx::hash::hash_string(name.c_str());
        auto iter = _records.find(hash);
        if (iter != _records.end()) {
            if (JINX_UNLIKELY(R::type_id() != iter->second->get_type_id())) {
                std::ostringstream oss;
                oss << "record \"" << name << "\" type conflict";
                throw std::invalid_argument(oss.str());
            }
            return static_cast<R*>(iter->second);
        }

        std::ostringstream oss;
        oss << "record \"" << name << "\" does not exists";
        throw std::invalid_argument(oss.str());
    }

    bool exists(const std::string& name) noexcept {
        auto hash = jinx::hash::hash_string(name.c_str());
        auto iter = _records.find(hash);
        return iter != _records.end();
    }

    template<typename R, typename... Args>
    R* create(const std::string& name, const std::string& desc, Args&&... args) {
        R* record = this->get<R>(name, std::forward<Args>(args)...);
        record->set_description(desc);
        return record;
    }

    bool visit(const std::string& name, RecordVisitor& visitor) {
        auto hash = jinx::hash::hash_string(name.c_str());
        auto iter = _records.find(hash);
        if (iter == _records.end()) {
            return false;
        }
        return iter->second->visit(visitor);
    }

    bool visit(HashValueType hash, RecordVisitor& visitor) {
        auto iter = _records.find(hash);
        if (iter == _records.end()) {
            return false;
        }
        return iter->second->visit(visitor);
    }

    bool foreach(RecordVisitor& visitor) {
        bool result = false;
        for (auto& record : _records) {
            result |= record.second->visit(visitor);
        }
        return result;
    }
};

template<size_t PrintVectorMax=16>
class RecordVisitorPrint : public RecordVisitor {
    std::ostream& _output_stream;

    template<typename T>
    void print_array(T value, size_t size) {
        _output_stream << "[";
        int idx = 0;
        for (; idx < std::min(size, PrintVectorMax) ; ++idx) {
            if (JINX_LIKELY(idx != 0)) {
                _output_stream << ",";
            }
            _output_stream << value[idx];
        }

        if (idx < size) {
            _output_stream << "," << "...";
        }
        _output_stream << "]";
    }
public:
    explicit RecordVisitorPrint(std::ostream& output_stream) : _output_stream(output_stream) { }

#define JINX_RECORD_VISIT_BASIC_PRINNT(t) \
    bool visit(const std::string& name, t value) override { \
        _output_stream << "   " << std::setw(30) << std::left << name << ": " << value << std::endl; return true; \
    }

#define JINX_RECORD_VISIT_ARRAY_PRINNT(t) \
    bool visit(const std::string& name, t value, size_t count) override { \
        _output_stream << " " << name << ": "; print_array(value, count); _output_stream << std::endl; return true; \
    }
    
    JINX_VISIT_TYPES(JINX_RECORD_TYPE_BASIC, JINX_RECORD_VISIT_BASIC_PRINNT)
    JINX_VISIT_TYPES(JINX_RECORD_TYPE_ARRAY, JINX_RECORD_VISIT_ARRAY_PRINNT)
};

template<size_t PrintVectorMax=16>
class RecordCategoryVisitorPrint : public RecordCategoryVisitor
{
    std::ostream& _output_stream;
    RecordVisitorPrint<PrintVectorMax> _print;
public:
    explicit RecordCategoryVisitorPrint(std::ostream& output_stream) : _output_stream(output_stream), _print(output_stream) { }

    bool visit(const std::string& name, RecordCategory& stat) override {
        _output_stream << name << ": " << std::endl;
        return stat.foreach(_print);
    }
};

} // namespace record
} // namespace jinx

#endif
