import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.15

Column {
    spacing: 0
    width: 110

    // Top label
    Item {
        width: parent.width
        height: 30

        property color backgroundColor: "#684706"

        Canvas {
            id: roundedTag
            anchors.fill: parent
            opacity: 0.7

            onPaint: {
                var ctx = getContext("2d");
                ctx.clearRect(0, 0, width, height);
                ctx.fillStyle = parent.backgroundColor;

                ctx.beginPath();
                ctx.moveTo(15, 0);
                ctx.arcTo(0, 0, 0, 15, 15);
                ctx.lineTo(0, height);
                ctx.lineTo(width, height);
                ctx.lineTo(width, 15);
                ctx.arcTo(width, 0, width - 15, 0, 15);
                ctx.closePath();
                ctx.fill();
            }
        }

        Text {
            anchors.centerIn: parent
            text: "Nhiên liệu"
            color: "white"
            font.bold: true
            font.italic: true
        }
    }

    // Metric data model
    ListModel {
        id: metricModel
        ListElement { name: "Két 1"; bgColor: "black"; value: "38"; unit: "L" }
        ListElement { name: "Két 2"; bgColor: "black"; value: "38"; unit: "L" }
        ListElement { name: "Két 3"; bgColor: "black"; value: "38"; unit: "L" }
        ListElement { name: "Két 4"; bgColor: "black"; value: "38"; unit: "L" }
        ListElement { name: "Két 5"; bgColor: "black"; value: "38"; unit: "L" }
    }

    // Metric display template
    Repeater {
        model: metricModel

        Rectangle {
            width: parent.width
            height: 47
            color: "transparent"
            border.color: "black"
            border.width: 1

            Rectangle {
                anchors.fill: parent
                color: model.bgColor
                opacity: 0.5
            }

            RowLayout {
                anchors.fill: parent
                anchors.margins: 3
                anchors.rightMargin: 6
                spacing: 0

                Text {
                    text: model.name
                    color: "white"
                    wrapMode: Text.Wrap
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                    Layout.fillHeight: true
                    Layout.leftMargin: 6
                    Layout.alignment: Qt.AlignLeft
                    Layout.maximumWidth: 50
                }

                Column {
                    Layout.alignment: Qt.AlignRight
                    Layout.rightMargin: 6

                    Text {
                        text: model.value
                        color: "white"
                        font.bold: true
                        font.pixelSize: 22
                        height: 27
                        horizontalAlignment: Text.AlignRight
                        verticalAlignment: Text.AlignTop
                    }
                    Text {
                        text: model.unit
                        color: "white"
                        font.pixelSize: 12
                        height: 15
                        horizontalAlignment: Text.AlignRight
                        verticalAlignment: Text.AlignVCenter
                        anchors.right: parent.right
                    }
                }
            }
        }
    }
}
