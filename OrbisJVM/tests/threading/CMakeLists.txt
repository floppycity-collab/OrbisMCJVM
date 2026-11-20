# Threading Tests

add_executable(test_thread_basic
    test_thread_basic.c
)

target_link_libraries(test_thread_basic PRIVATE orbisjvm)

add_test(NAME thread_basic COMMAND test_thread_basic)

add_executable(test_mutex
    test_mutex.c
)

target_link_libraries(test_mutex PRIVATE orbisjvm)

add_test(NAME mutex COMMAND test_mutex)

add_executable(test_condvar
    test_condvar.c
)

target_link_libraries(test_condvar PRIVATE orbisjvm)

add_test(NAME condvar COMMAND test_condvar)

add_executable(test_tls
    test_tls.c
)

target_link_libraries(test_tls PRIVATE orbisjvm)

add_test(NAME tls COMMAND test_tls)