import QtQuick
import QtQuick.Controls
import QtQuick.Shapes

Rectangle {
    id: root

    property string title: "ELEVATION"
    property real targetAngle: 0.0      // -90.0 to +90.0 degrees
    property int rawValue: 511          // 0 to 1023 (midpoint ~511)
    property bool isFresh: true

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

        // Elevation Arc Area
        Item {
            id: dialContainer
            width: parent.width
            height: parent.height - 76

            readonly property real dialSize: Math.min(width, height)
            readonly property real dialRadius: dialSize / 2
            readonly property real centerX: width / 2
            readonly property real centerY: height / 2

            // Elevation Canvas: draws 180° arc with negative (amber) and positive (cyan) zones
            Canvas {
                id: elevCanvas
                anchors.fill: parent

                onPaint: {
                    let ctx = getContext("2d");
                    ctx.reset();

                    let cx = dialContainer.centerX;
                    let cy = dialContainer.centerY;
                    let r = dialContainer.dialRadius - 10;
                    if (r <= 20) return;

                    // Active Elevation Track: -10° to +85°
                    let radMin = (-10 - 90) * Math.PI / 180.0;
                    let radZero = (0 - 90) * Math.PI / 180.0;
                    let radMax = (85 - 90) * Math.PI / 180.0;

                    // Background track bezel
                    ctx.beginPath();
                    ctx.arc(cx, cy, r, (-15 - 90) * Math.PI / 180.0, (90 - 90) * Math.PI / 180.0);
                    ctx.strokeStyle = "#162742";
                    ctx.lineWidth = 4;
                    ctx.stroke();

                    // Negative Track Arc (-10° to 0°)
                    ctx.beginPath();
                    ctx.arc(cx, cy, r, radMin, radZero);
                    ctx.strokeStyle = "#ff9100";
                    ctx.lineWidth = 3.5;
                    ctx.stroke();

                    // Positive Track Arc (0° to +85°)
                    ctx.beginPath();
                    ctx.arc(cx, cy, r, radZero, radMax);
                    ctx.strokeStyle = "#00e5ff";
                    ctx.lineWidth = 3.5;
                    ctx.stroke();

                    // Inner border arc
                    ctx.beginPath();
                    ctx.arc(cx, cy, r - 12, radMin, radMax);
                    ctx.strokeStyle = "#1e3355";
                    ctx.lineWidth = 1.5;
                    ctx.stroke();

                    // Specified elevation markings: 85, 65, 45, 10, 0, -10
                    let markedAngles = [-10, 0, 10, 45, 65, 85];
                    ctx.textAlign = "center";
                    ctx.textBaseline = "middle";

                    // Minor ticks every 5 degrees from -10 to 85
                    for (let a = -10; a <= 85; a += 5) {
                        let rad = (a - 90) * Math.PI / 180.0;
                        let isMajor = (markedAngles.indexOf(a) !== -1);
                        let isMedium = (a % 10 === 0 && !isMajor);
                        let tickLen = isMajor ? 13 : (isMedium ? 7 : 4);
                        let startR = r - 2;
                        let endR = startR - tickLen;

                        let x1 = cx + startR * Math.cos(rad);
                        let y1 = cy + startR * Math.sin(rad);
                        let x2 = cx + endR * Math.cos(rad);
                        let y2 = cy + endR * Math.sin(rad);

                        ctx.beginPath();
                        ctx.moveTo(x1, y1);
                        ctx.lineTo(x2, y2);
                        if (a === 0) {
                            ctx.strokeStyle = "#ffffff";
                            ctx.lineWidth = 2.5;
                        } else if (a < 0) {
                            ctx.strokeStyle = isMajor ? "#ff9100" : "#5d4037";
                            ctx.lineWidth = isMajor ? 2.0 : 1.0;
                        } else {
                            ctx.strokeStyle = isMajor ? "#00e5ff" : "#006064";
                            ctx.lineWidth = isMajor ? 2.0 : 1.0;
                        }
                        ctx.stroke();

                        if (isMajor) {
                            // Slight radial stagger for 0° and +10° to prevent label crowding
                            let textOffset = (a === 0) ? 20 : ((a === 10) ? 27 : 23);
                            let textR = r - textOffset;
                            let tx = cx + textR * Math.cos(rad);
                            let ty = cy + textR * Math.sin(rad);

                            let textLabel = (a > 0 ? "+" : "") + a.toString() + "°";
                            ctx.font = (a === 0) ? "bold 11px sans-serif" : "bold 9px sans-serif";
                            ctx.fillStyle = (a === 0) ? "#ffffff" : (a < 0 ? "#ffb74d" : "#80deea");
                            ctx.fillText(textLabel, tx, ty);
                        }
                    }

                    // Zone labels
                    ctx.font = "bold 9px sans-serif";
                    ctx.fillStyle = "#ff9100";
                    ctx.fillText("DOWN (-10°)", cx - r * 0.35, cy - r * 0.45);
                    ctx.fillStyle = "#00e5ff";
                    ctx.fillText("UP (+85°)", cx + r * 0.55, cy - r * 0.35);

                    // Horizon reference line at bottom
                    ctx.beginPath();
                    ctx.moveTo(cx - r * 0.5, cy);
                    ctx.lineTo(cx + r, cy);
                    ctx.strokeStyle = "#1a2c47";
                    ctx.lineWidth = 1;
                    ctx.stroke();
                }

                Component.onCompleted: requestPaint()
                onWidthChanged: requestPaint()
                onHeightChanged: requestPaint()
            }

            // Pointer (Rotates from -90 to +90 degrees)
            Item {
                id: elevNeedle
                x: dialContainer.centerX
                y: dialContainer.centerY
                width: 0
                height: 0

                rotation: root.targetAngle

                Behavior on rotation {
                    NumberAnimation {
                        duration: 180
                        easing.type: Easing.OutCubic
                    }
                }

                Shape {
                    anchors.centerIn: parent
                    width: dialContainer.dialRadius * 2
                    height: dialContainer.dialRadius * 2

                    ShapePath {
                        strokeColor: root.targetAngle >= 0 ? (root.isFresh ? "#00e5ff" : "#ffab00")
                                                           : (root.isFresh ? "#ff9100" : "#d84315")
                        strokeWidth: 1.5
                        fillColor: root.targetAngle >= 0 ? (root.isFresh ? "#00e5ff" : "#ffab00")
                                                         : (root.isFresh ? "#ff9100" : "#d84315")

                        startX: dialContainer.dialRadius
                        startY: dialContainer.dialRadius - dialContainer.dialRadius + 22

                        PathLine {
                            x: dialContainer.dialRadius - 6
                            y: dialContainer.dialRadius - 8
                        }
                        PathLine {
                            x: dialContainer.dialRadius + 6
                            y: dialContainer.dialRadius - 8
                        }
                        PathLine {
                            x: dialContainer.dialRadius
                            y: dialContainer.dialRadius - dialContainer.dialRadius + 22
                        }
                    }

                    // Pivot base
                    ShapePath {
                        strokeColor: "#37474f"
                        strokeWidth: 1
                        fillColor: "#263238"

                        startX: dialContainer.dialRadius - 4
                        startY: dialContainer.dialRadius + 4
                        PathLine {
                            x: dialContainer.dialRadius + 4
                            y: dialContainer.dialRadius + 4
                        }
                        PathLine {
                            x: dialContainer.dialRadius
                            y: dialContainer.dialRadius + 16
                        }
                        PathLine {
                            x: dialContainer.dialRadius - 4
                            y: dialContainer.dialRadius + 4
                        }
                    }
                }
            }

            // Center Readout Hub
            Rectangle {
                anchors.centerIn: parent
                width: Math.min(dialContainer.dialSize * 0.42, 110)
                height: width
                radius: width / 2
                color: "#0b1322"
                border.color: root.targetAngle >= 0 ? (root.isFresh ? "#00e5ff" : "#ffab00")
                                                    : (root.isFresh ? "#ff9100" : "#d84315")
                border.width: 2

                Column {
                    anchors.centerIn: parent
                    spacing: 2

                    Label {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: (root.targetAngle >= 0 ? "+" : "") + root.targetAngle.toFixed(1) + "°"
                        color: root.targetAngle >= 0 ? (root.isFresh ? "#00e5ff" : "#ffffff") : "#ffb74d"
                        font.pixelSize: Math.max(14, Math.min(22, parent.parent.width * 0.22))
                        font.bold: true
                    }

                    Label {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: "RAW: " + root.rawValue
                        color: root.targetAngle >= 0 ? "#80d8ff" : "#ffe082"
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
                text: "Range: -10.0° to +85.0°"
                color: "#607d8b"
                font.pixelSize: 11
            }

            Label {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                text: "10-bit: 0 - 1023 (Mid 511)"
                color: "#607d8b"
                font.pixelSize: 11
                font.family: "Consolas, monospace"
            }
        }
    }
}
