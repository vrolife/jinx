
add_library(thirdparty_cJSON STATIC cJSON.c cJSON_Utils.c)
target_include_directories(thirdparty_cJSON PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
