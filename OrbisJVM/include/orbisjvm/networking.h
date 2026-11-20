/**
 * @file networking.h
 * @brief OrbisJVM Networking API
 *
 * Provides socket operations compatible with Orbis OS Net API.
 * Critical for Minecraft server networking support.
 */

#ifndef ORBISJVM_NETWORKING_H
#define ORBISJVM_NETWORKING_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Socket handle
 */
typedef int32_t orbisjvm_socket_t;

#define ORBISJVM_INVALID_SOCKET (-1)

/**
 * @brief Address family
 */
typedef enum {
    ORBISJVM_AF_UNSPEC = 0,
    ORBISJVM_AF_INET   = 2,   ///< IPv4
    ORBISJVM_AF_INET6  = 10,  ///< IPv6
} orbisjvm_address_family_t;

/**
 * @brief Socket type
 */
typedef enum {
    ORBISJVM_SOCK_STREAM = 1,  ///< TCP
    ORBISJVM_SOCK_DGRAM  = 2,  ///< UDP
    ORBISJVM_SOCK_RAW    = 3,  ///< Raw socket
} orbisjvm_socket_type_t;

/**
 * @brief Protocol
 */
typedef enum {
    ORBISJVM_IPPROTO_IP   = 0,
    ORBISJVM_IPPROTO_TCP  = 6,
    ORBISJVM_IPPROTO_UDP  = 17,
    ORBISJVM_IPPROTO_IPV6 = 41,
} orbisjvm_protocol_t;

/**
 * @brief IPv4 address
 */
typedef struct {
    uint32_t s_addr;  ///< Network byte order
} orbisjvm_in_addr_t;

/**
 * @brief IPv6 address
 */
typedef struct {
    uint8_t s6_addr[16];
} orbisjvm_in6_addr_t;

/**
 * @brief Socket address (IPv4)
 */
typedef struct {
    uint16_t sin_family;  ///< AF_INET
    uint16_t sin_port;    ///< Network byte order
    orbisjvm_in_addr_t sin_addr;
    char sin_zero[8];
} orbisjvm_sockaddr_in_t;

/**
 * @brief Socket address (IPv6)
 */
typedef struct {
    uint16_t sin6_family;   ///< AF_INET6
    uint16_t sin6_port;     ///< Network byte order
    uint32_t sin6_flowinfo;
    orbisjvm_in6_addr_t sin6_addr;
    uint32_t sin6_scope_id;
} orbisjvm_sockaddr_in6_t;

/**
 * @brief Generic socket address
 */
typedef struct {
    uint16_t sa_family;
    char sa_data[14];
} orbisjvm_sockaddr_t;

/**
 * @brief Socket address storage (large enough for any address)
 */
typedef struct {
    uint16_t ss_family;
    char ss_padding[126];
} orbisjvm_sockaddr_storage_t;

/**
 * @brief I/O vector for scatter/gather
 */
typedef struct {
    void* iov_base;
    size_t iov_len;
} orbisjvm_iovec_t;

/**
 * @brief Message header for sendmsg/recvmsg
 */
typedef struct {
    void* msg_name;
    uint32_t msg_namelen;
    orbisjvm_iovec_t* msg_iov;
    int32_t msg_iovlen;
    void* msg_control;
    uint32_t msg_controllen;
    int32_t msg_flags;
} orbisjvm_msghdr_t;

/**
 * @brief DNS info
 */
typedef struct {
    orbisjvm_in_addr_t primary_dns;
    orbisjvm_in_addr_t secondary_dns;
} orbisjvm_dns_info_t;

/**
 * @brief Network interface entry
 */
typedef struct {
    char name[16];
    uint32_t ip_address;
    uint8_t mac_address[6];
    uint32_t flags;
    uint32_t mtu;
} orbisjvm_if_entry_t;

/**
 * @brief Socket options
 */
