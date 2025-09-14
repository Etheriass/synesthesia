# Add minimp3 library as an interface target because it's header-only
add_library(minimp3 INTERFACE)
target_include_directories(minimp3 INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/minimp3)

# ---- GLAD v0 (local) ----
enable_language(C)
set(GLAD_DIR ${CMAKE_SOURCE_DIR}/external/glad)
set(GLAD_SRC ${GLAD_DIR}/src/glad.c)

add_library(glad STATIC ${GLAD_SRC})
target_include_directories(glad PUBLIC ${GLAD_DIR}/include)

set_source_files_properties(${GLAD_SRC} PROPERTIES LANGUAGE C)
set_property(TARGET glad PROPERTY LINKER_LANGUAGE C)