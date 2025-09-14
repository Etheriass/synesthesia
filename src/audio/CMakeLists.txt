add_library(audio_lib
  audio.cpp
  decoder.cpp
  fourier.cpp
)

# Public headers advertised to dependents
target_sources(audio_lib
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${CMAKE_CURRENT_LIST_DIR}
    FILES
      audio.h
      decoder.h
      fourier.h
      helpers.h
      player.h
      spatial.h
)

# Consumers include this folder when they link audio_lib
target_include_directories(audio_lib
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

target_compile_features(audio_lib PUBLIC cxx_std_23)