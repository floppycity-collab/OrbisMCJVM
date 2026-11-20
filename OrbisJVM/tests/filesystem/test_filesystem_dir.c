/**
 * @file test_filesystem_dir.c
 * @brief Directory operation tests
 */

#include <orbisjvm/filesystem.h>
#include <orbisjvm/diagnostics.h>
#include <stdio.h>
#include <string.h>

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            orbisjvm_log_error(ORBISJVM_LOG_CAT_GENERAL, "ASSERT FAILED: %s: %s", #cond, msg); \
            return -1; \
        } \
    } while (0)

int test_dir_create_remove(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Testing directory create/remove...");

    const char* test_dir = "/tmp/orbisjvm_test_dir";

    /* Clean up if exists */
    orbisjvm_dir_remove(test_dir);

    int ret = orbisjvm_dir_create(test_dir, ORBISJVM_DEFAULT_DIR_MODE);
    TEST_ASSERT(ret == 0, "dir_create should succeed");

    int exists = orbisjvm_file_exists(test_dir);
    TEST_ASSERT(exists == 1, "directory should exist");

    int is_dir = orbisjvm_path_is_dir(test_dir);
    TEST_ASSERT(is_dir == 1, "should be a directory");

    ret = orbisjvm_dir_remove(test_dir);
    TEST_ASSERT(ret == 0, "dir_remove should succeed");

    exists = orbisjvm_file_exists(test_dir);
    TEST_ASSERT(exists == 0, "directory should not exist after remove");

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "dir create/remove test passed");
    return 0;
}

int test_dir_create_recursive(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Testing recursive directory create...");

    const char* test_dir = "/tmp/orbisjvm_test_recursive/a/b/c/d";

    int ret = orbisjvm_dir_create_recursive(test_dir, ORBISJVM_DEFAULT_DIR_MODE);
    TEST_ASSERT(ret == 0, "dir_create_recursive should succeed");

    int is_dir = orbisjvm_path_is_dir(test_dir);
    TEST_ASSERT(is_dir == 1, "deep directory should exist");

    /* Clean up */
    orbisjvm_dir_remove("/tmp/orbisjvm_test_recursive/a/b/c/d");
    orbisjvm_dir_remove("/tmp/orbisjvm_test_recursive/a/b/c");
    orbisjvm_dir_remove("/tmp/orbisjvm_test_recursive/a/b");
    orbisjvm_dir_remove("/tmp/orbisjvm_test_recursive/a");

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "recursive dir create test passed");
    return 0;
}

int test_dir_read(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Testing directory read...");

    const char* test_dir = "/tmp/orbisjvm_test_dir_read";

    /* Clean up and create */
    orbisjvm_dir_remove(test_dir);
    orbisjvm_dir_create(test_dir, ORBISJVM_DEFAULT_DIR_MODE);

    /* Create some test files */
    orbisjvm_file_t file;
    char path[256];

    for (int i = 0; i < 5; i++) {
        snprintf(path, sizeof(path), "%s/file%d.txt", test_dir, i);
        int ret = orbisjvm_file_open(&file, path,
                                     ORBISJVM_O_WRONLY | ORBISJVM_O_CREAT | ORBISJVM_O_TRUNC,
                                     ORBISJVM_DEFAULT_FILE_MODE);
        TEST_ASSERT(ret == 0, "file_open for test file should succeed");
        orbisjvm_file_close(file);
    }

    /* Create a subdirectory */
    snprintf(path, sizeof(path), "%s/subdir", test_dir);
    orbisjvm_dir_create(path, ORBISJVM_DEFAULT_DIR_MODE);

    /* Open and read directory */
    orbisjvm_dir_t dir;
    ret = orbisjvm_dir_open(&dir, test_dir);
    TEST_ASSERT(ret == 0, "dir_open should succeed");

    int file_count = 0;
    int dir_count = 0;
    orbisjvm_dirent_t entry;

    while (1) {
        ret = orbisjvm_dir_read(dir, &entry);
        if (ret == 1) break;  /* End of directory */
        TEST_ASSERT(ret == 0, "dir_read should succeed");

        if (entry.type == ORBISJVM_FILE_TYPE_REGULAR) {
            file_count++;
        } else if (entry.type == ORBISJVM_FILE_TYPE_DIR) {
            dir_count++;
        }
    }

    ret = orbisjvm_dir_close(dir);
    TEST_ASSERT(ret == 0, "dir_close should succeed");

    /* Should find 5 files + 1 subdir + . and .. (if included) */
    TEST_ASSERT(file_count >= 5, "should find at least 5 files");
    TEST_ASSERT(dir_count >= 1, "should find at least 1 directory");

    /* Clean up */
    for (int i = 0; i < 5; i++) {
        snprintf(path, sizeof(path), "%s/file%d.txt", test_dir, i);
        orbisjvm_file_remove(path);
    }
    orbisjvm_dir_remove("/tmp/orbisjvm_test_dir_read/subdir");
    orbisjvm_dir_remove(test_dir);

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "dir read test passed (files: %d, dirs: %d)", file_count, dir_count);
    return 0;
}

