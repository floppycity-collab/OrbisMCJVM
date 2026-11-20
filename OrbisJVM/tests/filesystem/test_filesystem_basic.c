/**
 * @file test_filesystem_basic.c
 * @brief Basic filesystem tests
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

int test_file_write_read(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Testing file write/read...");

    const char* test_file = "/tmp/orbisjvm_test_write.bin";
    orbisjvm_file_t file;

    int ret = orbisjvm_file_open(&file, test_file,
                                 ORBISJVM_O_WRONLY | ORBISJVM_O_CREAT | ORBISJVM_O_TRUNC,
                                 ORBISJVM_DEFAULT_FILE_MODE);
    TEST_ASSERT(ret == 0, "file_open for write should succeed");

    const char* test_data = "Hello, OrbisJVM Filesystem!";
    size_t written;
    ret = orbisjvm_file_write(file, test_data, strlen(test_data), &written);
    TEST_ASSERT(ret == 0, "file_write should succeed");
    TEST_ASSERT(written == strlen(test_data), "should write all bytes");

    ret = orbisjvm_file_close(file);
    TEST_ASSERT(ret == 0, "file_close should succeed");

    /* Read back */
    ret = orbisjvm_file_open(&file, test_file, ORBISJVM_O_RDONLY, 0);
    TEST_ASSERT(ret == 0, "file_open for read should succeed");

    char buffer[256];
    size_t read;
    ret = orbisjvm_file_read(file, buffer, sizeof(buffer) - 1, &read);
    TEST_ASSERT(ret == 0, "file_read should succeed");
    TEST_ASSERT(read == strlen(test_data), "should read all bytes");

    buffer[read] = '\0';
    TEST_ASSERT(strcmp(buffer, test_data) == 0, "read data should match written data");

    ret = orbisjvm_file_close(file);
    TEST_ASSERT(ret == 0, "file_close should succeed");

    /* Cleanup */
    orbisjvm_file_remove(test_file);

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "file write/read test passed");
    return 0;
}

int test_file_pread_pwrite(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Testing pread/pwrite...");

    const char* test_file = "/tmp/orbisjvm_test_pread.bin";
    orbisjvm_file_t file;

    int ret = orbisjvm_file_open(&file, test_file,
                                 ORBISJVM_O_RDWR | ORBISJVM_O_CREAT | ORBISJVM_O_TRUNC,
                                 ORBISJVM_DEFAULT_FILE_MODE);
    TEST_ASSERT(ret == 0, "file_open should succeed");

    const char* data1 = "FIRST";
    const char* data2 = "SECOND";
    size_t written;

    /* Write at offset 0 */
    ret = orbisjvm_file_pwrite(file, data1, strlen(data1), 0, &written);
    TEST_ASSERT(ret == 0, "pwrite at 0 should succeed");
    TEST_ASSERT(written == strlen(data1), "should write all bytes");

    /* Write at offset 100 */
    ret = orbisjvm_file_pwrite(file, data2, strlen(data2), 100, &written);
    TEST_ASSERT(ret == 0, "pwrite at 100 should succeed");

    /* Read back at offset 0 */
    char buffer[256];
    size_t read;
    ret = orbisjvm_file_pread(file, buffer, 5, 0, &read);
    TEST_ASSERT(ret == 0, "pread at 0 should succeed");
    buffer[read] = '\0';
    TEST_ASSERT(strcmp(buffer, "FIRST") == 0, "first data should match");

    /* Read back at offset 100 */
    ret = orbisjvm_file_pread(file, buffer, 6, 100, &read);
    TEST_ASSERT(ret == 0, "pread at 100 should succeed");
    buffer[read] = '\0';
    TEST_ASSERT(strcmp(buffer, "SECOND") == 0, "second data should match");

    ret = orbisjvm_file_close(file);
    TEST_ASSERT(ret == 0, "file_close should succeed");

    orbisjvm_file_remove(test_file);

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "pread/pwrite test passed");
    return 0;
}

