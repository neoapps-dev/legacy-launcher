cmake_minimum_required(VERSION 3.16)
project(LegacyLauncher VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)

set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/InstanceManager.cpp
    src/ProtonDetector.cpp
    src/GitHubReleaseTracker.cpp
    src/WeaveLoaderReleaseTracker.cpp
    src/Downloader.cpp
    src/LaunchManager.cpp
    src/AddInstanceDialog.cpp
    src/SettingsDialog.cpp
    src/InstanceWidget.cpp
    src/UpdateInstanceDialog.cpp
)

set(HEADERS
    include/MainWindow.h
    include/InstanceManager.h
    include/ProtonDetector.h
    include/GitHubReleaseTracker.h
    include/WeaveLoaderReleaseTracker.h
    include/Downloader.h
    include/LaunchManager.h
    include/AddInstanceDialog.h
    include/SettingsDialog.h
    include/InstanceWidget.h
    include/UpdateInstanceDialog.h
    include/Types.h
)

add_executable(legacy-launcher ${SOURCES} ${HEADERS} resources.qrc)

target_include_directories(legacy-launcher PRIVATE include)

target_link_libraries(legacy-launcher PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
)

install(TARGETS legacy-launcher DESTINATION bin)