int test_file_rename_remove(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Testing file rename/remove...");

    const char* src = "/tmp/orbisjvm_test_rename_src.txt";
    const char* dst = "/tmp/orbisjvm_test_rename_dst.txt";

    /* Clean up */
    orbisjvm_file_remove(src);
    orbisjvm_file_remove(dst);

    /* Create source file */
    orbisjvm_file_t file;
    int ret = orbisjvm_file_open(&file, src,
                                 ORBISJVM_O_WRONLY | ORBISJVM_O_CREAT | ORBISJVM_O_TRUNC,
                                 ORBISJVM_DEFAULT_FILE_MODE);
    TEST_ASSERT(ret == 0, "file_open for source should succeed");

    const char* data = "rename test data";
    size_t written;
    ret = orbisjvm_file_write(file, data, strlen(data), &written);
    TEST_ASSERT(ret == 0, "file_write should succeed");
    orbisjvm_file_close(file);

    /* Rename */
    ret = orbisjvm_file_rename(src, dst);
    TEST_ASSERT(ret == 0, "file_rename should succeed");

    /* Source should not exist */
    int exists = orbisjvm_file_exists(src);
    TEST_ASSERT(exists == 0, "source should not exist after rename");

    /* Destination should exist */
    exists = orbisjvm_file_exists(dst);
    TEST_ASSERT(exists == 1, "destination should exist after rename");

    /* Verify content */
    ret = orbisjvm_file_open(&file, dst, ORBISJVM_O_RDONLY, 0);
    TEST_ASSERT(ret == 0, "file_open for renamed should succeed");

    char buffer[256];
    size_t read;
    ret = orbisjvm_file_read(file, buffer, sizeof(buffer) - 1, &read);
    TEST_ASSERT(ret == 0, "file_read should succeed");
    buffer[read] = '\0';
    TEST_ASSERT(strcmp(buffer, data) == 0, "renamed file content should match");

    orbisjvm_file_close(file);

    /* Remove */
    ret = orbisjvm_file_remove(dst);
    TEST_ASSERT(ret == 0, "file_remove should succeed");

    exists = orbisjvm_file_exists(dst);
    TEST_ASSERT(exists == 0, "file should not exist after remove");

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "file rename/remove test passed");
    return 0;
}

int main(void) {
    orbisjvm_init();

    int failed = 0;

    if (test_dir_create_remove() < 0) failed++;
    if (test_dir_create_recursive() < 0) failed++;
    if (test_dir_read() < 0) failed++;
    if (test_file_rename_remove() < 0) failed++;

    orbisjvm_shutdown();

    if (failed > 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_GENERAL, "%d test(s) failed", failed);
        return 1;
    }

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "All filesystem directory tests passed!");
    return 0;
}