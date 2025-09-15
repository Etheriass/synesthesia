add_subdirectory(audio)
add_subdirectory(visual)


add_executable(synesthesia main.cpp)

# Link the two modules
target_link_libraries(synesthesia
  PRIVATE
    audio_lib
    visualizer_lib
)

# Optional: set warnings, LTO, etc. for the app
# target_compile_features(synesthesia PRIVATE cxx_std_23)


# (Optional) copy runtime assets like shaders next to the binary
add_custom_command(TARGET synesthesia POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_CURRENT_LIST_DIR}/visual/shaders
          $<TARGET_FILE_DIR:synesthesia>/shaders
  COMMENT "Copying shaders to runtime dir")


# Enable AddressSanitizer for debugging (if supported by compiler)
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)

function(enable_asan target)
  if (ENABLE_ASAN AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(${target} PRIVATE -fsanitize=thread -fno-omit-frame-pointer -g)
    target_link_options(${target} PRIVATE -fsanitize=thread)
  endif()
endfunction()
enable_asan(synesthesia)
