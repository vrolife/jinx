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
#ifndef __jinx_logging_hpp__
#define __jinx_logging_hpp__

#include <cerrno>
#include <iostream>

#ifndef __FILE_NAME__
#define __FILE_NAME__ __FILE__
#endif

#define jinx_log_error() (std::cerr << __FILE_NAME__ << ":" << __LINE__ << ": " << __func__ << "(...) error: ")
#define jinx_log_info() (std::cerr << __FILE_NAME__ << ":" << __LINE__ << ": " << __func__ << "(...) info: ")
#define jinx_log_warning() (std::cerr << __FILE_NAME__ << ":" << __LINE__ << ": " << __func__ << "(...) warning: ")

#endif
