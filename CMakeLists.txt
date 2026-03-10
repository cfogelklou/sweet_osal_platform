cmake_minimum_required(VERSION 3.5)
project(sweet_osal_platform)

enable_testing()

if(NOT DEFINED SOP_TOP_DIR)
    set(SOP_TOP_DIR ${CMAKE_SOURCE_DIR})
endif()

if (TRUE)
  # skip
elseif(DEFINED ENV{GITHUB_WORKFLOW})
  # Skip
elseif (UNIX OR LINUX OR MINGW OR APPLE)
    list(APPEND CMAKE_MODULE_PATH "${SOP_TOP_DIR}/sop_src/cmake/")
    include(CodeCoverage)
    APPEND_COVERAGE_COMPILER_FLAGS()
    # COVERAGE_GCOVR_EXCLUDE_DIRS is a regex with directories to exclude from coverage report
    set(COVERAGE_GCOVR_EXCLUDES
            '${SOP_TOP_DIR}/ext'
            '${SOP_TOP_DIR}/sop_src/tests'
            '${SOP_TOP_DIR}/test'
    )

    # SETUP_TARGET_FOR_COVERAGE_GCOVR_HTML(
    #     NAME ctest_coverage                    # New target name
    #     EXECUTABLE ctest -j ${PROCESSOR_COUNT} # Executable in PROJECT_BINARY_DIR
    #     DEPENDENCIES executable_target         # Dependencies to build first
    SETUP_TARGET_FOR_COVERAGE_GCOVR_HTML(
            NAME ctest_coverage
            EXECUTABLE ctest -j 4
            DEPENDENCIES
                osal_test
    )
    SETUP_TARGET_FOR_COVERAGE_GCOVR_XML(
            NAME cobertura
    )
endif()

add_subdirectory(sop_src)
add_subdirectory(test)
add_subdirectory(sop_src/osal/test)
add_subdirectory(sop_src/utils/tests)
add_subdirectory(sop_src/simple_plot/tests)
add_subdirectory(sop_src/buf_io/tests)

# Add clang-format targets
add_custom_target(format
    COMMAND find ${CMAKE_SOURCE_DIR}/sop_src -name "*.cpp" -o -name "*.hpp" -o -name "*.h"
        | xargs clang-format -i
    COMMENT "Formatting code with clang-format"
)

# Add format-check target for CI
add_custom_target(format-check
    COMMAND find ${CMAKE_SOURCE_DIR}/sop_src -name "*.cpp" -o -name "*.hpp" -o -name "*.h"
        | xargs clang-format --dry-run --Werror
    COMMENT "Checking code format"
)

