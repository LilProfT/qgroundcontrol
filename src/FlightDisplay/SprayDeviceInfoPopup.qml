import QtQuick          2.11
import QtQuick.Controls 2.4
import QtQuick.Dialogs  1.3
import QtQuick.Layouts  1.11

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Palette               1.0
import MAVLink                              1.0

import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Controllers   1.0

Item {
    id: _root

    property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    property var _vcu: _activeVehicle.vcu
    property bool _weightDataAvailable: (_vcu.weightADCValue.rawValue !== 0) ? true : false
    property int scaleFactor: 2

    property int zeroCalibType: 1
    property int tankCalibType: 2

    property bool sprayerON: false

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    Rectangle {
        width:  dataCol.width   + ScreenTools.defaultFontPixelWidth  * 3
        height: dataCol.height  + ScreenTools.defaultFontPixelHeight * 2
        radius: ScreenTools.defaultFontPixelHeight * 0.5
        color:  qgcPal.window
        border.color:   qgcPal.text
        Column {
            id:                 dataCol
            spacing:            ScreenTools.defaultFontPixelHeight * 0.5
            width:              Math.max(dataGrid.width,
                                         dataLabel.width,
                                         calibLabel.width,
                                         rowComp.width,
                                         armLockLabel.width,
                                         rowArmstatus.width)

            anchors.margins:    ScreenTools.defaultFontPixelHeight
            anchors.centerIn:   parent

            // function getArmlockedColor(armValue) {
            //     switch (armValue) {
            //     case 0: //Arm locked
            //         return qgcPal.colorGreen
            //     case 1: //Arm unlocked
            //         return qgcPal.colorRed
            //     default:
            //         return qgcPal.colorOrange
            //     }
            // }

            QGCLabel {
                id:             armLockLabel
                text:           qsTr("Arm lock status")
                font.family:    ScreenTools.demiboldFontFamily
                anchors.horizontalCenter: parent.horizontalCenter
            }

            GridLayout {
                id:         rowArmstatus
                columnSpacing:      ScreenTools.defaultFontPixelWidth * 1.5
                columns:            2
                anchors.horizontalCenter: parent.horizontalCenter

                Rectangle {
                    id: armTopLeft
                    width:  ScreenTools.defaultFontPixelWidth * scaleFactor
                    height: ScreenTools.defaultFontPixelWidth * scaleFactor
                    radius: ScreenTools.defaultFontPixelWidth * scaleFactor
                    visible: true
                    color: (_vcu.armThird.rawValue === 0) ? qgcPal.colorRed : qgcPal.colorGreen
                    border.color:   qgcPal.text

                }

                Rectangle {
                    id: armTopRight
                    width:  ScreenTools.defaultFontPixelWidth * scaleFactor
                    height: ScreenTools.defaultFontPixelWidth * scaleFactor
                    radius: ScreenTools.defaultFontPixelWidth * scaleFactor
                    visible: true
                    color: (_vcu.armFirst.rawValue === 0) ? qgcPal.colorRed : qgcPal.colorGreen
                    border.color:   qgcPal.text

                }

                Rectangle {
                    id: armBotLeft
                    width:  ScreenTools.defaultFontPixelWidth * scaleFactor
                    height: ScreenTools.defaultFontPixelWidth * scaleFactor
                    radius: ScreenTools.defaultFontPixelWidth * scaleFactor
                    visible: true
                    color: (_vcu.armSecond.rawValue === 0) ? qgcPal.colorRed : qgcPal.colorGreen
                    border.color:   qgcPal.text
                }

                Rectangle {
                    id: armBotRight
                    width:  ScreenTools.defaultFontPixelWidth * scaleFactor
                    height: ScreenTools.defaultFontPixelWidth * scaleFactor
                    radius: ScreenTools.defaultFontPixelWidth * scaleFactor
                    visible: true
                    color: (_vcu.armFourth.rawValue === 0) ? qgcPal.colorRed : qgcPal.colorGreen
                    border.color:   qgcPal.text
                }
            }

            QGCLabel {
                id:             dataLabel
                text:           qsTr("Spray Device Info")
                font.family:    ScreenTools.demiboldFontFamily
                anchors.horizontalCenter: parent.horizontalCenter
            }

            GridLayout {
                id:                 dataGrid
                // anchors.margins:    ScreenTools.defaultFontPixelHeight
                columnSpacing:      ScreenTools.defaultFontPixelWidth
                columns:            3
                // anchors.horizontalCenter: parent.horizontalCenter

                ///Tilte
                QGCLabel { text: qsTr("") }
                QGCLabel { text: qsTr("CH1") }
                QGCLabel { text: qsTr("CH2") }

                ///Device info
                QGCLabel { text: qsTr("Pump speed:") }
                QGCLabel { text: _activeVehicle ? _vcu.pumpRPMFirstChannel.valueString : qsTr("N/A", "No data to display") }
                QGCLabel { text: _activeVehicle ? _vcu.pumpRPMSecondChannel.valueString : qsTr("N/A", "No data to display") }
                QGCLabel { text: qsTr("Nozzle speed:") }
                QGCLabel { text: _activeVehicle ? _vcu.centrifugalRPMFirstChannel.valueString : qsTr("N/A", "No data to display") }
                QGCLabel { text: _activeVehicle ? _vcu.centrifugalRPMSecondChannel.valueString : qsTr("N/A", "No data to display") }
                QGCLabel { text: qsTr("Flowrate:") }
                QGCLabel { text: _activeVehicle ? _vcu.flowspeedFirstChannel.valueString : qsTr("N/A", "No data to display") }
                QGCLabel { text: _activeVehicle ? _vcu.flowspeedSecondChannel.valueString :qsTr("N/A", "No data to display") }
                QGCLabel { text: qsTr("Fuel:") }
                QGCLabel { text: _activeVehicle ? _vcu.fuelTankPct.valueString + "%" :qsTr("N/A", "No data to display") }
                QGCLabel { text: qsTr("") }
            }

            QGCLabel {
                id:             calibLabel
                text:           qsTr("Fuel calibration")
                font.family:    ScreenTools.demiboldFontFamily
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Row {
                id:         rowComp
                spacing:    ScreenTools.defaultFontPixelWidth * 3
                height:     Math.max(zeroCalib.height, tankCalib.height)
                anchors.horizontalCenter: parent.horizontalCenter

                QGCButton {
                    id:                     zeroCalib
                    anchors.margins:     ScreenTools.defaultFontPixelWidth * 0.5
                    text:                   qsTr("Zero")
                    visible:                true
                    onClicked: {
                        _activeVehicle.triggerSprayCalibration(sprayerON, zeroCalibType);
                    }
                }

                QGCButton {
                    id:                     tankCalib
                    anchors.margins:    ScreenTools.defaultFontPixelWidth * 0.5
                    text:                   qsTr("Tank")
                    visible:                true
                    enabled:                _weightDataAvailable
                    property string _tankCalibText: "Tank calibration"
                    onClicked: {
                        mainWindow.showComponentDialog(sprayCalibrateDialogComponent, _tankCalibText, mainWindow.showDialogDefaultWidth, StandardButton.Cancel | StandardButton.Ok)
                    }
                }
            }
        }
        Component {
            id: sprayCalibrateDialogComponent

            QGCViewDialog {
                id: sprayCalibrateDialog

                function accept() {
                    //Do something here
                    mainWindow.showPopupDialogFromComponent(sprayCalibrationPopup)
                    //Hiding dialog
                    sprayCalibrateDialog.hideDialog()
                }

                QGCLabel {
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    wrapMode:       Text.WordWrap
                    text:           qsTr("To calibrate fuel tank, make sure that data received from VCU Board is normal then press Ok.")
                }
            } // QGCViewDialog
        } // Component - sprayCalibrateDialogComponent
    }

    Component {
        id: sprayCalibrationPopup
        SprayCalibrationPopup {
        }
    }


}




