Cmake_minimum_required(VERSION 3.22)
project(BedrockClient VERSION 1.0.0 LANGUAGES CXX)

add_library(bedrock_client SHARED
    src/client.cpp
)

target_include_directories(bedrock_client
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(bedrock_client PUBLIC cxx_std_20)

target_compile_options(bedrock_client PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -fvisibility=hidden
)

set_target_properties(bedrock_client PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
    OUTPUT_NAME "bedrock_client"
)

if(ANDROID)
    target_link_libraries(bedrock_client PRIVATE log)
endif()