typedef enum {
    ORBISJVM_SO_DEBUG        = 1,
    ORBISJVM_SO_REUSEADDR    = 2,
    ORBISJVM_SO_KEEPALIVE    = 8,
    ORBISJVM_SO_DONTROUTE    = 16,
    ORBISJVM_SO_BROADCAST    = 32,
    ORBISJVM_SO_LINGER       = 128,
    ORBISJVM_SO_OOBINLINE    = 256,
    ORBISJVM_SO_SNDBUF       = 4097,
    ORBISJVM_SO_RCVBUF       = 4098,
    ORBISJVM_SO_SNDLOWAT     = 4099,
    ORBISJVM_SO_RCVLOWAT     = 4100,
    ORBISJVM_SO_SNDTIMEO     = 4101,
    ORBISJVM_SO_RCVTIMEO     = 4102,
    ORBISJVM_SO_ERROR        = 4103,
    ORBISJVM_SO_TYPE         = 4104,
    ORBISJVM_SO_NOSIGPIPE    = 4105,
    ORBISJVM_SO_REUSEPORT    = 4106,
    ORBISJVM_TCP_NODELAY     = 1,   ///< IPPROTO_TCP level
    ORBISJVM_TCP_KEEPALIVE   = 2,   ///< IPPROTO_TCP level
} orbisjvm_sockopt_t;

/**
 * @brief Socket level for getsockopt/setsockopt
 */
typedef enum {
    ORBISJVM_SOL_SOCKET   = 0xFFFF,
    ORBISJVM_IPPROTO_TCP  = 6,
    ORBISJVM_IPPROTO_UDP  = 17,
    ORBISJVM_IPPROTO_IP   = 0,
    ORBISJVM_IPPROTO_IPV6 = 41,
} orbisjvm_socklevel_t;

/**
 * @brief Initialize networking subsystem
 *
 * Must be called before any other networking functions.
 *
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_net_init(void);

/**
 * @brief Shutdown networking subsystem
 */
void orbisjvm_net_shutdown(void);

/**
 * @brief Create a socket
 *
 * @param domain Address family (AF_INET, AF_INET6)
 * @param type Socket type (SOCK_STREAM, SOCK_DGRAM)
 * @param protocol Protocol (0 for default)
 * @return Socket handle, or ORBISJVM_INVALID_SOCKET on failure
 */
orbisjvm_socket_t orbisjvm_socket(int domain, int type, int protocol);

/**
 * @brief Close a socket
 *
 * @param sock Socket handle
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_close(orbisjvm_socket_t sock);

/**
 * @brief Bind socket to address
 *
 * @param sock Socket handle
 * @param addr Socket address
 * @param addrlen Address length
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_bind(orbisjvm_socket_t sock, const orbisjvm_sockaddr_t* addr, uint32_t addrlen);

/**
 * @brief Listen for connections on socket
 *
 * @param sock Socket handle
 * @param backlog Maximum pending connections
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_listen(orbisjvm_socket_t sock, int backlog);

/**
 * @brief Accept a connection
 *
 * @param sock Listening socket
 * @param[out] addr Client address (or NULL)
 * @param[out] addrlen Client address length (or NULL)
 * @return New socket handle, or ORBISJVM_INVALID_SOCKET on failure
 */
orbisjvm_socket_t orbisjvm_accept(orbisjvm_socket_t sock, orbisjvm_sockaddr_t* addr, uint32_t* addrlen);

/**
 * @brief Connect to remote address
 *
 * @param sock Socket handle
 * @param addr Remote address
 * @param addrlen Address length
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_connect(orbisjvm_socket_t sock, const orbisjvm_sockaddr_t* addr, uint32_t addrlen);

/**
 * @brief Send data on connected socket
 *
 * @param sock Socket handle
 * @param buffer Data buffer
 * @param length Data length
 * @param flags Send flags
 * @return Bytes sent, or negative error code on failure
 */
int32_t orbisjvm_send(orbisjvm_socket_t sock, const void* buffer, size_t length, int flags);

/**
 * @brief Receive data from connected socket
 *
 * @param sock Socket handle
 * @param buffer Receive buffer
 * @param length Buffer length
 * @param flags Receive flags
 * @return Bytes received, 0 on EOF, or negative error code on failure
 */
int32_t orbisjvm_recv(orbisjvm_socket_t sock, void* buffer, size_t length, int flags);

/**
 * @brief Send data to specific address (UDP)
 *
 * @param sock Socket handle
 * @param buffer Data buffer
 * @param length Data length
 * @param flags Send flags
 * @param addr Destination address
 * @param addrlen Address length
 * @return Bytes sent, or negative error code on failure
 */
