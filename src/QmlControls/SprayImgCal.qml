import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.4

import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0

Rectangle {
    width: imgRec.width + inset * 2
    height: imgRec.height + inset * 2
    // Indicates whether calibration is valid for this control
    property bool calValid: false

    // Indicates whether the control is currently being calibrated
    property bool calInProgress: false

    // Text to show while calibration is in progress
    property string calInProgressText: ""

    // Image source
    property var imgSrc: ""

    readonly property int inset: 7

    QGCPalette {
        id: qgcPal
        colorGroupEnabled: enabled
    }

    color:  calInProgress ? "yellow" : (calValid ? qgcPal.colorGreen : qgcPal.colorRed)

    Rectangle {
        id:     imgRec
        x:      inset
        y:      inset
        width:  imgCal.width  + ScreenTools.defaultFontPixelWidth * 1
        height: imgCal.height + ScreenTools.defaultFontPixelHeight * 1.5
        color: qgcPal.window

        Image {
            id: imgCal
            x: ScreenTools.defaultFontPixelWidth * 0.5
            fillMode:   Image.PreserveAspectFit
            smooth:     true
            source:     imgSrc
        }

        QGCLabel {
            width:                  imgCal.width
            height:                 imgCal.height
            anchors.top:            imgCal.bottom
            x: ScreenTools.defaultFontPixelWidth * 0.5
            horizontalAlignment:     Text.AlignHCenter
            font.pointSize:         ScreenTools.mediumFontPointSize
            text:                   calInProgress ? calInProgressText : (calValid ? qsTr("Completed") : qsTr("Incomplete"))
        }
    }
}
