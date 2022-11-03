add_subdirectory(thirdparty)

add_library(jinx_cjson INTERFACE)
add_library(jinx::cjson ALIAS jinx_cjson)
target_include_directories(jinx_cjson INTERFACE include)
target_link_libraries(jinx_cjson INTERFACE jinx thirdparty_cJSON)

add_subdirectory(tests)
