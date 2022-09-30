find_package(PkgConfig REQUIRED)
pkg_check_modules(libevent_extra REQUIRED IMPORTED_TARGET libevent_extra)

add_library(jinx_evdns STATIC src/evdns.cpp)
add_library(jinx::evdns ALIAS jinx_evdns)
target_include_directories(jinx_evdns PUBLIC include)
target_link_libraries(jinx_evdns PUBLIC jinx PkgConfig::libevent_extra)

add_subdirectory(tests)