int test_file_seek_tell(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Testing file seek/tell...");

    const char* test_file = "/tmp/orbisjvm_test_seek.bin";
    orbisjvm_file_t file;

    int ret = orbisjvm_file_open(&file, test_file,
                                 ORBISJVM_O_RDWR | ORBISJVM_O_CREAT | ORBISJVM_O_TRUNC,
                                 ORBISJVM_DEFAULT_FILE_MODE);
    TEST_ASSERT(ret == 0, "file_open should succeed");

    const char* data = "0123456789ABCDEF";
    size_t written;
    ret = orbisjvm_file_write(file, data, strlen(data), &written);
    TEST_ASSERT(ret == 0, "file_write should succeed");

    /* Tell should be at end */
    int64_t pos;
    ret = orbisjvm_file_tell(file, &pos);
    TEST_ASSERT(ret == 0, "file_tell should succeed");
    TEST_ASSERT(pos == (int64_t)strlen(data), "position should be at end");

    /* Seek to beginning */
    ret = orbisjvm_file_seek(file, 0, ORBISJVM_SEEK_SET, &pos);
    TEST_ASSERT(ret == 0, "seek to SET should succeed");
    TEST_ASSERT(pos == 0, "position should be 0");

    /* Seek to middle */
    ret = orbisjvm_file_seek(file, 5, ORBISJVM_SEEK_SET, &pos);
    TEST_ASSERT(ret == 0, "seek to 5 should succeed");
    TEST_ASSERT(pos == 5, "position should be 5");

    /* Read from middle */
    char buffer[10];
    size_t read;
    ret = orbisjvm_file_read(file, buffer, 5, &read);
    TEST_ASSERT(ret == 0, "read from middle should succeed");
    buffer[read] = '\0';
    TEST_ASSERT(strcmp(buffer, "56789") == 0, "should read from position 5");

    ret = orbisjvm_file_close(file);
    TEST_ASSERT(ret == 0, "file_close should succeed");

    orbisjvm_file_remove(test_file);

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "seek/tell test passed");
    return 0;
}

int test_file_stat(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Testing file stat...");

    const char* test_file = "/tmp/orbisjvm_test_stat.bin";
    orbisjvm_file_t file;

    int ret = orbisjvm_file_open(&file, test_file,
                                 ORBISJVM_O_WRONLY | ORBISJVM_O_CREAT | ORBISJVM_O_TRUNC,
                                 ORBISJVM_DEFAULT_FILE_MODE);
    TEST_ASSERT(ret == 0, "file_open should succeed");

    const char* data = "Test data for stat";
    size_t written;
    ret = orbisjvm_file_write(file, data, strlen(data), &written);
    TEST_ASSERT(ret == 0, "file_write should succeed");

    orbisjvm_file_info_t info;
    ret = orbisjvm_file_stat(file, &info);
    TEST_ASSERT(ret == 0, "file_stat should succeed");
    TEST_ASSERT(info.size == strlen(data), "file size should match");
    TEST_ASSERT(info.type == ORBISJVM_FILE_TYPE_REGULAR, "should be regular file");

    ret = orbisjvm_file_close(file);
    TEST_ASSERT(ret == 0, "file_close should succeed");

    /* lstat */
    ret = orbisjvm_file_lstat(test_file, &info, 1);
    TEST_ASSERT(ret == 0, "file_lstat should succeed");
    TEST_ASSERT(info.size == strlen(data), "lstat size should match");

    orbisjvm_file_remove(test_file);

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "file stat test passed");
    return 0;
}

int main(void) {
    orbisjvm_init();

    int failed = 0;

    if (test_file_write_read() < 0) failed++;
    if (test_file_pread_pwrite() < 0) failed++;
    if (test_file_seek_tell() < 0) failed++;
    if (test_file_stat() < 0) failed++;

    orbisjvm_shutdown();

    if (failed > 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_GENERAL, "%d test(s) failed", failed);
        return 1;
    }

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "All filesystem basic tests passed!");
    return 0;
}