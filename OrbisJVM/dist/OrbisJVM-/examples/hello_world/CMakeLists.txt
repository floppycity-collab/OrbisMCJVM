# Hello World Example

add_executable(hello_world
    hello_world.c
)

target_link_libraries(hello_world PRIVATE orbisjvm)