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
#include <algorithm>

#include <jinx/argparse.hpp>

namespace jinx {
namespace argparse {

void Argument::validate(RecordCategory& category) const 
{
    switch(_type) {
        case TypeSize:
        {
            auto size = _get<size_t>(category);
            _check_range(_name, size, _size_min, _size_max);
            return;
        }
        case TypeInteger:
        {
            auto size = _get<int>(category);
            _check_range(_name, size, _int_min, _int_max);
            return;
        }
        case TypeFloat:
        {
            auto size = _get<float>(category);
            _check_range(_name, size, _float_min, _float_max);
            return;
        }
        case TypeString:
        {
            auto str = _get<std::string>(category);
            if (str.size() < _string_min or str.size() > _string_max) {
                std::ostringstream oss;
                oss << "the size of string " << _name << "='" << str << "' out of range. (" << _string_min << " <= size <= " << _string_max << ")";
                throw std::out_of_range(oss.str());
            }
            return;
        }
        case TypeBool:
        {
            _get<bool>(category);
            return;
        }
        case TypeStringList:
        {
            auto str = _get<std::string>(category);
            if (str.size() < _string_min or str.size() > _string_max) {
                std::ostringstream oss;
                oss << "the size of list " << _name << "='" << str << "' out of range. (" << _string_min << " <= size <= " << _string_max << ")";
                throw std::out_of_range(oss.str());
            }
            return;
        }
        case TypeInvalid:
            jinx_assert(0);
            return;
    }
}

void Argument::apply(RecordCategory& category, const std::string& value) const
{
    switch(_type) {
        case TypeSize:
            _apply<size_t>(category, std::stoul(value));
            return;
        case TypeInteger:
            _apply<int>(category, std::stoul(value));
            return;
        case TypeFloat:
            _apply<float>(category, std::stof(value));
            return;
        case TypeString:
            _apply<std::string>(category, value);
            return;
        case TypeBool:
            _apply<bool>(category, 
                value == "true" or value == "True" or value == "TRUE" or 
                value == "yes" or value == "Yes" or value == "YES" or
                value == "ok" or value == "positive");
            return;
        case TypeStringList:
            _apply<std::vector<std::string>>(category, value);
            return;
        case TypeInvalid:
            jinx_assert(0);
            return;
    }
}

void print_help(const Argument* arguments, size_t count, std::ostream& error_stream)
{
    size_t width = 0;
    std::for_each(arguments, arguments + count, [&](const Argument& arg){
        if (arg.get_name().size() > width) {
            width = arg.get_name().size();
        }
    });
    
    width += 10;

    std::for_each(arguments, arguments + count, [&](const Argument& arg){
        error_stream << std::setw(width) << std::left << (std::string("--") + arg.get_name() + "=...") << arg.get_description() << std::endl;
    });
}

void apply_options(
    std::unordered_map<std::string, std::string>& dict, 
    RecordCategory& category, 
    const Argument* arguments, size_t count,
    std::ostream& error_stream)
{
    std::for_each(arguments, arguments + count, [&](const Argument& arg){
        auto iter = dict.find(arg.get_name());
        if (iter != dict.end()) {
            arg.apply(category, iter->second);
            dict.erase(iter);
        }
        arg.validate(category);
    });

    if (not dict.empty()) {
        for (auto& pair : dict) {
            std::cerr << "unknown option --" << pair.first << "=" << pair.second << std::endl;
        }
        throw std::invalid_argument("unknown option");
    }
}

void parse_argv(
    int argc, const char* argv[], 
    RecordCategory& category, 
    const Argument* arguments, size_t count,
    std::ostream& error_stream)
{
    std::unordered_map<std::string, const Argument*> args{};
    std::for_each(arguments, arguments + count, [&](const Argument& arg){
        args.emplace(arg.get_name(), &arg);
    });

    std::regex regexp{"--([a-zA-Z0-9-]+)=(.+)"};
    for(int i = 1; i < argc; ++i) {
        std::smatch match;
        std::string string{argv[i]};
        if (not std::regex_match(string, match, regexp)) {
            std::ostringstream oss;
            oss << "invalid option: " << string;
            auto msg = oss.str();
            std::cerr << msg << std::endl;
            print_help(arguments, count, error_stream);
            throw std::invalid_argument(msg);
        }

        auto key = match[1].str();
        auto value = match[2].str();

        auto iter = args.find(match[1].str());
        if (iter == args.end()) {
            std::cerr << "unknown option --" << key << "=" << value<< std::endl;
            throw std::invalid_argument("unknown option");
        }
        iter->second->apply(category, value);
    }

    for (auto& pair : args) {
        pair.second->validate(category);
    }
}

} // namespace jinx
} // namespace argparse
