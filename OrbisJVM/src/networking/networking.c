/**
 * @file networking.c
 * @brief OrbisJVM Networking Implementation
 *
 * Implements socket operations using Orbis OS Net API.
 * Critical for Minecraft server networking support.
 */

#include <orbisjvm/networking.h>
#include <orbisjvm/diagnostics.h>
#include <orbisjvm/memory.h>
#include <orbisjvm/threading.h>
#include <orbisjvm/time.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

/* Statistics */
static uint64_t g_bytes_sent = 0;
static uint64_t g_bytes_received = 0;
static uint64_t g_packets_sent = 0;
static uint64_t g_packets_received = 0;
static uint64_t g_errors = 0;
static uint32_t g_sockets_created = 0;
static uint32_t g_sockets_closed = 0;
static uint32_t g_current_sockets = 0;
static uint32_t g_peak_sockets = 0;
static orbisjvm_mutex_t g_stats_mutex;

/**
 * @brief Initialize networking subsystem
 */
int orbisjvm_net_init(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_NETWORK, "Initializing networking subsystem");

    int ret = orbisjvm_mutex_create(&g_stats_mutex, NULL);
    if (ret < 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_NETWORK, "Failed to create stats mutex");
        return ret;
    }

    /* Initialize Orbis Net if needed */
    /* sceNetInit() would be called here on actual PS4 */

    return 0;
}

/**
 * @brief Shutdown networking subsystem
 */
void orbisjvm_net_shutdown(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_NETWORK, "Shutting down networking subsystem");
    orbisjvm_mutex_destroy(g_stats_mutex);
}

/* Socket operations */

orbisjvm_socket_t orbisjvm_socket(int domain, int type, int protocol) {
    int sock = socket(domain, type, protocol);
    if (sock < 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_NETWORK, "socket() failed: errno=%d", errno);
        return ORBISJVM_INVALID_SOCKET;
    }

    orbisjvm_mutex_lock(g_stats_mutex);
    g_sockets_created++;
    g_current_sockets++;
    if (g_current_sockets > g_peak_sockets) {
        g_peak_sockets = g_current_sockets;
    }
    orbisjvm_mutex_unlock(g_stats_mutex);

    orbisjvm_log_trace(ORBISJVM_LOG_CAT_NETWORK, "Created socket: %d (domain=%d type=%d)", sock, domain, type);
    return sock;
}

int orbisjvm_close(orbisjvm_socket_t sock) {
    if (sock == ORBISJVM_INVALID_SOCKET) return -1;

    int ret = close(sock);
    if (ret < 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_NETWORK, "close() failed: errno=%d", errno);
        return -errno;
    }

    orbisjvm_mutex_lock(g_stats_mutex);
    g_sockets_closed++;
    g_current_sockets--;
    orbisjvm_mutex_unlock(g_stats_mutex);

    orbisjvm_log_trace(ORBISJVM_LOG_CAT_NETWORK, "Closed socket: %d", sock);
    return 0;
}

int orbisjvm_bind(orbisjvm_socket_t sock, const orbisjvm_sockaddr_t* addr, uint32_t addrlen) {
    if (sock == ORBISJVM_INVALID_SOCKET || !addr) return -1;

    int ret = bind(sock, (const struct sockaddr*)addr, addrlen);
    if (ret < 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_NETWORK, "bind() failed: errno=%d", errno);
        return -errno;
    }

    return 0;
}

int orbisjvm_listen(orbisjvm_socket_t sock, int backlog) {
    if (sock == ORBISJVM_INVALID_SOCKET) return -1;

    int ret = listen(sock, backlog);
    if (ret < 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_NETWORK, "listen() failed: errno=%d", errno);
        return -errno;
    }

    return 0;
}

orbisjvm_socket_t orbisjvm_accept(orbisjvm_socket_t sock, orbisjvm_sockaddr_t* addr, uint32_t* addrlen) {
    if (sock == ORBISJVM_INVALID_SOCKET) return ORBISJVM_INVALID_SOCKET;

    struct sockaddr_storage storage;
    socklen_t len = addrlen ? *addrlen : sizeof(storage);
    int client_sock = accept(sock, (struct sockaddr*)(addr ? addr : &storage), addrlen ? addrlen : &len);
    if (client_sock < 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_NETWORK, "accept() failed: errno=%d", errno);
        return ORBISJVM_INVALID_SOCKET;
    }

    orbisjvm_mutex_lock(g_stats_mutex);
    g_sockets_created++;
    g_current_sockets++;
    if (g_current_sockets > g_peak_sockets) {
        g_peak_sockets = g_current_sockets;
    }
    orbisjvm_mutex_unlock(g_stats_mutex);

    orbisjvm_log_trace(ORBISJVM_LOG_CAT_NETWORK, "Accepted connection: %d", client_sock);
    return client_sock;
}

