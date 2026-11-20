# Filesystem Tests

add_executable(test_filesystem_basic
    test_filesystem_basic.c
)

target_link_libraries(test_filesystem_basic PRIVATE orbisjvm)

add_test(NAME filesystem_basic COMMAND test_filesystem_basic)

add_executable(test_filesystem_dir
    test_filesystem_dir.c
)

target_link_libraries(test_filesystem_dir PRIVATE orbisjvm)

add_test(NAME filesystem_dir COMMAND test_filesystem_dir)

add_executable(test_filesystem_path
    test_filesystem_path.c
)

target_link_libraries(test_filesystem_path PRIVATE orbisjvm)

add_test(NAME filesystem_path COMMAND test_filesystem_path)