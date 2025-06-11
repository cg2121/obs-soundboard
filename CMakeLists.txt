cmake_minimum_required(VERSION 3.16...3.26)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

project(${_name} VERSION ${_version})

option(ENABLE_FRONTEND_API "Use obs-frontend-api for UI functionality" OFF)
option(ENABLE_QT "Use Qt functionality" OFF)

include(compilerconfig)
include(defaults)
include(helpers)

add_library(${CMAKE_PROJECT_NAME} MODULE)

find_package(libobs REQUIRED)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::libobs)

if(ENABLE_FRONTEND_API)
  find_package(obs-frontend-api REQUIRED)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::obs-frontend-api)
endif()

if(ENABLE_QT)
  find_package(Qt6 COMPONENTS Widgets Core)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Core Qt6::Widgets)
  target_compile_options(
    ${CMAKE_PROJECT_NAME} PRIVATE $<$<C_COMPILER_ID:Clang,AppleClang>:-Wno-quoted-include-in-framework-header
                                  -Wno-comma>)
  set_target_properties(
    ${CMAKE_PROJECT_NAME}
    PROPERTIES AUTOMOC ON
               AUTOUIC ON
               AUTORCC ON)

  set_property(TARGET  ${CMAKE_PROJECT_NAME} APPEND PROPERTY AUTOUIC_SEARCH_PATHS src/forms)
endif()

target_sources(
  ${CMAKE_PROJECT_NAME}
  PRIVATE src/components/AbsoluteSlider.cpp
          src/components/AbsoluteSlider.hpp
          src/components/ClickableLabel.hpp
          src/components/MediaControls.cpp
          src/components/MediaControls.hpp
          src/components/SceneTree.cpp
          src/components/SceneTree.hpp
          src/dialogs/MediaEdit.cpp
          src/dialogs/MediaEdit.hpp
          src/forms/MediaControls.ui
          src/forms/MediaEdit.ui
          src/forms/Soundboard.ui
          src/models/MediaData.cpp
          src/models/MediaData.hpp
          src/plugin-support.c.in
          src/plugin-support.h
          src/Soundboard.cpp
          src/Soundboard.hpp)

set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${_name})
