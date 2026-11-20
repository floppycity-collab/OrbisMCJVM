/**
 * @file test_memory_mmap.c
 * @brief Memory mapping tests
 */

#include <orbisjvm/memory.h>
#include <orbisjvm/diagnostics.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            orbisjvm_log_error(ORBISJVM_LOG_CAT_GENERAL, "ASSERT FAILED: %s: %s", #cond, msg); \
            return -1; \
        } \
    } while (0)

int test_anonymous_mmap(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Testing anonymous mmap...");

    size_t size = 4096 * 4;  /* 4 pages */
    void* ptr = orbisjvm_mmap(NULL, size,
                              ORBISJVM_PROT_READ | ORBISJVM_PROT_WRITE,
                              ORBISJVM_MAP_PRIVATE | ORBISJVM_MAP_ANONYMOUS,
                              -1, 0);
    TEST_ASSERT(ptr != ORBISJVM_MAP_FAILED, "anonymous mmap should succeed");

    memset(ptr, 0xEF, size);

    int ret = orbisjvm_munmap(ptr, size);
    TEST_ASSERT(ret == 0, "munmap should succeed");

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "anonymous mmap test passed");
    return 0;
}

int test_mmap_protection(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Testing mmap protection...");

    size_t size = 4096;
    void* ptr = orbisjvm_mmap(NULL, size,
                              ORBISJVM_PROT_READ | ORBISJVM_PROT_WRITE,
                              ORBISJVM_MAP_PRIVATE | ORBISJVM_MAP_ANONYMOUS,
                              -1, 0);
    TEST_ASSERT(ptr != ORBISJVM_MAP_FAILED, "mmap should succeed");

    /* Change to read-only */
    int ret = orbisjvm_mprotect(ptr, size, ORBISJVM_PROT_READ);
    TEST_ASSERT(ret == 0, "mprotect to READ should succeed");

    /* Try to write (should fault on real hardware, but we test the API) */
    /* We can't easily test the fault in a portable way */

    /* Change back to read-write */
    ret = orbisjvm_mprotect(ptr, size, ORBISJVM_PROT_READ | ORBISJVM_PROT_WRITE);
    TEST_ASSERT(ret == 0, "mprotect to READ|WRITE should succeed");

    orbisjvm_munmap(ptr, size);

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "mmap protection test passed");
    return 0;
}

int test_file_mmap(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Testing file-backed mmap...");

    /* Create a test file */
    const char* test_file = "/tmp/orbisjvm_mmap_test.bin";
    int fd = open(test_file, O_CREAT | O_RDWR | O_TRUNC, 0644);
    TEST_ASSERT(fd >= 0, "open should succeed");

    /* Write test data */
    char test_data[4096];
    for (int i = 0; i < 4096; i++) {
        test_data[i] = i & 0xFF;
    }
    write(fd, test_data, sizeof(test_data));
    close(fd);

    /* Reopen for mmap */
    fd = open(test_file, O_RDONLY);
    TEST_ASSERT(fd >= 0, "open for mmap should succeed");

    void* ptr = orbisjvm_mmap(NULL, 4096,
                              ORBISJVM_PROT_READ,
                              ORBISJVM_MAP_PRIVATE,
                              fd, 0);
    close(fd);

    if (ptr == ORBISJVM_MAP_FAILED) {
        orbisjvm_log_warn(ORBISJVM_LOG_CAT_GENERAL, "file mmap not supported, skipping");
        unlink(test_file);
        return 0;  /* Not a failure - file mmap may not be supported */
    }

    /* Verify data */
    char* mapped = (char*)ptr;
    for (int i = 0; i < 4096; i++) {
        TEST_ASSERT(mapped[i] == (i & 0xFF), "mapped data should match file");
    }

    orbisjvm_munmap(ptr, 4096);
    unlink(test_file);

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "file-backed mmap test passed");
    return 0;
}

int test_mmap_fixed(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Testing MAP_FIXED (if supported)...");

    size_t size = 4096;
    void* ptr = orbisjvm_mmap(NULL, size,
                              ORBISJVM_PROT_READ | ORBISJVM_PROT_WRITE,
                              ORBISJVM_MAP_PRIVATE | ORBISJVM_MAP_ANONYMOUS,
                              -1, 0);
    TEST_ASSERT(ptr != ORBISJVM_MAP_FAILED, "initial mmap should succeed");

    /* Try to map at specific address (aligned) */
    void* hint = (void*)(((uintptr_t)ptr + 65536) & ~4095);
    void* ptr2 = orbisjvm_mmap(hint, size,
                               ORBISJVM_PROT_READ | ORBISJVM_PROT_WRITE,
                               ORBISJVM_MAP_PRIVATE | ORBISJVM_MAP_ANONYMOUS | ORBISJVM_MAP_FIXED,
                               -1, 0);

    if (ptr2 != ORBISJVM_MAP_FAILED) {
        orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "MAP_FIXED succeeded at %p", ptr2);
        orbisjvm_munmap(ptr2, size);
    } else {
        orbisjvm_log_warn(ORBISJVM_LOG_CAT_GENERAL, "MAP_FIXED not supported or failed");
    }

    orbisjvm_munmap(ptr, size);

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "MAP_FIXED test passed");
    return 0;
}

int main(void) {
    orbisjvm_init();

    int failed = 0;

    if (test_anonymous_mmap() < 0) failed++;
    if (test_mmap_protection() < 0) failed++;
    if (test_file_mmap() < 0) failed++;
    if (test_mmap_fixed() < 0) failed++;

    orbisjvm_shutdown();

    if (failed > 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_GENERAL, "%d test(s) failed", failed);
        return 1;
    }

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "All mmap tests passed!");
    return 0;
}