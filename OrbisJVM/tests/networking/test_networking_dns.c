/**
 * @file test_networking_dns.c
 * @brief DNS resolution tests
 */

#include <orbisjvm/networking.h>
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

int test_dns_resolve_localhost(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Testing DNS resolve localhost...");

    orbisjvm_in_addr_t addr;
    int ret = orbisjvm_resolve("localhost", &addr, 5000, 2);
    TEST_ASSERT(ret == 0, "resolve localhost should succeed");

    /* localhost should resolve to 127.0.0.1 */
    TEST_ASSERT(addr.s_addr == orbisjvm_htonl(0x7F000001), "localhost should be 127.0.0.1");

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "DNS resolve localhost test passed");
    return 0;
}

int test_dns_resolve_multiple(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Testing DNS resolve multiple...");

    orbisjvm_in_addr_t addrs[4];
    int count = 0;

    int ret = orbisjvm_resolve_multiple("localhost", addrs, 4, &count, 5000);
    TEST_ASSERT(ret == 0, "resolve_multiple should succeed");
    TEST_ASSERT(count > 0, "should resolve at least one address");

    for (int i = 0; i < count; i++) {
        TEST_ASSERT(addrs[i].s_addr == orbisjvm_htonl(0x7F000001),
                    "all addresses should be 127.0.0.1");
    }

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "DNS resolve multiple test passed (count: %d)", count);
    return 0;
}

int test_dns_inet_pton_ntop(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Testing inet_pton/ntop...");

    orbisjvm_in_addr_t addr;
    int ret = orbisjvm_inet_pton(ORBISJVM_AF_INET, "192.168.1.100", &addr);
    TEST_ASSERT(ret == 1, "inet_pton should succeed");

    char buffer[64];
    const char* result = orbisjvm_inet_ntop(ORBISJVM_AF_INET, &addr, buffer, sizeof(buffer));
    TEST_ASSERT(result != NULL, "inet_ntop should succeed");
    TEST_ASSERT(strcmp(buffer, "192.168.1.100") == 0, "ntop should produce correct string");

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "inet_pton/ntop test passed");
    return 0;
}

int test_byte_order(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Testing byte order conversions...");

    uint32_t host32 = 0x12345678;
    uint32_t net32 = orbisjvm_htonl(host32);
    uint32_t back32 = orbisjvm_ntohl(net32);
    TEST_ASSERT(back32 == host32, "htonl/ntohl should be reversible");

    uint16_t host16 = 0x1234;
    uint16_t net16 = orbisjvm_htons(host16);
    uint16_t back16 = orbisjvm_ntohs(net16);
    TEST_ASSERT(back16 == host16, "htons/ntohs should be reversible");

    uint64_t host64 = 0x1122334455667788ULL;
    uint64_t net64 = orbisjvm_htonll(host64);
    uint64_t back64 = orbisjvm_ntohll(net64);
    TEST_ASSERT(back64 == host64, "htonll/ntohll should be reversible");

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "byte order test passed");
    return 0;
}

int main(void) {
    orbisjvm_init();
    orbisjvm_net_init();

    int failed = 0;

    if (test_dns_resolve_localhost() < 0) failed++;
    if (test_dns_resolve_multiple() < 0) failed++;
    if (test_dns_inet_pton_ntop() < 0) failed++;
    if (test_byte_order() < 0) failed++;

    orbisjvm_net_shutdown();
    orbisjvm_shutdown();

    if (failed > 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_GENERAL, "%d test(s) failed", failed);
        return 1;
    }

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "All DNS networking tests passed!");
    return 0;
}