int32_t orbisjvm_sendto(orbisjvm_socket_t sock, const void* buffer, size_t length, int flags,
                        const orbisjvm_sockaddr_t* addr, uint32_t addrlen);

/**
 * @brief Receive data from specific address (UDP)
 *
 * @param sock Socket handle
 * @param buffer Receive buffer
 * @param length Buffer length
 * @param flags Receive flags
 * @param[out] addr Source address (or NULL)
 * @param[out] addrlen Source address length (or NULL)
 * @return Bytes received, or negative error code on failure
 */
int32_t orbisjvm_recvfrom(orbisjvm_socket_t sock, void* buffer, size_t length, int flags,
                          orbisjvm_sockaddr_t* addr, uint32_t* addrlen);

/**
 * @brief Send vectored data
 *
 * @param sock Socket handle
 * @param iov I/O vectors
 * @param iovcnt Number of vectors
 * @param flags Send flags
 * @return Bytes sent, or negative error code on failure
 */
int32_t orbisjvm_sendmsg(orbisjvm_socket_t sock, const orbisjvm_msghdr_t* msg, int flags);

/**
 * @brief Receive vectored data
 *
 * @param sock Socket handle
 * @param msg Message header
 * @param flags Receive flags
 * @return Bytes received, or negative error code on failure
 */
int32_t orbisjvm_recvmsg(orbisjvm_socket_t sock, orbisjvm_msghdr_t* msg, int flags);

/**
 * @brief Shutdown socket
 *
 * @param sock Socket handle
 * @param how Shutdown mode (0=read, 1=write, 2=both)
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_shutdown(orbisjvm_socket_t sock, int how);

/**
 * @brief Get socket option
 *
 * @param sock Socket handle
 * @param level Option level (SOL_SOCKET, IPPROTO_TCP, etc.)
 * @param optname Option name
 * @param[out] optval Option value
 * @param[out] optlen Option length
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_getsockopt(orbisjvm_socket_t sock, int level, int optname, void* optval, uint32_t* optlen);

/**
 * @brief Set socket option
 *
 * @param sock Socket handle
 * @param level Option level
 * @param optname Option name
 * @param optval Option value
 * @param optlen Option length
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_setsockopt(orbisjvm_socket_t sock, int level, int optname, const void* optval, uint32_t optlen);

/**
 * @brief Get local socket address
 *
 * @param sock Socket handle
 * @param[out] addr Local address
 * @param[out] addrlen Address length
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_getsockname(orbisjvm_socket_t sock, orbisjvm_sockaddr_t* addr, uint32_t* addrlen);

/**
 * @brief Get peer socket address
 *
 * @param sock Socket handle
 * @param[out] addr Peer address
 * @param[out] addrlen Address length
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_getpeername(orbisjvm_socket_t sock, orbisjvm_sockaddr_t* addr, uint32_t* addrlen);

/**
 * @brief Convert host byte order to network byte order (32-bit)
 *
 * @param host 32-bit value in host byte order
 * @return Value in network byte order
 */
uint32_t orbisjvm_htonl(uint32_t host);

/**
 * @brief Convert network byte order to host byte order (32-bit)
 *
 * @param net 32-bit value in network byte order
 * @return Value in host byte order
 */
uint32_t orbisjvm_ntohl(uint32_t net);

/**
 * @brief Convert host byte order to network byte order (16-bit)
 *
 * @param host 16-bit value in host byte order
 * @return Value in network byte order
 */
uint16_t orbisjvm_htons(uint16_t host);

/**
 * @brief Convert network byte order to host byte order (16-bit)
 *
 * @param net 16-bit value in network byte order
 * @return Value in host byte order
 */
uint16_t orbisjvm_ntohs(uint16_t net);

/**
 * @brief Convert host byte order to network byte order (64-bit)
 *
 * @param host 64-bit value in host byte order
 * @return Value in network byte order
 */
uint64_t orbisjvm_htonll(uint64_t host);

/**
 * @brief Convert network byte order to host byte order (64-bit)
 *
 * @param net 64-bit value in network byte order
 * @return Value in host byte order
 */
uint64_t orbisjvm_ntohll(uint64_t net);

/**
 * @brief Convert IPv4 address from text to binary
 *
 * @param af Address family (AF_INET)
 * @param src String representation
 * @param dst Binary address output
 * @return 1 on success, 0 on invalid format, negative error code on failure
 */
