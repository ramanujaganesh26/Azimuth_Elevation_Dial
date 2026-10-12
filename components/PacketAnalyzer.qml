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
        spacing: 8

        // Header
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Label {
                text: "RECEIVED PACKET ANALYZER"
                color: "#90caf9"
                font.pixelSize: 12
                font.bold: true
                font.letterSpacing: 1.0
            }

            Item { Layout.fillWidth: true }

            Rectangle {
                width: 75
                height: 20
                radius: 4
                color: receiver.lastPacketCrcValid ? "#00e6761f" : "#ff17441f"
                border.color: receiver.lastPacketCrcValid ? "#00e676" : "#ff1744"
                border.width: 1

                Label {
                    anchors.centerIn: parent
                    text: receiver.lastPacketCrcValid ? "CRC VALID" : "CRC ERR"
                    color: receiver.lastPacketCrcValid ? "#00e676" : "#ff1744"
                    font.pixelSize: 9
                    font.bold: true
                }
            }
        }

        // Complete Packet HEX Box (Selectable / Copyable)
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4

            Label {
                text: "Latest Complete Packet (HEX):"
                color: "#78909c"
                font.pixelSize: 10
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 36
                color: "#0a101d"
                border.color: receiver.lastPacketCrcValid ? "#00e5ff44" : "#ff174444"
                border.width: 1
                radius: 4

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 6
                    spacing: 6

                    TextEdit {
                        id: packetHexText
                        text: receiver.lastPacketHex
                        color: receiver.lastPacketCrcValid ? "#80d8ff" : "#ff8a80"
                        font.family: "Consolas, monospace"
                        font.pixelSize: 12
                        font.bold: true
                        readOnly: true
                        selectByMouse: true
                        Layout.fillWidth: true
                        verticalAlignment: Text.AlignVCenter
                    }

                    Button {
                        text: "Copy"
                        Layout.preferredHeight: 24
                        Layout.preferredWidth: 50
                        onClicked: {
                            packetHexText.selectAll();
                            packetHexText.copy();
                        }
                    }
                }
            }
        }

        // Protocol Breakdown Grid
        GridLayout {
            Layout.fillWidth: true
            columns: 4
            rowSpacing: 6
            columnSpacing: 10

            // Row 1: Message Type & Sequence
            Label { text: "Msg Type:"; color: "#90a4ae"; font.pixelSize: 11 }
            Label {
                text: receiver.lastPacketMessageType
                color: "#ffffff"; font.pixelSize: 11; font.bold: true
                Layout.fillWidth: true
            }

            Label { text: "Seq #:"; color: "#90a4ae"; font.pixelSize: 11 }
            Label {
                text: receiver.lastPacketSequence.toString()
                color: "#ffffff"; font.pixelSize: 11; font.family: "Consolas, monospace"
                Layout.fillWidth: true
            }

            // Row 2: Payload Length & Source
            Label { text: "Length:"; color: "#90a4ae"; font.pixelSize: 11 }
            Label {
                text: receiver.lastPacketPayloadLen.toString() + " bytes"
                color: "#ffffff"; font.pixelSize: 11; font.family: "Consolas, monospace"
                Layout.fillWidth: true
            }

            Label { text: "Source:"; color: "#90a4ae"; font.pixelSize: 11 }
            Label {
                text: receiver.lastPacketSource
                color: "#b0bec5"; font.pixelSize: 11
                elide: Text.ElideRight
                Layout.fillWidth: true
            }

            // Row 3: CRC Received & Calculated
            Label { text: "Rx CRC:"; color: "#90a4ae"; font.pixelSize: 11 }
            Label {
                text: receiver.lastPacketCrcReceived
                color: receiver.lastPacketCrcValid ? "#00e676" : "#ff1744"
                font.pixelSize: 11; font.family: "Consolas, monospace"; font.bold: true
                Layout.fillWidth: true
            }

            Label { text: "Calc CRC:"; color: "#90a4ae"; font.pixelSize: 11 }
            Label {
                text: receiver.lastPacketCrcCalculated
                color: "#80d8ff"
                font.pixelSize: 11; font.family: "Consolas, monospace"
                Layout.fillWidth: true
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#1c2c47"
        }

        // Decoded Telemetry Values Section
        GridLayout {
            Layout.fillWidth: true
            columns: 4
            rowSpacing: 6
            columnSpacing: 10

            Label { text: "Raw Azimuth:"; color: "#90a4ae"; font.pixelSize: 11 }
            Label {
                text: receiver.azimuthRaw.toString() + " (0x" + receiver.azimuthRaw.toString(16).toUpperCase().padStart(4, '0') + ")"
                color: "#00e5ff"; font.pixelSize: 11; font.family: "Consolas, monospace"; font.bold: true
                Layout.fillWidth: true
            }

            Label { text: "Decoded Az:"; color: "#90a4ae"; font.pixelSize: 11 }
            Label {
                text: receiver.azimuthDegrees.toFixed(2) + "°"
                color: "#ffffff"; font.pixelSize: 11; font.bold: true
                Layout.fillWidth: true
            }

            Label { text: "Raw Elevation:"; color: "#90a4ae"; font.pixelSize: 11 }
            Label {
                text: receiver.elevationRaw.toString() + " (0x" + receiver.elevationRaw.toString(16).toUpperCase().padStart(4, '0') + ")"
                color: receiver.elevationDegrees >= 0 ? "#00e5ff" : "#ff9100"
                font.pixelSize: 11; font.family: "Consolas, monospace"; font.bold: true
                Layout.fillWidth: true
            }

            Label { text: "Decoded El:"; color: "#90a4ae"; font.pixelSize: 11 }
            Label {
                text: (receiver.elevationDegrees >= 0 ? "+" : "") + receiver.elevationDegrees.toFixed(2) + "°"
                color: "#ffffff"; font.pixelSize: 11; font.bold: true
                Layout.fillWidth: true
            }
        }

        // Latest Incoming Byte Stream Batch (for detecting fragmentation/concatenation)
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2

            Label {
                text: "Recent Raw Inbound Bytes (Stream Chunk):"
                color: "#78909c"
                font.pixelSize: 10
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 26
                color: "#0a101d"
                border.color: "#1e3252"
                border.width: 1
                radius: 4

                Label {
                    anchors.fill: parent
                    anchors.margins: 4
                    text: receiver.lastReceivedBytesHex
                    color: "#90a4ae"
                    font.family: "Consolas, monospace"
                    font.pixelSize: 10
                    elide: Text.ElideRight
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }
    }
}
