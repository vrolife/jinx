
add_library(jinx_http STATIC http.cpp)
add_library(jinx::http ALIAS jinx_http)
target_include_directories(jinx_http PUBLIC include)
target_link_libraries(jinx_http PUBLIC jinx)

if(NOT JINX_NO_TESTS)
    add_subdirectory(tests)
endif(NOT JINX_NO_TESTS)

if(NOT JINX_NO_EXAMPLES)
    add_subdirectory(examples)
endif(NOT JINX_NO_EXAMPLES)
