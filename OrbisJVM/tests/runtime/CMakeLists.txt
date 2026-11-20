# Runtime Tests

add_executable(test_runtime_init
    test_runtime_init.c
)

target_link_libraries(test_runtime_init PRIVATE orbisjvm)

add_test(NAME runtime_init COMMAND test_runtime_init)

add_executable(test_runtime_config
    test_runtime_config.c
)

target_link_libraries(test_runtime_config PRIVATE orbisjvm)

add_test(NAME runtime_config COMMAND test_runtime_config)

add_executable(test_runtime_diagnostics
    test_runtime_diagnostics.c
)

target_link_libraries(test_runtime_diagnostics PRIVATE orbisjvm)

add_test(NAME runtime_diagnostics COMMAND test_runtime_diagnostics)