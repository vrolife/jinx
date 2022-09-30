cmake_minimum_required(VERSION 3.13)
project(jinx C CXX)

include(CTest)
include(cmake/scan_tests.cmake)

option(JINX_BUILD_HTTP "build jinx::http" ON)
option(JINX_BUILD_CJSON "build jinx::cjson" ON)
option(JINX_BUILD_OPENSSL "build jinx::openssl" ON)
option(JINX_BUILD_EVDNS "build jinx::evdns" ON)

set(CMAKE_CXX_STANDARD 11)

if(ENABLE_SANITIZER)
    set(SANITIZER_FLAGS "-fno-omit-frame-pointer -fsanitize=address,undefined")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${SANITIZER_FLAGS}")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${SANITIZER_FLAGS}")
    set(CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} ${SANITIZER_FLAGS}")
endif(ENABLE_SANITIZER)

if(ENABLE_GCOV)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fprofile-arcs -ftest-coverage")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fprofile-arcs -ftest-coverage")
    set(CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fprofile-arcs -ftest-coverage -lgcov")
    add_custom_target(generate_coverage_report
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
        COMMAND /bin/sh "${CMAKE_SOURCE_DIR}/scripts/gen-cov.sh" "${CMAKE_C_COMPILER}" "${CMAKE_CXX_COMPILER}"
    )
endif(ENABLE_GCOV)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wreturn-type")

file(GLOB_RECURSE HEADERS LIST_DIRECTORIES false RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/ include/*.hpp)

set(JINX_SOURCES
    ${HEADERS}
    src/error.cpp
    src/async.cpp 
    src/libevent.cpp 
    src/posix.cpp 
    src/argparse.cpp 
    src/stream.cpp
)

add_library(jinx STATIC ${JINX_SOURCES})
target_include_directories(jinx PUBLIC include)
set_target_properties(jinx PROPERTIES PUBLIC_HEADER "${HEADERS}")

if(NOT LIBEVENT_FIND_PACKAGE)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(libevent REQUIRED IMPORTED_TARGET libevent_core)

    if(LIBEVENT_STATIC_LINK)
        target_include_directories(jinx PUBLIC ${libevent_INCLUDE_DIRS})
        find_library(libevent_STATIC_LIBRARY libevent_core.a)
        if (libevent_STATIC_LIBRARY)
            target_link_libraries(jinx PUBLIC ${libevent_STATIC_LIBRARY})
        else(libevent_STATIC_LIBRARY)
            message(STATUS "libevent static library file not found")
            target_link_libraries(jinx PUBLIC PkgConfig::libevent)
        endif (libevent_STATIC_LIBRARY)
    else(LIBEVENT_STATIC_LINK)
        target_link_libraries(jinx PUBLIC PkgConfig::libevent)
    endif(LIBEVENT_STATIC_LINK)

else(NOT LIBEVENT_FIND_PACKAGE)
    find_package(Libevent REQUIRED)
    target_link_libraries(jinx PUBLIC libevent::core)
endif(NOT LIBEVENT_FIND_PACKAGE)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(jinx PUBLIC -DJINX_DEBUG=1)
endif()

if(ENABLE_PRECOMPILE_HEADERS)
    if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.16.0")
        target_precompile_headers(jinx PUBLIC ${HEADERS})
    endif()
endif(ENABLE_PRECOMPILE_HEADERS)

if(NOT JINX_NO_INSTALL)
install(TARGETS jinx ARCHIVE PUBLIC_HEADER DESTINATION include/jinx)
endif(NOT JINX_NO_INSTALL)

if(NOT JINX_NO_TESTS)
    add_subdirectory(tests)
endif(NOT JINX_NO_TESTS)

if(NOT JINX_NO_LIBS)
    add_subdirectory(libs)
endif(NOT JINX_NO_LIBS)
