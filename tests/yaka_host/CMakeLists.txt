cmake_minimum_required(VERSION 3.20)
project(hi_yaka_product_core_tests LANGUAGES CXX)

enable_testing()

add_executable(hi_yaka_product_core_test
    product_core_test.cc
    ../../main/yaka/capability_registry.cc
    ../../main/yaka/product_controller.cc
)

target_include_directories(hi_yaka_product_core_test PRIVATE ../../main/yaka)
target_compile_features(hi_yaka_product_core_test PRIVATE cxx_std_17)

if(MSVC)
    target_compile_options(hi_yaka_product_core_test PRIVATE /W4 /WX)
else()
    target_compile_options(hi_yaka_product_core_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME hi_yaka_product_core COMMAND hi_yaka_product_core_test)
