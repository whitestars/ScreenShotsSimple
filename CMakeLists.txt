cmake_minimum_required(VERSION 3.20)

project(ScreenShotApp LANGUAGES CXX RC)

if(NOT MSVC)
    message(FATAL_ERROR "ScreenShotApp 仅支持 MSVC (Visual Studio 2022) 构建")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_executable(ScreenShotApp WIN32
    src/main.cpp
    src/resources.rc
    src/app/Application.cpp
    src/app/Application.h
    src/app/AutoStart.cpp
    src/app/AutoStart.h
    src/app/HotkeyManager.cpp
    src/app/HotkeyManager.h
    src/app/Settings.cpp
    src/app/Settings.h
    src/app/SettingsWindow.cpp
    src/app/SettingsWindow.h
    src/capture/ImageEncoder.cpp
    src/capture/ImageEncoder.h
    src/capture/ScreenCapture.cpp
    src/capture/ScreenCapture.h
    src/overlay/OverlayWindow.cpp
    src/overlay/OverlayWindow.h
    src/overlay/SelectionController.cpp
    src/overlay/SelectionController.h
    src/overlay/Toolbar.cpp
    src/overlay/Toolbar.h
    src/annotation/IAnnotation.h
    src/annotation/Shapes.cpp
    src/annotation/Shapes.h
    src/annotation/AnnotationStack.cpp
    src/annotation/AnnotationStack.h
    src/common/DpiHelper.cpp
    src/common/DpiHelper.h
    src/common/Win32Types.h
)

target_include_directories(ScreenShotApp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_compile_definitions(ScreenShotApp PRIVATE
    UNICODE
    _UNICODE
    NOMINMAX
    _CRT_SECURE_NO_WARNINGS
    _WIN32_WINNT=0x0A00
)

target_compile_options(ScreenShotApp PRIVATE
    /utf-8
    /W3
    /permissive-
    /Gy
    $<$<CONFIG:Release>:/O1>
    $<$<CONFIG:Release>:/GL>
)

target_link_options(ScreenShotApp PRIVATE
    /SUBSYSTEM:WINDOWS
    $<$<CONFIG:Release>:/LTCG>
    $<$<CONFIG:Release>:/OPT:REF>
    $<$<CONFIG:Release>:/OPT:ICF>
)

target_link_libraries(ScreenShotApp PRIVATE
    gdiplus.lib
    windowscodecs.lib
    user32.lib
    gdi32.lib
    shell32.lib
    ole32.lib
    shlwapi.lib
    comdlg32.lib
)

set_target_properties(ScreenShotApp PROPERTIES
    OUTPUT_NAME "ScreenShotApp"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
)
