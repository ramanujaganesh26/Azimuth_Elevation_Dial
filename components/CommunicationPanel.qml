import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    color: "#131d31"
    border.color: "#233d66"
    border.width: 1
    radius: 12

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 10

        // Header with Transport Mode Buttons
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Label {
                text: "INTERFACE SELECT"
                color: "#90caf9"
                font.pixelSize: 12
                font.bold: true
                font.letterSpacing: 1.0
            }

            Item { Layout.fillWidth: true }

            // Simulation mode toggle
            Rectangle {
                height: 26
                width: 130
                radius: 4
                color: receiver.simulationActive ? "#00e67622" : "#263238"
                border.color: receiver.simulationActive ? "#00e676" : "#455a64"
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 4
                    spacing: 4

                    Rectangle {
                        width: 8
                        height: 8
                        radius: 4
                        color: receiver.simulationActive ? "#00e676" : "#78909c"
                    }

                    Label {
                        text: receiver.simulationActive ? "SIMULATION ON" : "SIMULATION OFF"
                        color: receiver.simulationActive ? "#00e676" : "#b0bec5"
                        font.pixelSize: 10
                        font.bold: true
                        Layout.fillWidth: true
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: receiver.simulationActive = !receiver.simulationActive
                }
            }
        }

        // Transport Selector Bar (Serial, UDP, TCP Client, TCP Server)
        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            Repeater {
                model: [
                    { name: "SERIAL", index: 0 },
                    { name: "UDP", index: 1 },
                    { name: "TCP CLIENT", index: 2 },
                    { name: "TCP SERVER", index: 3 }
                ]

                Button {
                    id: transportBtn
                    Layout.fillWidth: true
                    Layout.preferredHeight: 32
                    checkable: true
                    checked: receiver.selectedTransport === modelData.index
                    text: modelData.name

                    contentItem: Text {
                        text: transportBtn.text
                        color: transportBtn.checked ? "#ffffff" : "#90a4ae"
                        font.pixelSize: 11
                        font.bold: transportBtn.checked
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    background: Rectangle {
                        radius: 6
                        color: transportBtn.checked ? "#1976d2" : "#1a263d"
                        border.color: transportBtn.checked ? "#42a5f5" : "#223554"
                        border.width: 1
                    }

                    onClicked: {
                        receiver.selectedTransport = modelData.index
                    }
                }
            }
        }

        // Stacked Configuration Area
        StackLayout {
            id: configStack
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: receiver.selectedTransport

            // ================= 0: SERIAL CONFIGURATION =================
            Item {
                id: serialConfigTab

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 8

                    // Port Selection
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Label {
                            text: "Port:"
                            color: "#b0bec5"
                            font.pixelSize: 11
                            Layout.preferredWidth: 65
                        }

                        ComboBox {
                            id: portCombo
                            Layout.fillWidth: true
                            Layout.preferredHeight: 30
                            model: receiver.availableSerialPorts
                            enabled: !receiver.connected

                            onCurrentTextChanged: {
                                if (currentText.length > 0) {
                                    receiver.setSerialPort(currentText);
                                }
                            }
                            Component.onCompleted: {
                                if (count > 0) {
                                    receiver.setSerialPort(currentText);
                                }
                            }
                        }

                        Button {
                            text: "Refresh"
                            Layout.preferredHeight: 30
                            Layout.preferredWidth: 70
                            enabled: !receiver.connected
                            onClicked: {
                                receiver.refreshSerialPorts();
                                if (portCombo.count > 0) {
                                    receiver.setSerialPort(portCombo.currentText);
                                }
                            }
                        }
                    }

                    // Baud Rate & Data Bits
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Label {
                            text: "Baud:"
                            color: "#b0bec5"
                            font.pixelSize: 11
                            Layout.preferredWidth: 65
                        }

                        ComboBox {
                            id: baudCombo
                            Layout.fillWidth: true
                            Layout.preferredHeight: 30
                            model: [9600, 19200, 38400, 57600, 115200, 230400, 460800]
                            currentIndex: 4 // default 115200
                            enabled: !receiver.connected
                            onCurrentValueChanged: receiver.setSerialBaudRate(currentValue)
                        }

                        Label {
                            text: "Data:"
                            color: "#b0bec5"
                            font.pixelSize: 11
                        }

                        ComboBox {
                            id: dataBitsCombo
                            Layout.preferredWidth: 70
                            Layout.preferredHeight: 30
                            model: [8, 7, 6, 5]
                            currentIndex: 0
                            enabled: !receiver.connected
                            onCurrentValueChanged: receiver.setSerialDataBits(currentValue)
                        }
                    }

                    // Parity, Stop Bits, Flow Control
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Label {
                            text: "Parity:"
                            color: "#b0bec5"
                            font.pixelSize: 11
                            Layout.preferredWidth: 65
                        }

                        ComboBox {
                            id: parityCombo
                            Layout.fillWidth: true
                            Layout.preferredHeight: 30
                            model: ["None", "Even", "Odd", "Mark", "Space"]
                            currentIndex: 0
                            enabled: !receiver.connected
                            onCurrentIndexChanged: receiver.setSerialParity(currentIndex === 0 ? 0 : currentIndex + 1)
                        }

                        Label {
                            text: "Stop:"
                            color: "#b0bec5"
                            font.pixelSize: 11
                        }

                        ComboBox {
                            id: stopBitsCombo
                            Layout.preferredWidth: 70
                            Layout.preferredHeight: 30
                            model: ["1", "1.5", "2"]
                            currentIndex: 0
                            enabled: !receiver.connected
                            onCurrentIndexChanged: receiver.setSerialStopBits(currentIndex + 1)
                        }
                    }
                }
            }

            // ================= 1: UDP CONFIGURATION =================
            Item {
                id: udpConfigTab

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 8

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Label {
                            text: "Bind IP:"
                            color: "#b0bec5"
                            font.pixelSize: 11
                            Layout.preferredWidth: 65
                        }

                        TextField {
                            id: udpIpField
                            Layout.fillWidth: true
                            Layout.preferredHeight: 30
                            text: "0.0.0.0"
                            placeholderText: "0.0.0.0 (Any)"
                            enabled: !receiver.connected
                            color: "#ffffff"
                            onEditingFinished: receiver.setUdpBindAddress(text)
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Label {
                            text: "Port:"
                            color: "#b0bec5"
                            font.pixelSize: 11
                            Layout.preferredWidth: 65
                        }

                        TextField {
                            id: udpPortField
                            Layout.fillWidth: true
                            Layout.preferredHeight: 30
                            text: "14550"
                            placeholderText: "14550"
                            inputMethodHints: Qt.ImhDigitsOnly
                            enabled: !receiver.connected
                            color: "#ffffff"
                            onEditingFinished: receiver.setUdpBindPort(parseInt(text))
                        }
                    }

                    Label {
                        text: "Default: 0.0.0.0:14550 (Standard telemetry datagram port)"
                        color: "#607d8b"
                        font.pixelSize: 10
                    }
                }
            }

            // ================= 2: TCP CLIENT CONFIGURATION =================
            Item {
                id: tcpClientConfigTab

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 8

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Label {
                            text: "Host:"
                            color: "#b0bec5"
                            font.pixelSize: 11
                            Layout.preferredWidth: 65
                        }

                        TextField {
                            id: tcpHostField
                            Layout.fillWidth: true
                            Layout.preferredHeight: 30
                            text: "127.0.0.1"
                            placeholderText: "Remote Host / IP"
                            enabled: !receiver.connected
                            color: "#ffffff"
                            onEditingFinished: receiver.setTcpRemoteHost(text)
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Label {
                            text: "Port:"
                            color: "#b0bec5"
                            font.pixelSize: 11
                            Layout.preferredWidth: 65
                        }

                        TextField {
                            id: tcpPortField
                            Layout.fillWidth: true
                            Layout.preferredHeight: 30
                            text: "5000"
                            placeholderText: "5000"
                            inputMethodHints: Qt.ImhDigitsOnly
                            enabled: !receiver.connected
                            color: "#ffffff"
                            onEditingFinished: receiver.setTcpRemotePort(parseInt(text))
                        }
                    }

                    Label {
                        text: "Connects to remote transmitter stream server."
                        color: "#607d8b"
                        font.pixelSize: 10
                    }
                }
            }

            // ================= 3: TCP SERVER CONFIGURATION =================
            Item {
                id: tcpServerConfigTab

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 8

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Label {
                            text: "Bind IP:"
                            color: "#b0bec5"
                            font.pixelSize: 11
                            Layout.preferredWidth: 65
                        }

                        TextField {
                            id: tcpServerIpField
                            Layout.fillWidth: true
                            Layout.preferredHeight: 30
                            text: "0.0.0.0"
                            placeholderText: "0.0.0.0 (Any)"
                            enabled: !receiver.connected
                            color: "#ffffff"
                            onEditingFinished: receiver.setTcpLocalBindAddress(text)
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Label {
                            text: "Port:"
                            color: "#b0bec5"
                            font.pixelSize: 11
                            Layout.preferredWidth: 65
                        }

                        TextField {
                            id: tcpServerPortField
                            Layout.fillWidth: true
                            Layout.preferredHeight: 30
                            text: "5000"
                            placeholderText: "5000"
                            inputMethodHints: Qt.ImhDigitsOnly
                            enabled: !receiver.connected
                            color: "#ffffff"
                            onEditingFinished: receiver.setTcpLocalListeningPort(parseInt(text))
                        }
                    }

                    Label {
                        text: "Listens for inbound transmitter connections."
                        color: "#607d8b"
                        font.pixelSize: 10
                    }
                }
            }
        }

        // Connection Action Bar
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Button {
                id: connectBtn
                Layout.fillWidth: true
                Layout.preferredHeight: 34
                enabled: !receiver.connected
                text: (receiver.selectedTransport === 1 || receiver.selectedTransport === 3) ? "BIND / LISTEN" : "CONNECT"

                contentItem: Text {
                    text: connectBtn.text
                    color: connectBtn.enabled ? "#ffffff" : "#546e7a"
                    font.pixelSize: 11
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    radius: 6
                    color: connectBtn.enabled ? "#00c853" : "#1b2a3d"
                    opacity: connectBtn.enabled ? 1.0 : 0.6
                }

                onClicked: receiver.connectCurrentTransport()
            }

            Button {
                id: disconnectBtn
                Layout.fillWidth: true
                Layout.preferredHeight: 34
                enabled: receiver.connected
                text: (receiver.selectedTransport === 1 || receiver.selectedTransport === 3) ? "CLOSE" : "DISCONNECT"

                contentItem: Text {
                    text: disconnectBtn.text
                    color: disconnectBtn.enabled ? "#ffffff" : "#546e7a"
                    font.pixelSize: 11
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    radius: 6
                    color: disconnectBtn.enabled ? "#d50000" : "#1b2a3d"
                    opacity: disconnectBtn.enabled ? 1.0 : 0.6
                }

                onClicked: receiver.disconnectCurrentTransport()
            }
        }

        // Status & Diagnostics Line
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 28
            radius: 4
            color: "#0d1524"
            border.color: receiver.connected ? "#00e67644" : "#ff174444"
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.margins: 6
                spacing: 6

                Rectangle {
                    width: 8
                    height: 8
                    radius: 4
                    color: receiver.connected ? "#00e676" : "#ff1744"
                }

                Label {
                    text: receiver.connectionStatus
                    color: receiver.connected ? "#b9f6ca" : "#ff8a80"
                    font.pixelSize: 10
                    font.bold: true
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
            }
        }
    }
}
