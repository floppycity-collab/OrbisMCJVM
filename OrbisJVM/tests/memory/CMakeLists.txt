# Memory Tests

add_executable(test_memory_basic
    test_memory_basic.c
)

target_link_libraries(test_memory_basic PRIVATE orbisjvm)

add_test(NAME memory_basic COMMAND test_memory_basic)

add_executable(test_memory_page_alloc
    test_memory_page_alloc.c
)

target_link_libraries(test_memory_page_alloc PRIVATE orbisjvm)

add_test(NAME memory_page_alloc COMMAND test_memory_page_alloc)

add_executable(test_memory_mmap
    test_memory_mmap.c
)

target_link_libraries(test_memory_mmap PRIVATE orbisjvm)

add_test(NAME memory_mmap COMMAND test_memory_mmap)

add_executable(test_memory_tracking
    test_memory_tracking.c
)

target_link_libraries(test_memory_tracking PRIVATE orbisjvm)

add_test(NAME memory_tracking COMMAND test_memory_tracking)