int orbisjvm_connect(orbisjvm_socket_t sock, const orbisjvm_sockaddr_t* addr, uint32_t addrlen) {
    if (sock == ORBISJVM_INVALID_SOCKET || !addr) return -1;

    int ret = connect(sock, (const struct sockaddr*)addr, addrlen);
    if (ret < 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_NETWORK, "connect() failed: errno=%d", errno);
        return -errno;
    }

    return 0;
}

int32_t orbisjvm_send(orbisjvm_socket_t sock, const void* buffer, size_t length, int flags) {
    if (sock == ORBISJVM_INVALID_SOCKET || !buffer) return -1;

    ssize_t ret = send(sock, buffer, length, flags);
    if (ret < 0) {
        orbisjvm_mutex_lock(g_stats_mutex);
        g_errors++;
        orbisjvm_mutex_unlock(g_stats_mutex);
        orbisjvm_log_error(ORBISJVM_LOG_CAT_NETWORK, "send() failed: errno=%d", errno);
        return -errno;
    }

    orbisjvm_mutex_lock(g_stats_mutex);
    g_bytes_sent += ret;
    g_packets_sent++;
    orbisjvm_mutex_unlock(g_stats_mutex);

    return ret;
}

int32_t orbisjvm_recv(orbisjvm_socket_t sock, void* buffer, size_t length, int flags) {
    if (sock == ORBISJVM_INVALID_SOCKET || !buffer) return -1;

    ssize_t ret = recv(sock, buffer, length, flags);
    if (ret < 0) {
        orbisjvm_mutex_lock(g_stats_mutex);
        g_errors++;
        orbisjvm_mutex_unlock(g_stats_mutex);
        orbisjvm_log_error(ORBISJVM_LOG_CAT_NETWORK, "recv() failed: errno=%d", errno);
        return -errno;
    }

    orbisjvm_mutex_lock(g_stats_mutex);
    g_bytes_received += ret;
    g_packets_received++;
    orbisjvm_mutex_unlock(g_stats_mutex);

    return ret;
}

int32_t orbisjvm_sendto(orbisjvm_socket_t sock, const void* buffer, size_t length, int flags,
                        const orbisjvm_sockaddr_t* addr, uint32_t addrlen) {
    if (sock == ORBISJVM_INVALID_SOCKET || !buffer || !addr) return -1;

    ssize_t ret = sendto(sock, buffer, length, flags, (const struct sockaddr*)addr, addrlen);
    if (ret < 0) {
        orbisjvm_mutex_lock(g_stats_mutex);
        g_errors++;
        orbisjvm_mutex_unlock(g_stats_mutex);
        orbisjvm_log_error(ORBISJVM_LOG_CAT_NETWORK, "sendto() failed: errno=%d", errno);
        return -errno;
    }

    orbisjvm_mutex_lock(g_stats_mutex);
    g_bytes_sent += ret;
    g_packets_sent++;
    orbisjvm_mutex_unlock(g_stats_mutex);

    return ret;
}

int32_t orbisjvm_recvfrom(orbisjvm_socket_t sock, void* buffer, size_t length, int flags,
                          orbisjvm_sockaddr_t* addr, uint32_t* addrlen) {
    if (sock == ORBISJVM_INVALID_SOCKET || !buffer) return -1;

    struct sockaddr_storage storage;
    socklen_t len = addrlen ? *addrlen : sizeof(storage);
    ssize_t ret = recvfrom(sock, buffer, length, flags,
                           addr ? (struct sockaddr*)addr : (struct sockaddr*)&storage,
                           addrlen ? addrlen : &len);
    if (ret < 0) {
        orbisjvm_mutex_lock(g_stats_mutex);
        g_errors++;
        orbisjvm_mutex_unlock(g_stats_mutex);
        orbisjvm_log_error(ORBISJVM_LOG_CAT_NETWORK, "recvfrom() failed: errno=%d", errno);
        return -errno;
    }

    orbisjvm_mutex_lock(g_stats_mutex);
    g_bytes_received += ret;
    g_packets_received++;
    orbisjvm_mutex_unlock(g_stats_mutex);

    return ret;
}