int orbisjvm_inet_pton(int af, const char* src, void* dst);

/**
 * @brief Convert IPv4 address from binary to text
 *
 * @param af Address family (AF_INET)
 * @param src Binary address
 * @param dst String buffer
 * @param size Buffer size
 * @return String pointer on success, NULL on failure
 */
const char* orbisjvm_inet_ntop(int af, const void* src, char* dst, uint32_t size);

/**
 * @brief Resolve hostname to IPv4 address
 *
 * @param hostname Hostname to resolve
 * @param[out] addr Resolved IPv4 address
 * @param timeout Timeout in milliseconds
 * @param retry Retry count
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_resolve(const char* hostname, orbisjvm_in_addr_t* addr, int timeout, int retry);

/**
 * @brief Resolve hostname to multiple IPv4 addresses
 *
 * @param hostname Hostname to resolve
 * @param[out] addrs Array of resolved addresses
 * @param max_addrs Maximum addresses to return
 * @param[out] count Actual number of addresses returned
 * @param timeout Timeout in milliseconds
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_resolve_multiple(const char* hostname, orbisjvm_in_addr_t* addrs,
                              int max_addrs, int* count, int timeout);

/**
 * @brief Get DNS configuration
 *
 * @param[out] info DNS info structure
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_get_dns_info(orbisjvm_dns_info_t* info);

/**
 * @brief Set DNS configuration
 *
 * @param info DNS info structure
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_set_dns_info(const orbisjvm_dns_info_t* info);

/**
 * @brief Get network interface list
 *
 * @param[out] interfaces Array of interface entries
 * @param max_interfaces Maximum interfaces to return
 * @param[out] count Actual number of interfaces
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_get_interfaces(orbisjvm_if_entry_t* interfaces, int max_interfaces, int* count);

/**
 * @brief Get MAC address
 *
 * @param[out] mac 6-byte MAC address
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_get_mac_address(uint8_t mac[6]);

/**
 * @brief Get last socket error
 *
 * @return Error code
 */
int orbisjvm_get_last_error(void);

/**
 * @brief Set socket non-blocking mode
 *
 * @param sock Socket handle
 * @param nonblock 1 for non-blocking, 0 for blocking
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_set_nonblocking(orbisjvm_socket_t sock, int nonblock);

/**
 * @brief Check if socket is non-blocking
 *
 * @param sock Socket handle
 * @return 1 if non-blocking, 0 if blocking, negative error code on failure
 */
int orbisjvm_get_nonblocking(orbisjvm_socket_t sock);

/**
 * @brief Wait for socket events (poll equivalent)
 *
 * @param fds Array of pollfd structures
 * @param nfds Number of entries
 * @param timeout Timeout in milliseconds (-1 = infinite)
 * @return Number of ready descriptors, 0 on timeout, negative error code on failure
 */
typedef struct {
    orbisjvm_socket_t fd;
    int16_t events;    ///< Requested events
    int16_t revents;   ///< Returned events
} orbisjvm_pollfd_t;

#define ORBISJVM_POLLIN    (1 << 0)  ///< Data available to read
#define ORBISJVM_POLLOUT   (1 << 1)  ///< Ready to write
#define ORBISJVM_POLLERR   (1 << 2)  ///< Error condition
#define ORBISJVM_POLLHUP   (1 << 3)  ///< Hang up
#define ORBISJVM_POLLNVAL  (1 << 4)  ///< Invalid request

int orbisjvm_poll(orbisjvm_pollfd_t* fds, int nfds, int timeout);

/**
 * @brief Network statistics (for diagnostics)
 */
typedef struct {
    uint64_t bytes_sent;
    uint64_t bytes_received;
    uint64_t packets_sent;
    uint64_t packets_received;
    uint64_t errors;
    uint32_t sockets_created;
    uint32_t sockets_closed;
    uint32_t current_sockets;
    uint32_t peak_sockets;
} orbisjvm_net_stats_t;

/**
 * @brief Get network statistics
 *
 * @param[out] stats Output statistics
 */
void orbisjvm_get_net_stats(orbisjvm_net_stats_t* stats);

#ifdef __cplusplus
}
#endif

#endif // ORBISJVM_NETWORKING_H