/**
 * @file test_networking_tcp.c
 * @brief TCP networking tests
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

#define TEST_PORT 18080
#define TEST_BACKLOG 5

static orbisjvm_socket_t g_server_sock = ORBISJVM_INVALID_SOCKET;
static int g_server_ready = 0;

void* tcp_server_thread(void* arg) {
    (void)arg;

    orbisjvm_sockaddr_in_t server_addr = {0};
    server_addr.sin_family = ORBISJVM_AF_INET;
    server_addr.sin_port = orbisjvm_htons(TEST_PORT);
    server_addr.sin_addr.s_addr = orbisjvm_htonl(0x7F000001); /* 127.0.0.1 */

    g_server_sock = orbisjvm_socket(ORBISJVM_AF_INET, ORBISJVM_SOCK_STREAM, ORBISJVM_IPPROTO_TCP);
    if (g_server_sock == ORBISJVM_INVALID_SOCKET) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_NETWORK, "server socket creation failed");
        return (void*)-1;
    }

    int opt = 1;
    orbisjvm_setsockopt(g_server_sock, ORBISJVM_SOL_SOCKET, ORBISJVM_SO_REUSEADDR, &opt, sizeof(opt));

    int ret = orbisjvm_bind(g_server_sock, (orbisjvm_sockaddr_t*)&server_addr, sizeof(server_addr));
    if (ret < 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_NETWORK, "server bind failed: %d", ret);
        orbisjvm_close(g_server_sock);
        return (void*)-1;
    }

    ret = orbisjvm_listen(g_server_sock, TEST_BACKLOG);
    if (ret < 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_NETWORK, "server listen failed: %d", ret);
        orbisjvm_close(g_server_sock);
        return (void*)-1;
    }

    g_server_ready = 1;

    orbisjvm_sockaddr_in_t client_addr;
    uint32_t client_addrlen = sizeof(client_addr);
    orbisjvm_socket_t client_sock = orbisjvm_accept(g_server_sock, (orbisjvm_sockaddr_t*)&client_addr, &client_addrlen);
    if (client_sock == ORBISJVM_INVALID_SOCKET) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_NETWORK, "server accept failed");
        orbisjvm_close(g_server_sock);
        return (void*)-1;
    }

    char buffer[256];
    int32_t received = orbisjvm_recv(client_sock, buffer, sizeof(buffer) - 1, 0);
    if (received > 0) {
        buffer[received] = '\0';
        orbisjvm_log_info(ORBISJVM_LOG_CAT_NETWORK, "Server received: %s", buffer);

        const char* response = "Hello from server!";
        orbisjvm_send(client_sock, response, strlen(response), 0);
    }

    orbisjvm_close(client_sock);
    orbisjvm_close(g_server_sock);
    g_server_sock = ORBISJVM_INVALID_SOCKET;

    return NULL;
}

int test_tcp_client_server(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Testing TCP client-server...");

    orbisjvm_thread_t server_thread;
    int ret = orbisjvm_thread_create(&server_thread, NULL, tcp_server_thread, NULL);
    TEST_ASSERT(ret == 0, "server thread creation should succeed");

    /* Wait for server to be ready */
    while (!g_server_ready) {
        orbisjvm_thread_sleep_ms(10);
    }

    /* Client connection */
    orbisjvm_sockaddr_in_t server_addr = {0};
    server_addr.sin_family = ORBISJVM_AF_INET;
    server_addr.sin_port = orbisjvm_htons(TEST_PORT);
    server_addr.sin_addr.s_addr = orbisjvm_htonl(0x7F000001);

    orbisjvm_socket_t client_sock = orbisjvm_socket(ORBISJVM_AF_INET, ORBISJVM_SOCK_STREAM, ORBISJVM_IPPROTO_TCP);
    TEST_ASSERT(client_sock != ORBISJVM_INVALID_SOCKET, "client socket creation should succeed");

    ret = orbisjvm_connect(client_sock, (orbisjvm_sockaddr_t*)&server_addr, sizeof(server_addr));
    TEST_ASSERT(ret == 0, "client connect should succeed");

    const char* message = "Hello from client!";
    int32_t sent = orbisjvm_send(client_sock, message, strlen(message), 0);
    TEST_ASSERT(sent == (int32_t)strlen(message), "send should send all bytes");

    char buffer[256];
    int32_t received = orbisjvm_recv(client_sock, buffer, sizeof(buffer) - 1, 0);
    TEST_ASSERT(received > 0, "recv should receive data");
    buffer[received] = '\0';
    TEST_ASSERT(strcmp(buffer, "Hello from server!") == 0, "response should match");

    orbisjvm_close(client_sock);

    void* retval;
    ret = orbisjvm_thread_join(server_thread, &retval);
    TEST_ASSERT(ret == 0, "server thread join should succeed");

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "TCP client-server test passed");
    return 0;
}

