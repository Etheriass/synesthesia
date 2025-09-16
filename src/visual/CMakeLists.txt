add_library(visualizer_lib visual.cpp)

target_sources(visualizer_lib
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${CMAKE_CURRENT_LIST_DIR}
    FILES
      circle.h
      visual.h
      init.h
      loader.h
)

# Public include path for headers
target_include_directories(visualizer_lib
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

# Link to the graphics deps provided by external/
target_link_libraries(visualizer_lib
  PUBLIC
    glad
    glfw           # or glfw3 / GLFW if that is the target name
)

target_compile_features(visualizer_lib PUBLIC cxx_std_23)