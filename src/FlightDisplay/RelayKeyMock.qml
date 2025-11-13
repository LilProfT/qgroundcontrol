import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.15

import QGroundControl
import QGroundControl.ScreenTools
import QGroundControl.Controls
import QGroundControl.Palette
import QGroundControl.Vehicle
import QGroundControl.FlightMap
Column {
    spacing: 0
    width: 110
    property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    property var _vcuInfo: (_activeVehicle.vcu) ? _activeVehicle.vcu : 0

    // Top label
    Item {
        width: parent.width
        height: 30

        property color backgroundColor: "#060c44"

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
            text: "Pin"
            color: "white"
            font.bold: true
            font.italic: true
        }
    }

    // Metric data model
    ListModel {
        id: metricModel
        property bool completed: false
        property bool contactorState: _vcuInfo.contactorState.rawValue
        property bool relayDetonatorState: _vcuInfo.relayDetonatorState.rawValue
        property bool relayExplodeState: _vcuInfo.relayExplodeState.rawValue

        Component.onCompleted: {
            append({name:"Dung\nlượng", bgColor: "black", value: "38", unit: "%"});
            append({name:"FSU", bgColor: "orange", value: "22", unit: "V"});
            append({name:"Điểm hỏa", bgColor: "#42ff2b", value: "Mở", unit: ""});
            append({name:"Mở khóa 1", bgColor: "#42ff2b", value: "Mở", unit: ""});
            append({name:"Khóa từ\nVCU", bgColor: "#42ff2b", value: "Mở", unit: ""});
            completed = true;
        }

        onContactorStateChanged:
            if (completed) {
                setProperty(4,"value",contactorState ? "Ngắt" : "Đóng");
                setProperty(4,"bgColor",contactorState ? "#c50202" : "#42ff2b");
            }
        onRelayDetonatorStateChanged:
            if (completed) {
                setProperty(3,"value",relayDetonatorState ? "Ngắt" : "Đóng");
                setProperty(3,"bgColor",relayDetonatorState ? "#c50202" : "#42ff2b");
            }
        onRelayExplodeStateChanged:
            if (completed) {
                setProperty(2,"value",relayExplodeState ? "Ngắt" : "Đóng");
                setProperty(2,"bgColor",relayExplodeState ? "#c50202" : "#42ff2b");
            }
        // ListElement { name: "Dung\nlượng"; bgColor: "black";  value: "38"; unit: "%" }
        // ListElement { name: "Điện\náp";   bgColor: "orange"; value: "22"; unit: "V" }
        // ListElement { name: "Dòng\ntải";  bgColor: "black";    value: "8"; unit: "A" }
        // ListElement { name: "Nhiệt\nđộ";  bgColor: "black";  value: "40";  unit: "°C" }
        // ListElement { name: "Khóa\ntừ";    bgColor: "black";    value: "100"; unit: "°C" }
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
