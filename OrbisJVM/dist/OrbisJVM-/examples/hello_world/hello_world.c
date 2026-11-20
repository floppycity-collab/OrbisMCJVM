/**
 * @file hello_world.c
 * @brief OrbisJVM Hello World Example
 *
 * Demonstrates basic OrbisJVM initialization and diagnostics.
 */

#include <orbisjvm/runtime.h>
#include <orbisjvm/diagnostics.h>
#include <stdio.h>

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    printf("=== OrbisJVM Hello World ===\n");

    /* Initialize runtime */
    int ret = orbisjvm_init();
    if (ret < 0) {
        fprintf(stderr, "Failed to initialize OrbisJVM: %d\n", ret);
        return 1;
    }

    /* Print diagnostics */
    orbisjvm_print_diagnostics();

    /* Test basic functionality */
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Hello from OrbisJVM!");

    /* Test memory allocation */
    void* ptr = orbisjvm_malloc(1024, ORBISJVM_MEM_ZERO);
    if (ptr) {
        orbisjvm_log_info(ORBISJVM_LOG_CAT_MEMORY, "Allocated 1 KB at %p", ptr);
        orbisjvm_free(ptr);
    }

    /* Test threading */
    orbisjvm_thread_t thread;
    ret = orbisjvm_thread_create(&thread, NULL, [](void* arg) -> void* {
        orbisjvm_log_info(ORBISJVM_LOG_CAT_THREADING, "Thread running: %s", (char*)arg);
        orbisjvm_thread_sleep_ms(100);
        return NULL;
    }, "Hello Thread");

    if (ret == 0) {
        orbisjvm_thread_join(thread, NULL);
    }

    /* Test time */
    uint64_t now = orbisjvm_get_monotonic_ms();
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Current monotonic time: %llu ms", now);

    /* Shutdown */
    orbisjvm_shutdown();

    printf("=== OrbisJVM Hello World Complete ===\n");
    return 0;
}