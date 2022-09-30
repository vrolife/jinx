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
