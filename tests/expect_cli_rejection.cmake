if(NOT DEFINED DEMO OR NOT DEFINED EXPECTED_ERROR OR NOT DEFINED CLI_ARGS)
    message(FATAL_ERROR "DEMO, EXPECTED_ERROR, and CLI_ARGS are required")
endif()

execute_process(
    COMMAND "${DEMO}" ${CLI_ARGS}
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error
    TIMEOUT 5
)

if(result STREQUAL "0")
    message(FATAL_ERROR "CLI unexpectedly accepted arguments: ${CLI_ARGS}")
endif()

set(combined_output "${output}${error}")
if(NOT combined_output MATCHES "${EXPECTED_ERROR}")
    message(FATAL_ERROR
        "CLI did not report the expected validation error.\n"
        "Expected: ${EXPECTED_ERROR}\n"
        "Exit status: ${result}\n"
        "Output: ${combined_output}"
    )
endif()
