cmake_minimum_required(VERSION 3.21)
project(CaptureForge VERSION 1.0.3 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)

find_package(Qt6 REQUIRED COMPONENTS Widgets Concurrent Network Svg)
include(FetchContent)
FetchContent_Declare(qhotkey
 GIT_REPOSITORY https://github.com/Skycoder42/QHotkey.git
 GIT_TAG 1.5.0)
FetchContent_MakeAvailable(qhotkey)

qt_add_executable(CaptureForge WIN32
 src/main.cpp
 src/Controller.cpp src/Controller.h
 src/CaptureOverlay.cpp src/CaptureOverlay.h
 src/editors/ImageEditor.cpp src/editors/ImageEditor.h
 src/editors/TextEditor.cpp src/editors/TextEditor.h
 src/settings/SettingsDialog.cpp src/settings/SettingsDialog.h
 src/themes/ThemeManager.cpp src/themes/ThemeManager.h
 src/plugins/PluginManager.cpp src/plugins/PluginManager.h src/plugins/PluginInterface.h
 src/ocr/Ocr.cpp src/ocr/Ocr.h
 src/updates/Updater.cpp src/updates/Updater.h
 src/ui/Icons.cpp src/ui/Icons.h
)
if(WIN32)
 target_sources(CaptureForge PRIVATE windows.rc)
endif()

target_include_directories(CaptureForge PRIVATE src)
target_link_libraries(CaptureForge PRIVATE Qt6::Widgets Qt6::Concurrent Qt6::Network Qt6::Svg QHotkey::QHotkey)

# Future video:
# find_package(Qt6 REQUIRED COMPONENTS Multimedia MultimediaWidgets)
# target_link_libraries(CaptureForge PRIVATE Qt6::Multimedia Qt6::MultimediaWidgets)

target_compile_definitions(CaptureForge PRIVATE CAPTUREFORGE_VERSION="${PROJECT_VERSION}")
