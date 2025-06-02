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

    QtObject {
        id: errorBitMaskPos
        property int thermoHealthy: 0
        property int thermoConnected: 1
        property int overtempCWI: 2
        property int overtempCWO: 3
        property int overtempEC: 4
        property int overtempFC: 5
        property int overtempEP: 6
    }

    //Extract a bit from a position
    function bit32_getPos(value, pos) {
        return ((value >> pos) & 0x01)
    }

    function getBackGroundColor(overtemp_flags,value, valueWarningLimit) {
        return (overtemp_flags || value === 0) ? "#c50202" :
                (value > valueWarningLimit) ? "#FC7803" : "#42ff2b"
    }

    // Top label
    Item {
        width: parent.width
        height: 30

        property color backgroundColor: "#101f3a"
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
            text: "Nhiệt độ"
            color: "white"
            font.bold: true
            font.italic: true
        }
    }

    // Metric data model
    ListModel {
        id: metricModel
        property int errorMask: _vcuInfo ? _vcuInfo.thermoErrorMask.rawValue : 0
        property var coolantWaterInletTemp: _vcuInfo ? _vcuInfo.coolantWaterInletTempChannel.rawValue : 0
        property var coolantWaterOutletTemp: _vcuInfo ? _vcuInfo.coolantWaterOutletTempChannel.rawValue:0
        property var engineCabinTemp: _vcuInfo ? _vcuInfo.engineCabinTempChannel.rawValue:0
        property var fuelCabinTemp:  _vcuInfo ? _vcuInfo.fuelCabinTempChannel.rawValue:0
        property var exhaustPipeTemp: _vcuInfo ? _vcuInfo.exhaustPipeTempChannel.rawValue:0
        property bool thermo_healthy:  bit32_getPos(errorMask,0)
        property bool completed: false

        //Initialise the List model
        Component.onCompleted: {
            append({name:"Khoang\nTrong", bgColor: "black", value: engineCabinTemp.toString(), unit: "°C"});
            append({name:"Khoang\nNgoài", bgColor: "black", value: fuelCabinTemp.toString(), unit: "°C"});
            append({name:"Mát\nTrong", bgColor: "black", value: coolantWaterInletTemp.toString(), unit: "°C"});
            append({name:"Mát\nNgoài", bgColor: "black", value: coolantWaterOutletTemp.toString(), unit: "°C"});
            append({name:"Ống\nXả", bgColor: "black", value: exhaustPipeTemp.toString(), unit: "°C"});
            completed = true;
        }

        onEngineCabinTempChanged: {
            if(completed) {
                setProperty(0,"value",engineCabinTemp.toString());
                setProperty(0,"bgColor",getBackGroundColor(bit32_getPos(errorMask,
                                        errorBitMaskPos.overtempEC),engineCabinTemp,40));
            }
        }

        onFuelCabinTempChanged: {
            if(completed) {
                setProperty(1,"value",fuelCabinTemp.toString());
                setProperty(1,"bgColor",getBackGroundColor(bit32_getPos(errorMask,
                                        errorBitMaskPos.overtempFC),fuelCabinTemp,40));
            }
        }

        onCoolantWaterInletTempChanged: {
            if(completed) {
                setProperty(2,"value",coolantWaterInletTemp.toString());
                setProperty(2,"bgColor",getBackGroundColor(bit32_getPos(errorMask,
                                        errorBitMaskPos.overtempCWI),coolantWaterInletTemp,85));
            }
        }

        onCoolantWaterOutletTempChanged: {
            if(completed) {
                setProperty(3,"value",coolantWaterOutletTemp.toString());
                setProperty(3,"bgColor",getBackGroundColor(bit32_getPos(errorMask,
                                        errorBitMaskPos.overtempCWO), coolantWaterOutletTemp,85));
            }
        }

        onExhaustPipeTempChanged: {
            if(completed) {
                setProperty(4,"value",exhaustPipeTemp.toString());
                setProperty(4,"bgColor",getBackGroundColor(bit32_getPos(errorMask,
                                        errorBitMaskPos.overtempEP),exhaustPipeTemp,90));
            }
        }
        onErrorMaskChanged:
            if(completed) {
                setProperty(0,"bgColor",getBackGroundColor(bit32_getPos(errorMask,
                                        errorBitMaskPos.overtempEC),engineCabinTemp,40));
                setProperty(1,"bgColor",getBackGroundColor(bit32_getPos(errorMask,
                                        errorBitMaskPos.overtempFC),fuelCabinTemp,40));
                setProperty(2,"bgColor",getBackGroundColor(bit32_getPos(errorMask,
                                        errorBitMaskPos.overtempCWI),coolantWaterInletTemp,85));
                setProperty(3,"bgColor",getBackGroundColor(bit32_getPos(errorMask,
                                        errorBitMaskPos.overtempCWO), coolantWaterOutletTemp,85));
                setProperty(4,"bgColor",getBackGroundColor(bit32_getPos(errorMask,
                                        errorBitMaskPos.overtempEP),exhaustPipeTemp,90));
            }
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
