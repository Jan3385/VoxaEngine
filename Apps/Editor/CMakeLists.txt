project(VoxaEditor LANGUAGES C CXX)

file(GLOB_RECURSE EDITOR_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_executable(VoxaEditor ${EDITOR_SOURCES})

target_include_directories(VoxaEngine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(VoxaEditor PRIVATE VoxaEngine)

set_property(TARGET VoxaEditor PROPERTY CXX_STANDARD 20)

set(FONTS_FOLDER "${CMAKE_SOURCE_DIR}/Fonts")
set(RENDER_SHADERS_FOLDER "${CMAKE_SOURCE_DIR}/Engine/Shader/Rendering/RenderShaders")
set(COMPUTE_SHADERS_FOLDER "${CMAKE_SOURCE_DIR}/Engine/Shader/Computing/ComputeShaders")
set(INCLUDE_SHADERS_FOLDER "${CMAKE_SOURCE_DIR}/Engine/Shader/Includes")

add_custom_target(copy_assets ALL
  # Fonts
  COMMAND ${CMAKE_COMMAND} -E rm -rf $<TARGET_FILE_DIR:VoxaEditor>/Fonts
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${FONTS_FOLDER} $<TARGET_FILE_DIR:VoxaEditor>/Fonts

  # Shaders
  COMMAND ${CMAKE_COMMAND} -E rm -rf $<TARGET_FILE_DIR:VoxaEditor>/Shaders
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${RENDER_SHADERS_FOLDER} $<TARGET_FILE_DIR:VoxaEditor>/Shaders/RenderShaders
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${COMPUTE_SHADERS_FOLDER} $<TARGET_FILE_DIR:VoxaEditor>/Shaders/ComputeShaders
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${INCLUDE_SHADERS_FOLDER} $<TARGET_FILE_DIR:VoxaEditor>/Shaders/Includes

  DEPENDS VoxaEngine
)

if(WIN32)
  add_custom_command(TARGET copy_assets POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/Engine/DLLs $<TARGET_FILE_DIR:VoxaEditor>
  )
endif()
