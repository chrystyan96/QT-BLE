cmake_minimum_required(VERSION 3.21)

project(BLE_Test VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.9 REQUIRED COMPONENTS Core Gui Qml Quick Bluetooth)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(appBLE_Test
    main.cpp
    SecurityUtils.h SecurityUtils.cpp
    PresenceAdvertiser.h PresenceAdvertiser.cpp
    PresenceScanner.h PresenceScanner.cpp
)

qt_add_qml_module(appBLE_Test
    URI BLE_Test              # <- este URI é o que você importa/carrega
    VERSION 1.0
    QML_FILES
        Main.qml
        SOURCES BlePresenceConfig.h
)

target_link_libraries(appBLE_Test
    PRIVATE Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::Bluetooth
)

# Aponta a pasta android/ (Manifest, gradle.properties etc.)
set_target_properties(appBLE_Test PROPERTIES
    QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android"
)

# (Instalação desktop opcional)
include(GNUInstallDirs)
install(TARGETS appBLE_Test
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
