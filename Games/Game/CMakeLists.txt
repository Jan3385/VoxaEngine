project(VoxaGame LANGUAGES C CXX)

file(GLOB_RECURSE GAME_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_executable(VoxaGame ${GAME_SOURCES})

target_include_directories(VoxaEngine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(VoxaGame PRIVATE VoxaEngine)

set_property(TARGET VoxaGame PROPERTY CXX_STANDARD 20)

set(FONTS_FOLDER "${CMAKE_SOURCE_DIR}/Fonts")
set(TEXTURES_FOLDER "${CMAKE_SOURCE_DIR}/Textures")
set(RENDER_SHADERS_FOLDER "${CMAKE_SOURCE_DIR}/Engine/Shader/Rendering/RenderShaders")
set(COMPUTE_SHADERS_FOLDER "${CMAKE_SOURCE_DIR}/Engine/Shader/Computing/ComputeShaders")
set(INCLUDE_SHADERS_FOLDER "${CMAKE_SOURCE_DIR}/Engine/Shader/Includes")

add_custom_target(copy_assets ALL
  COMMAND ${CMAKE_COMMAND} -E rm -rf $<TARGET_FILE_DIR:VoxaGame>/Fonts
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${FONTS_FOLDER} $<TARGET_FILE_DIR:VoxaGame>/Fonts
  COMMAND ${CMAKE_COMMAND} -E rm -rf $<TARGET_FILE_DIR:VoxaGame>/Textures
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${TEXTURES_FOLDER} $<TARGET_FILE_DIR:VoxaGame>/Textures
  COMMAND ${CMAKE_COMMAND} -E rm -rf $<TARGET_FILE_DIR:VoxaGame>/Textures/SRC
  COMMAND ${CMAKE_COMMAND} -E rm -rf $<TARGET_FILE_DIR:VoxaGame>/Shaders
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${RENDER_SHADERS_FOLDER} $<TARGET_FILE_DIR:VoxaGame>/Shaders/RenderShaders
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${COMPUTE_SHADERS_FOLDER} $<TARGET_FILE_DIR:VoxaGame>/Shaders/ComputeShaders
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${INCLUDE_SHADERS_FOLDER} $<TARGET_FILE_DIR:VoxaGame>/Shaders/Includes
  DEPENDS VoxaEngine
)

# copy DLLs
if(WIN32)
  file(GLOB VCPKG_DLLS "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin/*.dll")
  add_custom_target(copy_dlls ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      ${VCPKG_DLLS}
      $<TARGET_FILE_DIR:VoxaGame>

    COMMAND ${CMAKE_COMMAND} -E copy
      $<TARGET_FILE_DIR:VoxaEngine>/libVoxaEngine.dll
      $<TARGET_FILE_DIR:VoxaGame>

    DEPENDS VoxaEngine
  )
endif()

# TODO: linux support


