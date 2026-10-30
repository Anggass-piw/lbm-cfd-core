cmake_minimum_required(VERSION 3.16)
project(lbm_cfd_core VERSION 0.1.0 LANGUAGES CXX)

option(LBM_BUILD_TESTS "Build the dependency-free CTest suite" ON)

add_library(lbm_core
    src/simulator.cpp
    src/vtk_writer.cpp
)
target_include_directories(lbm_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(lbm_core PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_features(lbm_core PUBLIC cxx_std_17)

if(MSVC)
    target_compile_options(lbm_core PRIVATE /W4)
else()
    target_compile_options(lbm_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(lbm_demo src/main.cpp)
target_link_libraries(lbm_demo PRIVATE lbm_core)
target_include_directories(lbm_demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

if(LBM_BUILD_TESTS)
    enable_testing()
    add_executable(lbm_tests tests/test_lbm.cpp)
    target_link_libraries(lbm_tests PRIVATE lbm_core)
    target_include_directories(lbm_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    add_test(NAME lbm_tests COMMAND lbm_tests)

    function(add_cli_rejection_test name expected_error)
        add_test(NAME ${name} COMMAND
            ${CMAKE_COMMAND}
            "-DDEMO=$<TARGET_FILE:lbm_demo>"
            "-DEXPECTED_ERROR=${expected_error}"
            "-DCLI_ARGS=${ARGN}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/expect_cli_rejection.cmake"
        )
    endfunction()

    add_cli_rejection_test(cli_rejects_negative_nx
        "nx must be a positive decimal integer that fits in size_t" -1)
    add_cli_rejection_test(cli_rejects_negative_ny
        "ny must be a positive decimal integer that fits in size_t" 1 -2)
    add_cli_rejection_test(cli_rejects_negative_nz
        "nz must be a positive decimal integer that fits in size_t" 1 1 -3)
    add_cli_rejection_test(cli_rejects_negative_steps
        "steps must be a positive decimal integer that fits in size_t" 1 1 1 -1)
    add_cli_rejection_test(cli_rejects_plus_sign
        "nx must be a positive decimal integer that fits in size_t" +1)
    add_cli_rejection_test(cli_rejects_leading_whitespace
        "nx must be a positive decimal integer that fits in size_t" " 1")
    add_cli_rejection_test(cli_rejects_trailing_characters
        "nx must be a positive decimal integer that fits in size_t" 1x)
    add_cli_rejection_test(cli_rejects_zero
        "nx must be a positive decimal integer that fits in size_t" 0)
    add_cli_rejection_test(cli_rejects_overflow
        "nx must be a positive decimal integer that fits in size_t" 18446744073709551616)
endif()
