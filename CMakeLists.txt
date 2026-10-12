cmake_minimum_required(VERSION 3.21)

project(Azimuth_Elevation VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(QT_QML_GENERATE_QMLLS_INI ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS
    Core
    Gui
    Qml
    Quick
    QuickControls2
    SerialPort
    Network
)

# Optional Qt6 Test component for unit tests
find_package(Qt6 COMPONENTS Test QUIET)

qt_standard_project_setup(REQUIRES 6.5)
qt_policy(SET QTP0004 OLD)

set(PROJECT_SOURCES
    main.cpp
    src/PacketProtocol.h
    src/PacketProtocol.cpp
    src/ITransport.h
    src/SerialTransport.h
    src/SerialTransport.cpp
    src/UdpTransport.h
    src/UdpTransport.cpp
    src/TcpTransport.h
    src/TcpTransport.cpp
    src/LogModel.h
    src/LogModel.cpp
    src/ReceiverController.h
    src/ReceiverController.cpp
)

qt_add_executable(appAzimuth_Elevation
    ${PROJECT_SOURCES}
)

qt_add_qml_module(appAzimuth_Elevation
    URI Azimuth_Elevation
    VERSION 1.0
    QML_FILES
        Main.qml
        components/AngleDial.qml
        components/ElevationDial.qml
        components/CommunicationPanel.qml
        components/PacketAnalyzer.qml
        components/ReceiveLog.qml
)

set_target_properties(appAzimuth_Elevation PROPERTIES
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appAzimuth_Elevation
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::SerialPort
        Qt6::Network
)

target_include_directories(appAzimuth_Elevation PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Unit Tests Target
if (TARGET Qt6::Test)
    enable_testing()

    add_executable(ReceiverTests
        tests/tst_protocol.cpp
        src/PacketProtocol.h
        src/PacketProtocol.cpp
    )

    target_link_libraries(ReceiverTests
        PRIVATE
            Qt6::Core
            Qt6::Test
    )

    target_include_directories(ReceiverTests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    add_test(NAME ReceiverTests COMMAND ReceiverTests)
endif()
