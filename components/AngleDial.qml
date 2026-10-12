import QtQuick
import QtQuick.Controls
import QtQuick.Shapes

Rectangle {
    id: root

    property string title: "AZIMUTH"
    property real targetAngle: 0.0      // 0 to 360 degrees
    property int rawValue: 0            // 0 to 1023
    property bool isFresh: true

    // Internal unwrapped angle for shortest-path rotation
    property real unwrappedAngle: 0.0

    onTargetAngleChanged: {
        let normalizedCurrent = (unwrappedAngle % 360 + 360) % 360;
        let delta = targetAngle - normalizedCurrent;
        if (delta > 180) delta -= 360;
        if (delta < -180) delta += 360;
        unwrappedAngle += delta;
    }

    color: "#131d31"
    border.color: isFresh ? "#233d66" : "#4a3319"
    border.width: 1
    radius: 12

    Column {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 8

        // Header
        Item {
            width: parent.width
            height: 24

            Label {
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                text: root.title
                color: "#90caf9"
                font.pixelSize: 13
                font.bold: true
                font.letterSpacing: 1.2
            }

            Rectangle {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                width: 70
                height: 20
                radius: 4
                color: root.isFresh ? "#00e6761f" : "#ffab001f"
                border.color: root.isFresh ? "#00e676" : "#ffab00"
                border.width: 1

                Label {
                    anchors.centerIn: parent
                    text: root.isFresh ? "LIVE" : "STALE"
                    color: root.isFresh ? "#00e676" : "#ffab00"
                    font.pixelSize: 10
                    font.bold: true
                    font.letterSpacing: 0.5
                }
            }
        }

        // Circular Dial Area
        Item {
            id: dialContainer
            width: parent.width
            height: parent.height - 76

            readonly property real dialSize: Math.min(width, height)
            readonly property real dialRadius: dialSize / 2
            readonly property real centerX: width / 2
            readonly property real centerY: height / 2

            // Background Dial Canvas (Draws ring, tick marks, and degree labels)
            Canvas {
                id: dialCanvas
                anchors.fill: parent

                onPaint: {
                    let ctx = getContext("2d");
                    ctx.reset();

                    let cx = dialContainer.centerX;
                    let cy = dialContainer.centerY;
                    let r = dialContainer.dialRadius - 10;
                    if (r <= 20) return;

                    // Outer circular bezel
                    ctx.beginPath();
                    ctx.arc(cx, cy, r, 0, 2 * Math.PI);
                    ctx.strokeStyle = "#1e3355";
                    ctx.lineWidth = 4;
                    ctx.stroke();

                    // Inner track
                    ctx.beginPath();
                    ctx.arc(cx, cy, r - 12, 0, 2 * Math.PI);
                    ctx.strokeStyle = "#162742";
                    ctx.lineWidth = 1.5;
                    ctx.stroke();

                    // Tick marks & Degree numbers
                    ctx.textAlign = "center";
                    ctx.textBaseline = "middle";

                    for (let deg = 0; deg < 360; deg += 5) {
                        let rad = (deg - 90) * Math.PI / 180.0;
                        let isMajor = (deg % 30 === 0);
                        let isMedium = (deg % 10 === 0 && !isMajor);

                        let tickLen = isMajor ? 12 : (isMedium ? 7 : 4);
                        let startR = r - 2;
                        let endR = startR - tickLen;

                        let x1 = cx + startR * Math.cos(rad);
                        let y1 = cy + startR * Math.sin(rad);
                        let x2 = cx + endR * Math.cos(rad);
                        let y2 = cy + endR * Math.sin(rad);

                        ctx.beginPath();
                        ctx.moveTo(x1, y1);
                        ctx.lineTo(x2, y2);
                        ctx.strokeStyle = (deg === 0) ? "#00e5ff" : (isMajor ? "#64b5f6" : (isMedium ? "#37474f" : "#263238"));
                        ctx.lineWidth = (deg === 0) ? 2.5 : (isMajor ? 2.0 : 1.0);
                        ctx.stroke();

                        // Degree labels at 0, 30, 60, ..., 330
                        if (isMajor) {
                            let textR = r - 24;
                            let tx = cx + textR * Math.cos(rad);
                            let ty = cy + textR * Math.sin(rad);

                            ctx.font = (deg === 0) ? "bold 11px sans-serif" : "9px sans-serif";
                            ctx.fillStyle = (deg === 0) ? "#00e5ff" : "#90a4ae";
                            ctx.fillText(deg.toString() + "°", tx, ty);
                        }
                    }

                    // Cardinal references: N at top, E, S, W
                    ctx.font = "bold 11px sans-serif";
                    ctx.fillStyle = "#00e5ff";
                    ctx.fillText("N", cx, cy - r + 38);
                    ctx.fillStyle = "#607d8b";
                    ctx.fillText("E", cx + r - 38, cy);
                    ctx.fillText("S", cx, cy + r - 38);
                    ctx.fillText("W", cx - r + 38, cy);
                }

                Component.onCompleted: requestPaint()
                onWidthChanged: requestPaint()
                onHeightChanged: requestPaint()
            }

            // Rotating Needle
            Item {
                id: needleItem
                x: dialContainer.centerX
                y: dialContainer.centerY
                width: 0
                height: 0

                rotation: root.unwrappedAngle

                Behavior on rotation {
                    NumberAnimation {
                        duration: 180
                        easing.type: Easing.OutCubic
                    }
                }

                // Needle Graphic
                Shape {
                    anchors.centerIn: parent
                    width: dialContainer.dialRadius * 2
                    height: dialContainer.dialRadius * 2

                    ShapePath {
                        strokeColor: root.isFresh ? "#00e5ff" : "#ffab00"
                        strokeWidth: 1.5
                        fillColor: root.isFresh ? "#00e5ff" : "#ffab00"

                        // Pointer pointing upwards (0 deg reference)
                        startX: dialContainer.dialRadius
                        startY: dialContainer.dialRadius - dialContainer.dialRadius + 22

                        PathLine {
                            x: dialContainer.dialRadius - 6
                            y: dialContainer.dialRadius - 10
                        }
                        PathLine {
                            x: dialContainer.dialRadius + 6
                            y: dialContainer.dialRadius - 10
                        }
                        PathLine {
                            x: dialContainer.dialRadius
                            y: dialContainer.dialRadius - dialContainer.dialRadius + 22
                        }
                    }

                    // Counter-balance tail
                    ShapePath {
                        strokeColor: "#37474f"
                        strokeWidth: 1
                        fillColor: "#263238"

                        startX: dialContainer.dialRadius - 3
                        startY: dialContainer.dialRadius + 8
                        PathLine {
                            x: dialContainer.dialRadius + 3
                            y: dialContainer.dialRadius + 8
                        }
                        PathLine {
                            x: dialContainer.dialRadius
                            y: dialContainer.dialRadius + 28
                        }
                        PathLine {
                            x: dialContainer.dialRadius - 3
                            y: dialContainer.dialRadius + 8
                        }
                    }
                }
            }

            // Center Display Hub (Circular center badge showing angle & raw value)
            Rectangle {
                anchors.centerIn: parent
                width: Math.min(dialContainer.dialSize * 0.42, 110)
                height: width
                radius: width / 2
                color: "#0b1322"
                border.color: root.isFresh ? "#00e5ff" : "#ffab00"
                border.width: 2

                Column {
                    anchors.centerIn: parent
                    spacing: 2

                    Label {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: root.targetAngle.toFixed(1) + "°"
                        color: root.isFresh ? "#ffffff" : "#eceff1"
                        font.pixelSize: Math.max(14, Math.min(22, parent.parent.width * 0.22))
                        font.bold: true
                    }

                    Label {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: "RAW: " + root.rawValue
                        color: "#80d8ff"
                        font.pixelSize: Math.max(9, Math.min(11, parent.parent.width * 0.11))
                        font.family: "Consolas, monospace"
                    }

                    Label {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: "[0x" + root.rawValue.toString(16).toUpperCase().padStart(4, '0') + "]"
                        color: "#546e7a"
                        font.pixelSize: Math.max(8, Math.min(10, parent.parent.width * 0.1))
                        font.family: "Consolas, monospace"
                    }
                }
            }
        }

        // Bottom summary line
        Item {
            width: parent.width
            height: 20

            Label {
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                text: "Range: 0.0° - 360.0°"
                color: "#607d8b"
                font.pixelSize: 11
            }

            Label {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                text: "10-bit: 0 - 1023"
                color: "#607d8b"
                font.pixelSize: 11
                font.family: "Consolas, monospace"
            }
        }
    }
}