int32_t orbisjvm_sendmsg(orbisjvm_socket_t sock, const orbisjvm_msghdr_t* msg, int flags) {
    if (sock == ORBISJVM_INVALID_SOCKET || !msg) return -1;

    ssize_t ret = sendmsg(sock, (const struct msghdr*)msg, flags);
    if (ret < 0) {
        orbisjvm_mutex_lock(g_stats_mutex);
        g_errors++;
        orbisjvm_mutex_unlock(g_stats_mutex);
        return -errno;
    }

    orbisjvm_mutex_lock(g_stats_mutex);
    g_bytes_sent += ret;
    g_packets_sent++;
    orbisjvm_mutex_unlock(g_stats_mutex);

    return ret;
}

int32_t orbisjvm_recvmsg(orbisjvm_socket_t sock, orbisjvm_msghdr_t* msg, int flags) {
    if (sock == ORBISJVM_INVALID_SOCKET || !msg) return -1;

    ssize_t ret = recvmsg(sock, (struct msghdr*)msg, flags);
    if (ret < 0) {
        orbisjvm_mutex_lock(g_stats_mutex);
        g_errors++;
        orbisjvm_mutex_unlock(g_stats_mutex);
        return -errno;
    }

    orbisjvm_mutex_lock(g_stats_mutex);
    g_bytes_received += ret;
    g_packets_received++;
    orbisjvm_mutex_unlock(g_stats_mutex);

    return ret;
}

int orbisjvm_shutdown(orbisjvm_socket_t sock, int how) {
    if (sock == ORBISJVM_INVALID_SOCKET) return -1;

    int ret = shutdown(sock, how);
    if (ret < 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_NETWORK, "shutdown() failed: errno=%d", errno);
        return -errno;
    }

    return 0;
}

int orbisjvm_getsockopt(orbisjvm_socket_t sock, int level, int optname, void* optval, uint32_t* optlen) {
    if (sock == ORBISJVM_INVALID_SOCKET) return -1;

    socklen_t len = optlen ? *optlen : 0;
    int ret = getsockopt(sock, level, optname, optval, optlen ? &len : NULL);
    if (ret < 0) {
        return -errno;
    }

    if (optlen) *optlen = len;
    return 0;
}

int orbisjvm_setsockopt(orbisjvm_socket_t sock, int level, int optname, const void* optval, uint32_t optlen) {
    if (sock == ORBISJVM_INVALID_SOCKET) return -1;

    int ret = setsockopt(sock, level, optname, optval, optlen);
    if (ret < 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_NETWORK, "setsockopt() failed: errno=%d", errno);
        return -errno;
    }

    return 0;
}

int orbisjvm_getsockname(orbisjvm_socket_t sock, orbisjvm_sockaddr_t* addr, uint32_t* addrlen) {
    if (sock == ORBISJVM_INVALID_SOCKET || !addr || !addrlen) return -1;

    socklen_t len = *addrlen;
    int ret = getsockname(sock, (struct sockaddr*)addr, &len);
    if (ret < 0) {
        return -errno;
    }

    *addrlen = len;
    return 0;
}

int orbisjvm_getpeername(orbisjvm_socket_t sock, orbisjvm_sockaddr_t* addr, uint32_t* addrlen) {
    if (sock == ORBISJVM_INVALID_SOCKET || !addr || !addrlen) return -1;

    socklen_t len = *addrlen;
    int ret = getpeername(sock, (struct sockaddr*)addr, &len);
    if (ret < 0) {
        return -errno;
    }

    *addrlen = len;
    return 0;
}

/* Byte order conversions */
uint32_t orbisjvm_htonl(uint32_t host) { return htonl(host); }
uint32_t orbisjvm_ntohl(uint32_t net) { return ntohl(net); }
uint16_t orbisjvm_htons(uint16_t host) { return htons(host); }
uint16_t orbisjvm_ntohs(uint16_t net) { return ntohs(net); }
uint64_t orbisjvm_htonll(uint64_t host) {
    return ((uint64_t)htonl((uint32_t)host) << 32) | htonl((uint32_t)(host >> 32));
}
uint64_t orbisjvm_ntohll(uint64_t net) {
    return ((uint64_t)ntohl((uint32_t)net) << 32) | ntohl((uint32_t)(net >> 32));
}

/* Address conversion */
int orbisjvm_inet_pton(int af, const char* src, void* dst) {
    if (!src || !dst) return -1;
    return inet_pton(af, src, dst);
}

const char* orbisjvm_inet_ntop(int af, const void* src, char* dst, uint32_t size) {
    if (!src || !dst || size == 0) return NULL;
    return inet_ntop(af, src, dst, size);
}

