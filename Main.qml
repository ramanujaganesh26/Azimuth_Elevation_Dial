import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "components"

ApplicationWindow {
    id: appWindow
    width: 1280
    height: 860
    minimumWidth: 1080
    minimumHeight: 740
    visible: true
    title: qsTr("Azimuth & Elevation Command Receiver")
    color: "#0a101d"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 12

        // ================= HEADER =================
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 64
            color: "#111a2d"
            border.color: "#1f3252"
            border.width: 1
            radius: 10

            RowLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 14

                // App Title & Subtitle
                ColumnLayout {
                    spacing: 2
                    Label {
                        text: "AZIMUTH & ELEVATION COMMAND RECEIVER"
                        color: "#ffffff"
                        font.pixelSize: 15
                        font.bold: true
                        font.letterSpacing: 1.2
                    }
                    Label {
                        text: "Gimbal & Antenna Pointing Ground Monitoring Station"
                        color: "#78909c"
                        font.pixelSize: 10
                    }
                }

                // Strict RX-Only Badge
                Rectangle {
                    Layout.preferredHeight: 28
                    Layout.preferredWidth: 110
                    radius: 4
                    color: "#d5000022"
                    border.color: "#ff1744"
                    border.width: 1.5

                    RowLayout {
                        anchors.centerIn: parent
                        spacing: 4
                        Rectangle {
                            width: 6; height: 6; radius: 3; color: "#ff1744"
                        }
                        Label {
                            text: "RX ONLY"
                            color: "#ff5252"
                            font.pixelSize: 11
                            font.bold: true
                            font.letterSpacing: 1.0
                        }
                    }
                }

                Item { Layout.fillWidth: true }

                // Statistics Badges
                RowLayout {
                    spacing: 8

                    // Valid Packets
                    Rectangle {
                        Layout.preferredHeight: 38
                        Layout.preferredWidth: 80
                        radius: 6
                        color: "#0a1322"
                        border.color: "#00e67644"
                        border.width: 1

                        Column {
                            anchors.centerIn: parent
                            spacing: 1
                            Label {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: "VALID PKTS"
                                color: "#81c784"
                                font.pixelSize: 8
                                font.bold: true
                            }
                            Label {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: receiver.validPacketCount.toString()
                                color: "#00e676"
                                font.pixelSize: 13
                                font.bold: true
                                font.family: "Consolas, monospace"
                            }
                        }
                    }

                    // CRC Errors
                    Rectangle {
                        Layout.preferredHeight: 38
                        Layout.preferredWidth: 80
                        radius: 6
                        color: "#0a1322"
                        border.color: receiver.crcErrorCount > 0 ? "#ff1744" : "#ff174444"
                        border.width: 1

                        Column {
                            anchors.centerIn: parent
                            spacing: 1
                            Label {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: "CRC ERRORS"
                                color: "#ef9a9a"
                                font.pixelSize: 8
                                font.bold: true
                            }
                            Label {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: receiver.crcErrorCount.toString()
                                color: receiver.crcErrorCount > 0 ? "#ff1744" : "#b0bec5"
                                font.pixelSize: 13
                                font.bold: true
                                font.family: "Consolas, monospace"
                            }
                        }
                    }

                    // Malformed / Rejected
                    Rectangle {
                        Layout.preferredHeight: 38
                        Layout.preferredWidth: 80
                        radius: 6
                        color: "#0a1322"
                        border.color: receiver.malformedPacketCount > 0 ? "#ffab00" : "#ffab0044"
                        border.width: 1

                        Column {
                            anchors.centerIn: parent
                            spacing: 1
                            Label {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: "MALFORMED"
                                color: "#ffe082"
                                font.pixelSize: 8
                                font.bold: true
                            }
                            Label {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: receiver.malformedPacketCount.toString()
                                color: receiver.malformedPacketCount > 0 ? "#ffab00" : "#b0bec5"
                                font.pixelSize: 13
                                font.bold: true
                                font.family: "Consolas, monospace"
                            }
                        }
                    }

                    // Reset Counters Button
                    Button {
                        text: "Reset"
                        Layout.preferredHeight: 28
                        Layout.preferredWidth: 50
                        onClicked: receiver.resetCounters()
                    }
                }

                // Connection & Telemetry Status Summary
                ColumnLayout {
                    spacing: 2
                    Layout.alignment: Qt.AlignRight

                    RowLayout {
                        spacing: 6
                        Rectangle {
                            width: 10; height: 10; radius: 5
                            color: receiver.connected ? (receiver.telemetryFresh ? "#00e676" : "#ffab00") : "#ff1744"
                        }
                        Label {
                            text: receiver.connected
                                  ? (receiver.telemetryFresh ? "TELEMETRY LIVE" : "TELEMETRY STALE")
                                  : "DISCONNECTED"
                            color: receiver.connected
                                   ? (receiver.telemetryFresh ? "#00e676" : "#ffab00")
                                   : "#ff5252"
                            font.pixelSize: 11
                            font.bold: true
                        }
                    }

                    Label {
                        text: "Transport: " + receiver.currentTransportName
                        color: "#90caf9"
                        font.pixelSize: 10
                    }

                    Label {
                        text: "Last Pkt: " + receiver.lastPacketTimestamp
                        color: "#78909c"
                        font.pixelSize: 9
                        font.family: "Consolas, monospace"
                    }
                }
            }
        }

        // ================= MAIN GRID =================
        // Upper Deck: Two Gauges (Azimuth 360° and Elevation -90°..+90°)
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 330
            spacing: 12

            AngleDial {
                id: azimuthGauge
                Layout.fillWidth: true
                Layout.fillHeight: true
                title: "AZIMUTH GAUGE (0° - 360°)"
                targetAngle: receiver.azimuthDegrees
                rawValue: receiver.azimuthRaw
                isFresh: receiver.telemetryFresh
            }

            ElevationDial {
                id: elevationGauge
                Layout.fillWidth: true
                Layout.fillHeight: true
                title: "ELEVATION GAUGE (-10° - +85°)"
                targetAngle: receiver.elevationDegrees
                rawValue: receiver.elevationRaw
                isFresh: receiver.telemetryFresh
            }
        }

        // Middle Deck: Communication Setup & Packet Analyzer
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 210
            spacing: 12

            CommunicationPanel {
                id: commPanel
                Layout.preferredWidth: 440
                Layout.fillHeight: true
            }

            PacketAnalyzer {
                id: packetAnalyzer
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
        }

        // Lower Deck: Communication Receive Log
        ReceiveLog {
            id: receiveLog
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }
}
