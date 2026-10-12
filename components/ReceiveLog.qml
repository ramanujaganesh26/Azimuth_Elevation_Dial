import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    property bool autoScroll: true

    color: "#131d31"
    border.color: "#233d66"
    border.width: 1
    radius: 12

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 8

        // Header and Toolbar
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Label {
                text: "COMMUNICATION RECEIVE LOG"
                color: "#90caf9"
                font.pixelSize: 12
                font.bold: true
                font.letterSpacing: 1.0
            }

            Rectangle {
                width: 90
                height: 20
                radius: 4
                color: "#1a2c47"
                border.color: "#2b4670"
                border.width: 1

                Label {
                    anchors.centerIn: parent
                    text: logModel.count + " / 1000 MAX"
                    color: "#90a4ae"
                    font.pixelSize: 9
                    font.family: "Consolas, monospace"
                }
            }

            Item { Layout.fillWidth: true }

            // Pause / Resume Button
            Button {
                id: pauseBtn
                text: logModel.paused ? "Resume Updates" : "Pause Updates"
                Layout.preferredHeight: 26
                checkable: true
                checked: logModel.paused
                onClicked: logModel.paused = !logModel.paused

                contentItem: Text {
                    text: pauseBtn.text
                    color: logModel.paused ? "#ffab00" : "#90caf9"
                    font.pixelSize: 10
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    radius: 4
                    color: logModel.paused ? "#ffab0022" : "#1a2a44"
                    border.color: logModel.paused ? "#ffab00" : "#2a4268"
                    border.width: 1
                }
            }

            // Auto Scroll Toggle
            CheckBox {
                id: autoScrollBox
                text: "Auto-Scroll"
                checked: root.autoScroll
                onCheckedChanged: root.autoScroll = checked

                contentItem: Text {
                    text: autoScrollBox.text
                    color: "#b0bec5"
                    font.pixelSize: 11
                    verticalAlignment: Text.AlignVCenter
                    leftPadding: autoScrollBox.indicator.width + 4
                }
            }

            // Clear Log Button
            Button {
                text: "Clear Log"
                Layout.preferredHeight: 26
                onClicked: logModel.clear()
            }
        }

        // Table Header
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 26
            color: "#0a101d"
            radius: 4

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                spacing: 8

                Label { text: "Time"; color: "#78909c"; font.pixelSize: 10; font.bold: true; Layout.preferredWidth: 80 }
                Label { text: "Transport"; color: "#78909c"; font.pixelSize: 10; font.bold: true; Layout.preferredWidth: 70 }
                Label { text: "Source"; color: "#78909c"; font.pixelSize: 10; font.bold: true; Layout.preferredWidth: 100 }
                Label { text: "Event"; color: "#78909c"; font.pixelSize: 10; font.bold: true; Layout.preferredWidth: 95 }
                Label { text: "Message Type"; color: "#78909c"; font.pixelSize: 10; font.bold: true; Layout.preferredWidth: 110 }
                Label { text: "Details / Decoded Info"; color: "#78909c"; font.pixelSize: 10; font.bold: true; Layout.fillWidth: true }
                Label { text: "Status"; color: "#78909c"; font.pixelSize: 10; font.bold: true; Layout.preferredWidth: 65; horizontalAlignment: Text.AlignRight }
            }
        }

        // Log ListView
        ListView {
            id: logListView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: logModel

            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
            }

            Connections {
                target: logModel
                function onEntryAdded() {
                    if (root.autoScroll && !logModel.paused) {
                        logListView.positionViewAtEnd();
                    }
                }
            }

            delegate: Rectangle {
                id: delegateRect
                width: logListView.width
                height: 28
                color: index % 2 === 0 ? "#111a2c" : "#0e1524"

                // Highlight on hover
                MouseArea {
                    id: rowMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    acceptedButtons: Qt.RightButton | Qt.LeftButton
                    onClicked: (mouse) => {
                        logListView.currentIndex = index;
                    }
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    spacing: 8

                    // Timestamp
                    Label {
                        text: model.timestamp
                        color: "#90a4ae"
                        font.family: "Consolas, monospace"
                        font.pixelSize: 10
                        Layout.preferredWidth: 80
                    }

                    // Transport
                    Label {
                        text: model.transport
                        color: "#80d8ff"
                        font.pixelSize: 10
                        Layout.preferredWidth: 70
                    }

                    // Source
                    Label {
                        text: model.source
                        color: "#b0bec5"
                        font.pixelSize: 10
                        elide: Text.ElideRight
                        Layout.preferredWidth: 100
                    }

                    // Event
                    Label {
                        text: model.event
                        color: model.isError ? "#ff5252" : "#e0e0e0"
                        font.pixelSize: 10
                        font.bold: true
                        Layout.preferredWidth: 95
                    }

                    // Message Type
                    Label {
                        text: model.messageType
                        color: "#81d4fa"
                        font.pixelSize: 10
                        elide: Text.ElideRight
                        Layout.preferredWidth: 110
                    }

                    // Details
                    Label {
                        text: model.details
                        color: model.isError ? "#ff8a80" : "#eceff1"
                        font.pixelSize: 10
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    // Status Badge
                    Rectangle {
                        Layout.preferredWidth: 65
                        Layout.preferredHeight: 18
                        radius: 3
                        color: model.isError ? "#ff174422" : (model.status === "VALID" ? "#00e67622" : "#263238")
                        border.color: model.isError ? "#ff1744" : (model.status === "VALID" ? "#00e676" : "#455a64")
                        border.width: 1

                        Label {
                            anchors.centerIn: parent
                            text: model.status
                            color: model.isError ? "#ff5252" : (model.status === "VALID" ? "#00e676" : "#90a4ae")
                            font.pixelSize: 9
                            font.bold: true
                        }
                    }
                }
            }
        }
    }
}