/* DNS resolution */
int orbisjvm_resolve(const char* hostname, orbisjvm_in_addr_t* addr, int timeout, int retry) {
    if (!hostname || !addr) return -1;

    struct addrinfo hints = {0}, *res = NULL;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int ret = getaddrinfo(hostname, NULL, &hints, &res);
    if (ret != 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_NETWORK, "getaddrinfo() failed: %s", gai_strerror(ret));
        return -ret;
    }

    struct sockaddr_in* in = (struct sockaddr_in*)res->ai_addr;
    addr->s_addr = in->sin_addr.s_addr;

    freeaddrinfo(res);
    return 0;
}

int orbisjvm_resolve_multiple(const char* hostname, orbisjvm_in_addr_t* addrs,
                              int max_addrs, int* count, int timeout) {
    if (!hostname || !addrs || !count || max_addrs <= 0) return -1;

    struct addrinfo hints = {0}, *res = NULL;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int ret = getaddrinfo(hostname, NULL, &hints, &res);
    if (ret != 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_NETWORK, "getaddrinfo() failed: %s", gai_strerror(ret));
        return -ret;
    }

    int found = 0;
    for (struct addrinfo* p = res; p && found < max_addrs; p = p->ai_next) {
        if (p->ai_family == AF_INET) {
            struct sockaddr_in* in = (struct sockaddr_in*)p->ai_addr;
            addrs[found].s_addr = in->sin_addr.s_addr;
            found++;
        }
    }

    *count = found;
    freeaddrinfo(res);
    return 0;
}

int orbisjvm_get_dns_info(orbisjvm_dns_info_t* info) {
    if (!info) return -1;
    /* On PS4, would use sceNetGetDnsInfo */
    memset(info, 0, sizeof(*info));
    return 0;
}

int orbisjvm_set_dns_info(const orbisjvm_dns_info_t* info) {
    if (!info) return -1;
    /* On PS4, would use sceNetSetDnsInfo */
    return 0;
}

int orbisjvm_get_interfaces(orbisjvm_if_entry_t* interfaces, int max_interfaces, int* count) {
    if (!interfaces || !count || max_interfaces <= 0) return -1;

    /* On PS4, would use sceNetGetIfList */
    *count = 0;
    return 0;
}

int orbisjvm_get_mac_address(uint8_t mac[6]) {
    if (!mac) return -1;
    /* On PS4, would use sceNetGetMacAddress */
    memset(mac, 0, 6);
    return 0;
}

int orbisjvm_get_last_error(void) {
    return errno;
}

int orbisjvm_set_nonblocking(orbisjvm_socket_t sock, int nonblock) {
    if (sock == ORBISJVM_INVALID_SOCKET) return -1;

    int flags = fcntl(sock, F_GETFL, 0);
    if (flags < 0) return -errno;

    if (nonblock) flags |= O_NONBLOCK;
    else flags &= ~O_NONBLOCK;

    int ret = fcntl(sock, F_SETFL, flags);
    if (ret < 0) return -errno;

    return 0;
}

int orbisjvm_get_nonblocking(orbisjvm_socket_t sock) {
    if (sock == ORBISJVM_INVALID_SOCKET) return -1;

    int flags = fcntl(sock, F_GETFL, 0);
    if (flags < 0) return -errno;

    return (flags & O_NONBLOCK) ? 1 : 0;
}

int orbisjvm_poll(orbisjvm_pollfd_t* fds, int nfds, int timeout) {
    if (!fds || nfds <= 0) return -1;

    struct pollfd* pfds = orbisjvm_malloc(nfds * sizeof(struct pollfd), ORBISJVM_MEM_ZERO);
    if (!pfds) return -ENOMEM;

    for (int i = 0; i < nfds; i++) {
        pfds[i].fd = fds[i].fd;
        pfds[i].events = fds[i].events;
        pfds[i].revents = 0;
    }

    int ret = poll(pfds, nfds, timeout);
    if (ret < 0) {
        orbisjvm_free(pfds);
        return -errno;
    }

    for (int i = 0; i < nfds; i++) {
        fds[i].revents = pfds[i].revents;
    }

    orbisjvm_free(pfds);
    return ret;
}

void orbisjvm_get_net_stats(orbisjvm_net_stats_t* stats) {
    if (!stats) return;

    orbisjvm_mutex_lock(g_stats_mutex);
    stats->bytes_sent = g_bytes_sent;
    stats->bytes_received = g_bytes_received;
    stats->packets_sent = g_packets_sent;
    stats->packets_received = g_packets_received;
    stats->errors = g_errors;
    stats->sockets_created = g_sockets_created;
    stats->sockets_closed = g_sockets_closed;
    stats->current_sockets = g_current_sockets;
    stats->peak_sockets = g_peak_sockets;
    orbisjvm_mutex_unlock(g_stats_mutex);
}