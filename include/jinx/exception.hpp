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
#ifndef __jinx_exception_hpp__
#define __jinx_exception_hpp__

#include <stdexcept>

#include <jinx/error.hpp>

namespace jinx {
namespace exception {

class JinxError : public std::exception {
    error::Error _code{};
public:
    JinxError() = default;
    explicit JinxError(const error::Error& code) : _code(code) { }
    
    const char* what() const noexcept override {
        return _code.message();
    }

    const error::Error& code() const { return _code; }
};

} // namespace exception
} // namespace jinx

#endif
