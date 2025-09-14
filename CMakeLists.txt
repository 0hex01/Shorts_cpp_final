cmake_minimum_required(VERSION 3.16)
project(shortcut_manager VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Use system Qt 6.4.2
set(QT_VERSION 6.4.2)
set(CMAKE_PREFIX_PATH "/usr/lib/x86_64-linux-gnu/cmake/Qt6")

# Find required Qt components
find_package(Qt6 ${QT_VERSION} COMPONENTS Core Gui Widgets REQUIRED)

# Set environment to use system Qt
set(ENV{PATH} "/usr/lib/qt6/bin:$ENV{PATH}")
set(ENV{LD_LIBRARY_PATH} "/usr/lib/x86_64-linux-gnu:$ENV{LD_LIBRARY_PATH}")
set(ENV{QT_PLUGIN_PATH} "/usr/lib/x86_64-linux-gnu/qt6/plugins")

# Set Qt tools to use system versions
set(QT_QMAKE_EXECUTABLE "/usr/lib/qt6/bin/qmake")
set(QT_UIC_EXECUTABLE "/usr/lib/qt6/libexec/uic")
set(QT_MOC_EXECUTABLE "/usr/lib/qt6/libexec/moc")
set(QT_RCC_EXECUTABLE "/usr/lib/qt6/libexec/rcc")

# Add source files
set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    resources.qrc
    src/mainwindow.h
)

# Add the executable
add_executable(shorts ${SOURCES})

# Set the target properties for the executable
set_target_properties(shorts PROPERTIES
    OUTPUT_NAME "shorts"
    WIN32_EXECUTABLE FALSE
    MACOSX_BUNDLE FALSE
)

# Link against system Qt 6.4.2
target_link_directories(shorts PRIVATE /usr/lib/x86_64-linux-gnu)
target_link_libraries(shorts PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

# Set RPATH to use system libraries
set_target_properties(shorts PROPERTIES
    INSTALL_RPATH "/usr/lib/x86_64-linux-gnu"
    BUILD_WITH_INSTALL_RPATH TRUE
)

# Set the application icon (simplified for Linux)
if(UNIX AND NOT APPLE)
    # Install desktop file for Linux
    configure_file(
        ${CMAKE_SOURCE_DIR}/shorts.desktop.in
        ${CMAKE_CURRENT_BINARY_DIR}/shorts.desktop
        @ONLY
    )
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/shorts.desktop
        DESTINATION share/applications
    )
    
    # Install icon
    install(FILES ${CMAKE_SOURCE_DIR}/shorts.png
        DESTINATION share/icons/hicolor/512x512/apps
        RENAME shorts.png
    )
endif()

# Set RPATH for Linux
if(UNIX AND NOT APPLE)
    set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib")
    set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
endif()

# Installation (local to project directory)
set(INSTALL_BIN_DIR ${CMAKE_SOURCE_DIR}/bin)
file(MAKE_DIRECTORY ${INSTALL_BIN_DIR})

install(TARGETS shorts
    RUNTIME DESTINATION ${INSTALL_BIN_DIR}
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)

# Create a run script that uses sudo
file(WRITE ${CMAKE_BINARY_DIR}/run_shorts.sh "#!/bin/bash\n# Run with sudo if not root\nif [ "$(id -u)" -ne 0 ]; then\n    exec sudo "$0" "$@"\nfi\n\n# Change to script directory\ncd \"$(dirname \"$0\")\"\n\n# Run the application\n${INSTALL_BIN_DIR}/shorts "$@"\n")

# Install the run script
file(COPY ${CMAKE_BINARY_DIR}/run_shorts.sh DESTINATION ${CMAKE_SOURCE_DIR} FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

# Copy resources
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/resources DESTINATION ${CMAKE_BINARY_DIR})
