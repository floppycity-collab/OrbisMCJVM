# Networking Tests

add_executable(test_networking_tcp
    test_networking_tcp.c
)

target_link_libraries(test_networking_tcp PRIVATE orbisjvm)

add_test(NAME networking_tcp COMMAND test_networking_tcp)

add_executable(test_networking_udp
    test_networking_udp.c
)

target_link_libraries(test_networking_udp PRIVATE orbisjvm)

add_test(NAME networking_udp COMMAND test_networking_udp)

add_executable(test_networking_dns
    test_networking_dns.c
)

target_link_libraries(test_networking_dns PRIVATE orbisjvm)

add_test(NAME networking_dns COMMAND test_networking_dns)