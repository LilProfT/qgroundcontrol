/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.11
import QtQuick.Controls 2.4
import QtQuick.Dialogs  1.3
import QtQuick.Layouts 1.2

import QGroundControl               1.0
import QGroundControl.Vehicle       1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Palette       1.0

import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Controllers   1.0

/// Popup container for tank calibration
QGCPopupDialog {
    id:         _root
    title:      qsTr("Fuel calibration")
    buttons:    StandardButton.Close

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    property var _vcu: _activeVehicle.vcu

    property Fact _fuelTankLiters: calibControl.getParameterFact(-1,"VCU_FUEL_TANK")
    property bool _validTextFieldValue: calibControl.isValidMeasuredValue
    property bool _isTextFieldValueEdited: calibControl.isTextFieldValueEdited

    property string invalidValueText: qsTr("Value must be between 0 and ") + qsTr(_fuelTankLiters.valueString)
    property string isValueEditedText: qsTr("Input measured value to continue")
    property string defaultDescriptMessage: qsTr("Click Start to begin")

    Item {
        width:  Math.max(botRow.width, topRow.width) + ScreenTools.defaultFontPixelWidth
        // leftCol.width + rightCol.width + ScreenTools.defaultFontPixelWidth  * 3 + columnSpacer.width
        height: botRow.height + topRow.height + ScreenTools.defaultFontPixelHeight + rowSpacer.height

        // anchors.centerIn: parent
        // anchors.margins:    ScreenTools.defaultFontPixelHeight

        APMSprayCalibrationController{
            id: calibControl
            nextButton: nextButton
            cancelButton: cancelButton
            startButton: startButton
            calibrateHelpText: descriptMsg
            measuredTextField: measuredVolumeTextField

            onCalibCompleteSign: {
                switch(type) {
                case APMSprayCalibrationController.CalTypeTank:
                    mainWindow.showPopupDialogFromComponent(postCalibPopupDialogComponent)
                    break;
                }
            }
        }

        Component {
            id: postCalibPopupDialogComponent

            QGCPopupDialog {
                title:      qsTr("Notification")
                buttons:    StandardButton.Close

                Item {
                    width:  postCalibText.width + ScreenTools.defaultFontPixelWidth
                    height:  postCalibText.height + ScreenTools.defaultFontPixelHeight * 0.5
                    anchors.horizontalCenter:   parent.horizontalCenter

                    QGCLabel {
                        id: postCalibText
                        wrapMode:       Text.WordWrap
                        font.family:    ScreenTools.demiboldFontFamily
                        text:           qsTr("Calibration finished")
                    }
                }

                onHideDialog: {
                    _root.hideDialog()
                }

            }
        }

        Column {
            id: botRow
            anchors.top: rowSpacer.bottom
            anchors.left: parent.left
            spacing:        ScreenTools.defaultFontPixelHeight
            width: Math.max(columnLayout.width, leftColButton.width)

            RowLayout {
                id:         columnLayout
                spacing: ScreenTools.defaultFontPixelWidth


                QGCLabel { text: qsTr("Measured liters:") }

                QGCTextField {
                    id: measuredVolumeTextField
                    enabled: false
                    onEditingFinished: {
                        calibControl.saveMeasuredParam(parseFloat(text))
                    }
                }
            }

            QGCLabel {
                text: (_isTextFieldValueEdited) ? (_validTextFieldValue ? "" : invalidValueText) : isValueEditedText
                wrapMode:       Text.WordWrap
                visible: calibControl.calibSecondStepInProgress
                color: qgcPal.colorRed
            }

            RowLayout {
                id: leftColButton

                QGCButton {
                    id: cancelButton
                    text: qsTr("Cancel")
                    enabled: false
                    onClicked:  {
                        calibControl.cancelCalibration()
                        _root.hideDialog()
                    }
                }

                QGCButton {
                    id: nextButton
                    text: qsTr("Next")
                    enabled: false
                    onClicked:  calibControl.nextButtonClicked()
                }

                QGCButton {
                    id: startButton
                    text: qsTr("Start")
                    onClicked: calibControl.calibrateTank()
                }
            }
        }
        Item {
            id:            rowSpacer
            anchors.top:   topRow.bottom
            height:        ScreenTools.defaultFontPixelHeight * 1.5
            width:         parent.width
        }

        // Right Column Top Row
        Column {
            id: topRow
            spacing: ScreenTools.defaultFontPixelHeight
            // anchors.left: columnSpacer.right
            anchors.top: parent.top
            anchors.horizontalCenter: parent.horizontalCenter

            QGCLabel {
                id:             descriptMsg
                anchors.left:   parent.left
                width:          parent.width
                wrapMode:       Text.WordWrap
                text:           defaultDescriptMessage
                font.family:    ScreenTools.demiboldFontFamily
            }

            RowLayout {
                spacing: ScreenTools.defaultFontPixelWidth * 1.5

                SprayImgCal {
                    visible:            true
                    calValid:           calibControl.calibFirstStepDone
                    calInProgress:      calibControl.calibFirstStepInProgress
                    calInProgressText:  qsTr("Steady")
                    imgSrc:             "/res/gasTankEmpty.png"
                }

                SprayImgCal {
                    visible:            true
                    calValid:           calibControl.calibSecondStepDone
                    calInProgress:      calibControl.calibSecondStepInProgress
                    calInProgressText:  qsTr("Steady")
                    imgSrc:             "/res/gasTank.png"
                }

                SprayImgCal {
                    visible:            true
                    calValid:           calibControl.calibThirdStepDone
                    calInProgress:      calibControl.calibThirdStepInProgresss
                    calInProgressText:  qsTr("Steady")
                    imgSrc:             "/res/gasTankFull.png"
                }
            } // Column
        }
    }
}
