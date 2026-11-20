# App CMakeLists.txt

set(APP_SOURCES
    main.c
    server_manager.c
    world_manager.c
    config_manager.c
    console.c
    player_manager.c
    remote_api.c
)

add_library(orbismc_app STATIC ${APP_SOURCES})
target_link_libraries(orbismc_app PUBLIC orbisjvm)

target_include_directories(orbismc_app PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/../include
)