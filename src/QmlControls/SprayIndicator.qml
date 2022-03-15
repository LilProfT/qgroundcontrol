//Mismart: Custom file

import QtQuick          2.3
import QtQuick.Controls 1.2

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0

Rectangle {
    height:     _value.height + _margins * 2 + _icon.height
    color:      Qt.hsla(_baseBGColor.hslHue, _baseBGColor.hslSaturation, _baseBGColor.hslLightness, 0.5) //Copy from the telemetryBar
    radius:     _margins * 2
    property real   _margins:       ScreenTools.defaultFontPixelWidth / 2
    property var    _vehicle:       QGroundControl.multiVehicleManager.activeVehicle ? QGroundControl.multiVehicleManager.activeVehicle : QGroundControl.multiVehicleManager.offlineEditingVehicle
    property color  _baseBGColor: qgcPal.window

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    QGCColoredImage {
        id:                         _icon
        anchors.horizontalCenter:   parent.horizontalCenter
        anchors.top:                parent.top
        anchors.topMargin:          _margins
        source:                     "/InstrumentValueIcons/dashboard.svg"
        mipmap:                     true
        width:                      parent.width * 0.2
        height:                     width
        sourceSize.width:           width
        color:                      qgcPal.text
        fillMode:                   Image.PreserveAspectFit
    }
    Row {
        spacing:                    0
        anchors.bottom:             parent.bottom
        anchors.horizontalCenter:   _icon.horizontalCenter

        QGCLabel {
            id:                         _value
            text:                       _vehicle.areaSprayed.value.toFixed(2) + " "
            font.pointSize:             ScreenTools.largeFontPointSize * 1.5 //King sized
            font.bold:                  true
        }
//        QGCLabel {
//            id:                         _totalArea
//            text:                       _vehicle.totalArea.value.toFixed(2) + " "
//            font.pointSize:             ScreenTools.mediumFontPointSize //King sized
//        }
        QGCLabel {
            id:                         _units
            anchors.verticalCenter:     _value.verticalCenter
            text:                       _vehicle.areaSprayed.units
            font.pointSize:             ScreenTools.mediumFontPointSize
        }
    }
}