int test_tcp_socket_options(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Testing TCP socket options...");

    orbisjvm_socket_t sock = orbisjvm_socket(ORBISJVM_AF_INET, ORBISJVM_SOCK_STREAM, ORBISJVM_IPPROTO_TCP);
    TEST_ASSERT(sock != ORBISJVM_INVALID_SOCKET, "socket creation should succeed");

    /* Test SO_REUSEADDR */
    int opt = 1;
    int ret = orbisjvm_setsockopt(sock, ORBISJVM_SOL_SOCKET, ORBISJVM_SO_REUSEADDR, &opt, sizeof(opt));
    TEST_ASSERT(ret == 0, "setsockopt SO_REUSEADDR should succeed");

    int get_opt = 0;
    uint32_t optlen = sizeof(get_opt);
    ret = orbisjvm_getsockopt(sock, ORBISJVM_SOL_SOCKET, ORBISJVM_SO_REUSEADDR, &get_opt, &optlen);
    TEST_ASSERT(ret == 0, "getsockopt SO_REUSEADDR should succeed");
    TEST_ASSERT(get_opt == 1, "SO_REUSEADDR should be enabled");

    /* Test TCP_NODELAY */
    opt = 1;
    ret = orbisjvm_setsockopt(sock, ORBISJVM_IPPROTO_TCP, ORBISJVM_TCP_NODELAY, &opt, sizeof(opt));
    TEST_ASSERT(ret == 0, "setsockopt TCP_NODELAY should succeed");

    get_opt = 0;
    optlen = sizeof(get_opt);
    ret = orbisjvm_getsockopt(sock, ORBISJVM_IPPROTO_TCP, ORBISJVM_TCP_NODELAY, &get_opt, &optlen);
    TEST_ASSERT(ret == 0, "getsockopt TCP_NODELAY should succeed");
    TEST_ASSERT(get_opt == 1, "TCP_NODELAY should be enabled");

    /* Test send/recv buffer sizes */
    int bufsize = 65536;
    ret = orbisjvm_setsockopt(sock, ORBISJVM_SOL_SOCKET, ORBISJVM_SO_SNDBUF, &bufsize, sizeof(bufsize));
    TEST_ASSERT(ret == 0, "setsockopt SO_SNDBUF should succeed");

    ret = orbisjvm_setsockopt(sock, ORBISJVM_SOL_SOCKET, ORBISJVM_SO_RCVBUF, &bufsize, sizeof(bufsize));
    TEST_ASSERT(ret == 0, "setsockopt SO_RCVBUF should succeed");

    orbisjvm_close(sock);

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "TCP socket options test passed");
    return 0;
}

int test_tcp_shutdown(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Testing TCP shutdown...");

    orbisjvm_socket_t sock = orbisjvm_socket(ORBISJVM_AF_INET, ORBISJVM_SOCK_STREAM, ORBISJVM_IPPROTO_TCP);
    TEST_ASSERT(sock != ORBISJVM_INVALID_SOCKET, "socket creation should succeed");

    int ret = orbisjvm_shutdown(sock, 0); /* SHUT_RD */
    TEST_ASSERT(ret == 0, "shutdown SHUT_RD should succeed");

    ret = orbisjvm_shutdown(sock, 1); /* SHUT_WR */
    TEST_ASSERT(ret == 0, "shutdown SHUT_WR should succeed");

    ret = orbisjvm_shutdown(sock, 2); /* SHUT_RDWR */
    TEST_ASSERT(ret == 0, "shutdown SHUT_RDWR should succeed");

    orbisjvm_close(sock);

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "TCP shutdown test passed");
    return 0;
}

int main(void) {
    orbisjvm_init();
    orbisjvm_net_init();

    int failed = 0;

    if (test_tcp_client_server() < 0) failed++;
    if (test_tcp_socket_options() < 0) failed++;
    if (test_tcp_shutdown() < 0) failed++;

    orbisjvm_net_shutdown();
    orbisjvm_shutdown();

    if (failed > 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_GENERAL, "%d test(s) failed", failed);
        return 1;
    }

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "All TCP networking tests passed!");
    return 0
}