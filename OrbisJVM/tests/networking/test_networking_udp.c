/**
 * @file test_networking_udp.c
 * @brief UDP networking tests
 */

#include <orbisjvm/networking.h>
#include <orbisjvm/diagnostics.h>
#include <orbisjvm/threading.h>
#include <stdio.h>
#include <string.h>

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            orbisjvm_log_error(ORBISJVM_LOG_CAT_GENERAL, "ASSERT FAILED: %s: %s", #cond, msg); \
            return -1; \
        } \
    } while (0)

#define TEST_PORT 18081

static int g_udp_server_ready = 0;

void* udp_server_thread(void* arg) {
    (void)arg;

    orbisjvm_sockaddr_in_t server_addr = {0};
    server_addr.sin_family = ORBISJVM_AF_INET;
    server_addr.sin_port = orbisjvm_htons(TEST_PORT);
    server_addr.sin_addr.s_addr = orbisjvm_htonl(0x7F000001);

    orbisjvm_socket_t sock = orbisjvm_socket(ORBISJVM_AF_INET, ORBISJVM_SOCK_DGRAM, ORBISJVM_IPPROTO_UDP);
    if (sock == ORBISJVM_INVALID_SOCKET) {
        return (void*)-1;
    }

    int ret = orbisjvm_bind(sock, (orbisjvm_sockaddr_t*)&server_addr, sizeof(server_addr));
    if (ret < 0) {
        orbisjvm_close(sock);
        return (void*)-1;
    }

    g_udp_server_ready = 1;

    char buffer[256];
    orbisjvm_sockaddr_in_t client_addr;
    uint32_t client_addrlen = sizeof(client_addr);

    int32_t received = orbisjvm_recvfrom(sock, buffer, sizeof(buffer) - 1, 0,
                                         (orbisjvm_sockaddr_t*)&client_addr, &client_addrlen);
    if (received > 0) {
        buffer[received] = '\0';
        orbisjvm_log_info(ORBISJVM_LOG_CAT_NETWORK, "UDP Server received: %s", buffer);

        const char* response = "UDP response from server!";
        orbisjvm_sendto(sock, response, strlen(response), 0,
                       (orbisjvm_sockaddr_t*)&client_addr, client_addrlen);
    }

    orbisjvm_close(sock);
    return NULL;
}

int test_udp_client_server(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Testing UDP client-server...");

    orbisjvm_thread_t server_thread;
    int ret = orbisjvm_thread_create(&server_thread, NULL, udp_server_thread, NULL);
    TEST_ASSERT(ret == 0, "server thread creation should succeed");

    while (!g_udp_server_ready) {
        orbisjvm_thread_sleep_ms(10);
    }

    orbisjvm_sockaddr_in_t server_addr = {0};
    server_addr.sin_family = ORBISJVM_AF_INET;
    server_addr.sin_port = orbisjvm_htons(TEST_PORT);
    server_addr.sin_addr.s_addr = orbisjvm_htonl(0x7F000001);

    orbisjvm_socket_t client_sock = orbisjvm_socket(ORBISJVM_AF_INET, ORBISJVM_SOCK_DGRAM, ORBISJVM_IPPROTO_UDP);
    TEST_ASSERT(client_sock != ORBISJVM_INVALID_SOCKET, "client socket creation should succeed");

    const char* message = "Hello UDP server!";
    int32_t sent = orbisjvm_sendto(client_sock, message, strlen(message), 0,
                                  (orbisjvm_sockaddr_t*)&server_addr, sizeof(server_addr));
    TEST_ASSERT(sent == (int32_t)strlen(message), "sendto should send all bytes");

    char buffer[256];
    orbisjvm_sockaddr_in_t from_addr;
    uint32_t from_addrlen = sizeof(from_addr);
    int32_t received = orbisjvm_recvfrom(client_sock, buffer, sizeof(buffer) - 1, 0,
                                        (orbisjvm_sockaddr_t*)&from_addr, &from_addrlen);
    TEST_ASSERT(received > 0, "recvfrom should receive data");
    buffer[received] = '\0';
    TEST_ASSERT(strcmp(buffer, "UDP response from server!") == 0, "response should match");

    orbisjvm_close(client_sock);

    void* retval;
    ret = orbisjvm_thread_join(server_thread, &retval);
    TEST_ASSERT(ret == 0, "server thread join should succeed");

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "UDP client-server test passed");
    return 0;
}

int test_udp_broadcast(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Testing UDP broadcast option...");

    orbisjvm_socket_t sock = orbisjvm_socket(ORBISJVM_AF_INET, ORBISJVM_SOCK_DGRAM, ORBISJVM_IPPROTO_UDP);
    TEST_ASSERT(sock != ORBISJVM_INVALID_SOCKET, "socket creation should succeed");

    int opt = 1;
    int ret = orbisjvm_setsockopt(sock, ORBISJVM_SOL_SOCKET, ORBISJVM_SO_BROADCAST, &opt, sizeof(opt));
    TEST_ASSERT(ret == 0, "setsockopt SO_BROADCAST should succeed");

    int get_opt = 0;
    uint32_t optlen = sizeof(get_opt);
    ret = orbisjvm_getsockopt(sock, ORBISJVM_SOL_SOCKET, ORBISJVM_SO_BROADCAST, &get_opt, &optlen);
    TEST_ASSERT(ret == 0, "getsockopt SO_BROADCAST should succeed");
    TEST_ASSERT(get_opt == 1, "SO_BROADCAST should be enabled");

    orbisjvm_close(sock);

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "UDP broadcast test passed");
    return 0;
}

int main(void) {
    orbisjvm_init();
    orbisjvm_net_init();

    int failed = 0;

    if (test_udp_client_server() < 0) failed++;
    if (test_udp_broadcast() < 0) failed++;

    orbisjvm_net_shutdown();
    orbisjvm_shutdown();

    if (failed > 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_GENERAL, "%d test(s) failed", failed);
        return 1;
    }

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "All UDP networking tests passed!");
    return 0;
}