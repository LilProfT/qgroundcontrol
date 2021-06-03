import QtQuick          2.3
import QtQuick.Controls 1.2
import QtQuick.Layouts  1.2

import QGroundControl                   1.0
import QGroundControl.ScreenTools       1.0
import QGroundControl.Controls          1.0
import QGroundControl.FactControls      1.0
import QGroundControl.Palette           1.0

// Camera calculator "Grid" section for mission item editors
Column {
    anchors.left:   parent.left
    anchors.right:  parent.right
    spacing:        _margin

    property var    cameraCalc
    property bool   vehicleFlightIsFrontal:         true
    property string distanceToSurfaceLabel
    property int    distanceToSurfaceAltitudeMode:  QGroundControl.AltitudeModeNone
    property string frontalDistanceLabel
    property string sideDistanceLabel

    property real   _margin:            ScreenTools.defaultFontPixelWidth / 2
    property string _cameraName:        cameraCalc.cameraName.value
    property real   _fieldWidth:        ScreenTools.defaultFontPixelWidth * 10.5
    property var    _cameraList:        [ ]
    property var    _vehicle:           QGroundControl.multiVehicleManager.activeVehicle ? QGroundControl.multiVehicleManager.activeVehicle : QGroundControl.multiVehicleManager.offlineEditingVehicle
    property var    _vehicleCameraList: _vehicle ? _vehicle.staticCameraList : []
    property bool   _cameraComboFilled: false

    readonly property int _gridTypeManual:          0
    readonly property int _gridTypeCustomCamera:    1
    readonly property int _gridTypeCamera:          2

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    Column {
        anchors.left:   parent.left
        anchors.right:  parent.right
        spacing:        _margin
        visible:        !cameraCalc.isManualCamera

        RowLayout {
            anchors.left:   parent.left
            anchors.right:  parent.right
            spacing:        _margin
            Item { Layout.fillWidth: true }
            QGCLabel {
                Layout.preferredWidth:  _root._fieldWidth
                text:                   qsTr("Front Lap")
            }
            QGCLabel {
                Layout.preferredWidth:  _root._fieldWidth
                text:                   qsTr("Side Lap")
            }
        }

        RowLayout {
            anchors.left:   parent.left
            anchors.right:  parent.right
            spacing:        _margin
            QGCLabel { text: qsTr("Overlap"); Layout.fillWidth: true }
            FactTextField {
                Layout.preferredWidth:  _root._fieldWidth
                fact:                   cameraCalc.frontalOverlap
            }
            FactTextField {
                Layout.preferredWidth:  _root._fieldWidth
                fact:                   cameraCalc.sideOverlap
            }
        }

        QGCLabel {
            wrapMode:               Text.WordWrap
            text:                   qsTr("Select one:")
            Layout.preferredWidth:  parent.width
            Layout.columnSpan:      2
        }

        GridLayout {
            anchors.left:   parent.left
            anchors.right:  parent.right
            columnSpacing:  _margin
            rowSpacing:     _margin
            columns:        2

            QGCRadioButton {
                id:                     fixedDistanceRadio
                leftPadding:            0
                text:                   distanceToSurfaceLabel
                checked:                !!cameraCalc.valueSetIsDistance.value
                onClicked:              cameraCalc.valueSetIsDistance.value = 1
            }

            AltitudeFactTextField {
                fact:                       cameraCalc.distanceToSurface
                altitudeMode:               distanceToSurfaceAltitudeMode
                enabled:                    fixedDistanceRadio.checked
                Layout.fillWidth:           true
            }

            QGCRadioButton {
                id:                     fixedImageDensityRadio
                leftPadding:            0
                text:                   qsTr("Grnd Res")
                checked:                !cameraCalc.valueSetIsDistance.value
                onClicked:              cameraCalc.valueSetIsDistance.value = 0
            }

            FactTextField {
                fact:                   cameraCalc.imageDensity
                enabled:                fixedImageDensityRadio.checked
                Layout.fillWidth:       true
            }
        }
    } // Column - Camera spec based ui

    // No camera spec ui
    GridLayout {
        anchors.left:   parent.left
        anchors.right:  parent.right
        columnSpacing:  _margin
        rowSpacing:     _margin
        columns:        2
        visible:        cameraCalc.isManualCamera

        QGCLabel { text: distanceToSurfaceLabel }
        AltitudeFactTextField {
            fact:                       cameraCalc.distanceToSurface
            altitudeMode:               distanceToSurfaceAltitudeMode
            Layout.fillWidth:           true
        }

        QGCLabel { text: frontalDistanceLabel }
        FactTextField {
            Layout.fillWidth:   true
            fact:               cameraCalc.adjustedFootprintFrontal
        }

        QGCLabel { text: sideDistanceLabel }
        FactTextField {
            Layout.fillWidth:   true
            fact:               cameraCalc.adjustedFootprintSide
            onUpdated:          {
                sideDistanceSlider.value = cameraCalc.adjustedFootprintSide.value
                cameraCalc.adjustedFootprintSide.value = _vehicle.spacing.value
            }
            visible:            false
        }
        QGCLabel {
            text:                   cameraCalc.adjustedFootprintSide.value.toFixed(2) + " " + cameraCalc.adjustedFootprintSide.units
            Layout.alignment:       Qt.AlignRight
        }
        QGCSlider {
            id:                     sideDistanceSlider
            minimumValue:           3
            maximumValue:           10
            stepSize:               0.1
            tickmarksEnabled:       false
            Layout.fillWidth:       true
            Layout.columnSpan:      2
            Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 1.5
            value:                  cameraCalc.adjustedFootprintSide.value
            onValueChanged:         {
                cameraCalc.adjustedFootprintSide.value = value
                //Mismart: Carbon-copy of the cameraCalc spacing, should be improved in the future
                _vehicle.spacing.value = cameraCalc.adjustedFootprintSide.value
            }
            Component.onCompleted:  {
                value = cameraCalc.adjustedFootprintSide.value
                _vehicle.spacing.value = cameraCalc.adjustedFootprintSide.value
            }
            updateValueWhileDragging: true
        }
    } // GridLayout
} // Column
