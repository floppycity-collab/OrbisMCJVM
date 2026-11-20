# UI CMakeLists.txt

set(UI_SOURCES
    ui.c
)

add_library(orbismc_ui STATIC ${UI_SOURCES})
target_link_libraries(orbismc_ui PUBLIC orbisjvm orbismc_app)

target_include_directories(orbismc_ui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/../include
    ${CMAKE_SOURCE_DIR}/../app
)