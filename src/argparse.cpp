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
