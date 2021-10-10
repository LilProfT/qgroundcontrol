import QtQuick          2.3
import QtQuick.Controls 1.2
import QtQuick.Dialogs  1.2
import QtQuick.Extras   1.4
import QtQuick.Layouts  1.2

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Vehicle       1.0
import QGroundControl.Controls      1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Palette       1.0
import QGroundControl.FlightMap     1.0

Rectangle {
    id:         _root
    height:     visible ? (editorColumn.height + (_margin * 2)) : 0
    width:      availableWidth
    color:      qgcPal.windowShadeDark
    radius:     _radius

    // The following properties must be available up the hierarchy chain
    //property real   availableWidth    ///< Width for control
    //property var    missionItem       ///< Mission Item for editor

    property real   _margin:                    ScreenTools.defaultFontPixelWidth / 2
    property real   _fieldWidth:                ScreenTools.defaultFontPixelWidth * 10.5
    property var    _vehicle:                   QGroundControl.multiVehicleManager.activeVehicle ? QGroundControl.multiVehicleManager.activeVehicle : QGroundControl.multiVehicleManager.offlineEditingVehicle
    property real   _cameraMinTriggerInterval:  _missionItem.cameraCalc.minTriggerInterval.rawValue
    property bool   _polygonDone:               false
    property string _doneAdjusting:             qsTr("Done")
    property var    _missionItem:               missionItem
    property bool   _presetsAvailable:          _missionItem.presetNames.length !== 0
    property int _marginSlider : 20

    function polygonCaptureStarted() {
        _missionItem.clearPolygon()
    }

    function polygonCaptureFinished(coordinates) {
        for (var i=0; i<coordinates.length; i++) {
            _missionItem.addPolygonCoordinate(coordinates[i])
        }
    }

    function polygonAdjustVertex(vertexIndex, vertexCoordinate) {
        _missionItem.adjustPolygonCoordinate(vertexIndex, vertexCoordinate)
    }

    function polygonAdjustStarted() { }
    function polygonAdjustFinished() { }

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    Column {
        id:                 editorColumn
        anchors.margins:    _margin
        anchors.top:        parent.top
        anchors.left:       parent.left
        anchors.right:      parent.right

        ColumnLayout {
            id:             wizardColumn
            anchors.left:   parent.left
            anchors.right:  parent.right
            spacing:        _margin
            visible:        !_missionItem.surveyAreaPolygon.isValid || _missionItem.wizardMode

            ColumnLayout {
                Layout.fillWidth:   true
                spacing:             _margin
                visible:            !_polygonDone

                QGCLabel {
                    Layout.fillWidth:       true
                    wrapMode:               Text.WordWrap
                    horizontalAlignment:    Text.AlignHCenter
                    text:                   qsTr("Use the Polygon Tools to create the polygon which outlines your survey area.")
                }
            }
        }

        Column {
            anchors.left:   parent.left
            anchors.right:  parent.right
            spacing:        _margin
            visible:        !wizardColumn.visible

            QGCTabBar {
                id:             tabBar
                width:          parent.width
                anchors.left:   parent.left
                anchors.right:  parent.right

                Component.onCompleted: currentIndex = QGroundControl.settingsManager.planViewSettings.displayPresetsTabFirst.rawValue ? 1 : 0

                QGCTabButton { icon.source: "/qmlimages/PatternGrid.png"; icon.height: ScreenTools.defaultFontPixelHeight }
//                QGCTabButton { icon.source: "/qmlimages/PatternTerrain.png"; icon.height: ScreenTools.defaultFontPixelHeight }
                QGCTabButton { icon.source: "/qmlimages/PatternPresets.png"; icon.height: ScreenTools.defaultFontPixelHeight }
            }


            // Grid tab

            Column {
                anchors.left:       parent.left
                anchors.right:      parent.right
                spacing:            _margin
                visible:            tabBar.currentIndex === 0

                QGCLabel {
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    text:           qsTr("WARNING: Photo interval is below minimum interval (%1 secs) supported by camera.").arg(_cameraMinTriggerInterval.toFixed(1))
                    wrapMode:       Text.WordWrap
                    color:          qgcPal.warningText
                    visible:        _missionItem.cameraShots > 0 && _cameraMinTriggerInterval !== 0 && _cameraMinTriggerInterval > _missionItem.timeBetweenShots
                }

                CameraCalcGrid {
                    cameraCalc:                     _missionItem.cameraCalc
                    vehicleFlightIsFrontal:         true
                    distanceToSurfaceLabel:         qsTr("Altitude")
                    distanceToSurfaceAltitudeMode:  _missionItem.followTerrain ?
                                                        QGroundControl.AltitudeModeCalcAboveTerrain :
                                                        (_missionItem.cameraCalc.distanceToSurfaceRelative ? QGroundControl.AltitudeModeRelative : QGroundControl.AltitudeModeAbsolute)
                    //frontalDistanceLabel:           qsTr("Trigger Dist")
                    sideDistanceLabel:              qsTr("Spacing")
                }

                SectionHeader {
                    id:             sprayConfigHeader
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    text:           qsTr("Spray Configuration")
                }

                GridLayout {
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    columnSpacing:  _margin
                    rowSpacing:     _margin
                    columns:        2
                    visible:        sprayConfigHeader.checked
                    width:          _root.width - _margin*2

                    QGCLabel { text: qsTr("Application Rate") }
                    FactTextField {
                        fact:                   _missionItem.applicationRate
                        Layout.fillWidth:       true
                    }

                    QGCLabel { text: qsTr("Settle Velocity") }
//                    QGCButton {
//                                        Layout.alignment: Qt.AlignRight
//                                        width: 20
//                                        height: 20
//                                        text: qsTr("+")
//                                        onClicked: {
//                                            //_root.value = _root.value + _root.stepSize
//                                        }
//                                    }
                    FactTextField {
                        fact:                   _missionItem.velocity
                        Layout.fillWidth:       true
                        onUpdated:              velocitySlider.value = _missionItem.velocity.value
                        visible:                false
                    }
                    QGCLabel {
                        text:               _missionItem.velocity.value.toFixed(1) + " " + _missionItem.velocity.units
//                        font.pointSize:     ScreenTools.mediumFontPointSize
                        Layout.alignment:  Qt.AlignRight
                    }

                    QGCSlider {
                        id:                     velocitySlider
                        minimumValue:           4.0
                        maximumValue:           7.0
                        stepSize:               0.1
                        tickmarksEnabled:       false
                        Layout.leftMargin: _marginSlider
                        Layout.rightMargin: _marginSlider

                        Layout.fillWidth:       true
                        Layout.columnSpan:      2
                        Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 1.5
                        value:                  _missionItem.velocity.value
                        onValueChanged:         _missionItem.velocity.value = value
                        Component.onCompleted:  value = _missionItem.velocity.value
                        updateValueWhileDragging: true
                    }

                    QGCLabel { text: qsTr("Nozzle Rate"); visible: false }
                    FactTextField {
                        fact:                   _missionItem.nozzleRate
                        Layout.fillWidth:       true
                        visible:                false
                    }

                    QGCLabel { text: qsTr("Nozzle Offset"); visible: false }
                    FactTextField {
                        fact:                   _missionItem.nozzleOffset
                        Layout.fillWidth:       true
                        visible:                false
                    }

                    QGCLabel { text: qsTr("Spray Flow Rate") }
                    FactTextField {
                        fact:                   _missionItem.sprayFlowRate
                        Layout.fillWidth:       true
                        visible:                false
                    }
                    QGCLabel {
                        text:               _missionItem.sprayFlowRate.value.toFixed(2) + " " + _missionItem.sprayFlowRate.units
                        font.pointSize:     ScreenTools.largeFontPointSize
                        color:              qgcPal.globalTheme !== QGCPalette.Light ? "deepskyblue" : "forestgreen"
                        Layout.alignment:   Qt.AlignHCenter
                    }
                }

                SectionHeader {
                    id:             transectsHeader
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    text:           qsTr("Transects")
                }

                GridLayout {
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    columnSpacing:  _margin
                    rowSpacing:     _margin
                    columns:        2
                    visible:        transectsHeader.checked
                    width:          _root.width - _margin*2

                    FactTextField {
                        fact:                   _missionItem.firstLaneOffset
                        Layout.fillWidth:       true
                        onUpdated:              firstLaneOffset.value = _missionItem.firstLaneOffset.value
                        visible:                false
                    }
                    QGCLabel { text: qsTr("1st lane offset"); }
                    QGCLabel {
                        text:                   (_missionItem.firstLaneOffset.value * 100).toFixed() + " %" + _missionItem.firstLaneOffset.units
                        Layout.alignment: Qt.AlignRight
                    }
                    QGCSlider {
                        id:                     firstLaneOffsetSlider
                        minimumValue:           0.05
                        maximumValue:           1
                        stepSize:               0.05
                        tickmarksEnabled:       false
                        Layout.fillWidth:       true
                        Layout.columnSpan:      2
                        Layout.leftMargin: _marginSlider
                        Layout.rightMargin: _marginSlider
                        Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 1.5
                        value:                  _missionItem.firstLaneOffset.value
                        onValueChanged:         _missionItem.firstLaneOffset.value = value
                        Component.onCompleted:  value = _missionItem.firstLaneOffset.value
                        updateValueWhileDragging: true
                    }

                    QGCCheckBox {
                        text:               qsTr("Ascend transect terminals")
                        checked:            _missionItem.ascendTerminals.value
                        onClicked:          _missionItem.ascendTerminals.value = checked
                        Layout.columnSpan: 2
                    }

                    FactTextField {
                        fact:                   _missionItem.ascendAltitude
                        Layout.fillWidth:       true
                        onUpdated:              ascendAltitude.value = _missionItem.ascendAltitude.value
                        visible:                false
                    }
                    QGCLabel { text: qsTr("Ascent Alt"); }
                    QGCLabel {
                        text:                   _missionItem.ascendAltitude.value.toFixed(1) + " " + _missionItem.ascendAltitude.units
                        Layout.alignment:       Qt.AlignRight
                    }
                    QGCSlider {
                        id:                     ascendAltitudeSlider
                        minimumValue:           0
                        maximumValue:           5
                        stepSize:               0.1
                        tickmarksEnabled:       false
                        Layout.fillWidth:       true
                        Layout.columnSpan:      2
                        Layout.leftMargin: _marginSlider
                        Layout.rightMargin: _marginSlider
                        Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 1.5
                        value:                  _missionItem.ascendAltitude.value
                        onValueChanged:         _missionItem.ascendAltitude.value = value
                        Component.onCompleted:  value = _missionItem.ascendAltitude.value
                        updateValueWhileDragging: true
                    }

                    FactTextField {
                        fact:                   _missionItem.ascendLength
                        Layout.fillWidth:       true
                        onUpdated:              ascendLength.value = _missionItem.ascendLength.value
                        visible:                false
                    }
                    QGCLabel { text: qsTr("Ascent Len"); }
                    QGCLabel {
                        text:                   _missionItem.ascendLength.value.toFixed(1) + " " + _missionItem.ascendLength.units
                        Layout.alignment:       Qt.AlignRight
                    }
                    QGCSlider {
                        id:                     ascendLengthSlider
                        minimumValue:           0
                        maximumValue:           5
                        stepSize:               0.1
                        tickmarksEnabled:       false
                        Layout.fillWidth:       true
                        Layout.columnSpan:      2
                        Layout.leftMargin: _marginSlider
                        Layout.rightMargin: _marginSlider
                        Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 1.5
                        value:                  _missionItem.ascendLength.value
                        onValueChanged:         _missionItem.ascendLength.value = value
                        Component.onCompleted:  value = _missionItem.ascendLength.value
                        updateValueWhileDragging: true
                    }

                    QGCCheckBox {
                        text:               qsTr("Auto Optimize")
                        checked:            false
                        onClicked:          _missionItem.autoOptimize.value = checked
                        Layout.columnSpan:  2
                        visible:            false
                    }

                    QGCButton {
                        text:               qsTr("Optimize")
                        Layout.fillWidth:   true
                        onClicked:          _missionItem.optimize();
                    }

                    QGCLabel { text: qsTr("Angle")
                        visible:                false}
                    FactTextField {
                        fact:                   _missionItem.gridAngle
                        Layout.fillWidth:       true
                        onUpdated:              angleSlider.value = _missionItem.gridAngle.value
                        visible:                false
                    }
                    QGCLabel {
                        text:               _missionItem.gridAngle.value.toFixed(1) + " " + _missionItem.gridAngle.units
                        //                      font.pointSize:     ScreenTools.mediumFontPointSize
                        Layout.alignment:   Qt.AlignRight
                        visible:                false
                    }

                    QGCSlider {
                        id:                     angleSlider
                        minimumValue:           0
                        maximumValue:           359
                        stepSize:               1
                        tickmarksEnabled:       false
                        Layout.fillWidth:       true
                        Layout.columnSpan:      2
                        Layout.leftMargin: _marginSlider
                        Layout.rightMargin: _marginSlider
                        Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 1.5
                        value:                  _missionItem.gridAngle.value
                        onValueChanged:         _missionItem.gridAngle.value = value
                        Component.onCompleted:  value = _missionItem.gridAngle.value
                        updateValueWhileDragging: true
                        visible:                false
                    }

                    QGCButton {
                        text:               qsTr("Rotate Angle")
                        onClicked:          _missionItem.rotateAngle();
                        Layout.fillWidth:   true
                    }

                    QGCLabel {
                        text:       qsTr("Turnaround dist")
                        Layout.fillWidth:       true
                        visible:            false
                    }
                    FactTextField {
                        fact:               _missionItem.turnAroundDistance
                        Layout.fillWidth:   true
                        onUpdated:          turnAroundDistSlider.value = _missionItem.turnAroundDistance.value
                        visible:            false
                    }
                    QGCLabel {
                        text:               _missionItem.turnAroundDistance.value.toFixed(2) + " " + _missionItem.turnAroundDistance.units
                        //                        font.pointSize:     ScreenTools.mediumFontPointSize
                        Layout.alignment:   Qt.AlignRight
                        visible:            false
                    }

                    QGCSlider {
                        id:                     turnAroundDistSlider
                        minimumValue:           0
                        maximumValue:           10
                        stepSize:               0.1
                        tickmarksEnabled:       false
                        Layout.fillWidth:       true
                        Layout.columnSpan:      2
                        Layout.leftMargin: _marginSlider
                        Layout.rightMargin: _marginSlider
                        Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 1.5
                        value:                  _missionItem.turnAroundDistance.value
                        onValueChanged:         _missionItem.turnAroundDistance.value = value
                        Component.onCompleted:  value = _missionItem.turnAroundDistance.value
                        updateValueWhileDragging: true
                        visible:            false
                    }

                    QGCButton {
                        text:               qsTr("Rotate Entry Point")
                        Layout.fillWidth:   true
                        Layout.columnSpan:  2
                        onClicked:          _missionItem.rotateEntryPoint();
                    }
                }

                ColumnLayout {
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    spacing:        _margin
                    //visible:        transectsHeader.checked
                    visible:        false

                    QGCOptionsComboBox {
                        Layout.fillWidth: true

                        model: [
                            {
                                text:       qsTr("Hover and capture image"),
                                fact:       _missionItem.hoverAndCapture,
                                enabled:    !_missionItem.followTerrain,
                                visible:    _missionItem.hoverAndCaptureAllowed
                            },
                            {
                                text:       qsTr("Refly at 90 deg offset"),
                                fact:       _missionItem.refly90Degrees,
                                enabled:    !_missionItem.followTerrain,
                                visible:    true
                            },
                            {
                                text:       qsTr("Images in turnarounds"),
                                fact:       _missionItem.cameraTriggerInTurnAround,
                                enabled:    _missionItem.hoverAndCaptureAllowed ? !_missionItem.hoverAndCapture.rawValue : true,
                                visible:    true
                            },
                            {
                                text:       qsTr("Fly alternate transects"),
                                fact:       _missionItem.flyAlternateTransects,
                                enabled:    true,
                                visible:    _vehicle ? (_vehicle.fixedWing || _vehicle.vtol) : false
                            },
                            {
                                text:       qsTr("Relative altitude"),
                                enabled:    _missionItem.cameraCalc.isManualCamera && !_missionItem.followTerrain,
                                visible:    QGroundControl.corePlugin.options.showMissionAbsoluteAltitude || (!_missionItem.cameraCalc.distanceToSurfaceRelative && !_missionItem.followTerrain),
                                checked:    _missionItem.cameraCalc.distanceToSurfaceRelative
                            }
                        ]

                        onItemClicked: {
                            if (index == 4) {
                                _missionItem.cameraCalc.distanceToSurfaceRelative = !_missionItem.cameraCalc.distanceToSurfaceRelative
                                console.log(_missionItem.cameraCalc.distanceToSurfaceRelative)
                            }
                        }
                    }
                }

                SectionHeader {
                    id:             statsHeader
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    text:           qsTr("Statistics")
                    visible: false
                }

                TransectStyleComplexItemStats {
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    visible:        false // statsHeader.checked
                }
            } // Grid Column

            // Camera Tab
            /*Column {
                anchors.left:       parent.left
                anchors.right:      parent.right
                spacing:            _margin
                visible:            tabBar.currentIndex === 1

                CameraCalcCamera {
                    cameraCalc: _missionItem.cameraCalc
                }
            }*/ // Camera Column

            // Terrain Tab
//            TransectStyleComplexItemTerrainFollow {
//                anchors.left:   parent.left
//                anchors.right:  parent.right
//                spacing:        _margin
//                visible:        tabBar.currentIndex === 1
//                missionItem:    _missionItem
//            }

            // Presets Tab
            Column {
                anchors.left:       parent.left
                anchors.right:      parent.right
                spacing:            _margin
                visible:            tabBar.currentIndex === 1

                QGCLabel {
                    //Layout.fillWidth:   true
                    anchors.left:       parent.left
                    anchors.right:      parent.right
                    text:               qsTr("Presets")
                    wrapMode:           Text.WordWrap
                }

                QGCComboBox {
                    id:                 presetCombo
                    //Layout.fillWidth:   true
                    anchors.left:       parent.left
                    anchors.right:      parent.right
                    model:              _missionItem.presetNames
                }

                RowLayout {
                    //Layout.fillWidth:   true
                    anchors.left:       parent.left
                    anchors.right:      parent.right

                    QGCButton {
                        Layout.fillWidth:   true
                        text:               qsTr("Apply Preset")
                        enabled:            _missionItem.presetNames.length != 0
                        onClicked:          _missionItem.loadPreset(presetCombo.textAt(presetCombo.currentIndex))
                    }

                    QGCButton {
                        Layout.fillWidth:   true
                        text:               qsTr("Delete Preset")
                        enabled:            _missionItem.presetNames.length != 0
                        onClicked:          mainWindow.showComponentDialog(deletePresetMessage, qsTr("Delete Preset"), mainWindow.showDialogDefaultWidth, StandardButton.Yes | StandardButton.No)

                        Component {
                            id: deletePresetMessage
                            QGCViewMessage {
                                message: qsTr("Are you sure you want to delete '%1' preset?").arg(presetName)
                                property string presetName: presetCombo.textAt(presetCombo.currentIndex)
                                function accept() {
                                    _missionItem.deletePreset(presetName)
                                    hideDialog()
                                }
                            }
                        }
                    }
                }

                Item { height: ScreenTools.defaultFontPixelHeight; width: 1 }

                QGCButton {
                    Layout.alignment:   Qt.AlignCenter
                    //Layout.fillWidth:   true
                    anchors.left:       parent.left
                    anchors.right:      parent.right
                    text:               qsTr("Save Settings As New Preset")
                    onClicked:          mainWindow.showComponentDialog(savePresetDialog, qsTr("Save Preset"), mainWindow.showDialogDefaultWidth, StandardButton.Save | StandardButton.Cancel)
                }
                /*
                SectionHeader {
                    id:                 presectsTransectsHeader
                    Layout.fillWidth:   true
                    text:               qsTr("Transects")
                }

                GridLayout {
                    Layout.fillWidth:   true
                    columnSpacing:      _margin
                    rowSpacing:         _margin
                    columns:            2
                    visible:            presectsTransectsHeader.checked

                    QGCLabel { text: qsTr("Angle") }
                    FactTextField {
                        fact:                   _missionItem.gridAngle
                        Layout.fillWidth:       true
                        onUpdated:              presetsAngleSlider.value = _missionItem.gridAngle.value
                    }

                    QGCSlider {
                        id:                     presetsAngleSlider
                        minimumValue:           0
                        maximumValue:           359
                        stepSize:               1
                        tickmarksEnabled:       false
                        Layout.fillWidth:       true
                        Layout.columnSpan:      2
                        Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 1.5
                        onValueChanged:         _missionItem.gridAngle.value = value
                        Component.onCompleted:  value = _missionItem.gridAngle.value
                        updateValueWhileDragging: true
                    }

                    QGCButton {
                        Layout.columnSpan:  2
                        Layout.fillWidth:   true
                        text:               qsTr("Rotate Entry Point")
                        onClicked:          _missionItem.rotateEntryPoint();
                    }
                }

                SectionHeader {
                    id:                 presetsStatsHeader
                    Layout.fillWidth:   true
                    text:               qsTr("Statistics")
                }

                TransectStyleComplexItemStats {
                    Layout.fillWidth:   true
                    visible:            presetsStatsHeader.checked
                }*/
                //Mismart Implementation
                Item { height: ScreenTools.defaultFontPixelHeight; width: 1 }

                CameraCalcGrid {
                    cameraCalc:                     _missionItem.cameraCalc
                    vehicleFlightIsFrontal:         true
                    distanceToSurfaceLabel:         qsTr("Altitude")
                    distanceToSurfaceAltitudeMode:  _missionItem.followTerrain ?
                                                        QGroundControl.AltitudeModeCalcAboveTerrain :
                                                        (_missionItem.cameraCalc.distanceToSurfaceRelative ? QGroundControl.AltitudeModeRelative : QGroundControl.AltitudeModeAbsolute)
                    //frontalDistanceLabel:           qsTr("Trigger Dist")
                    sideDistanceLabel:              qsTr("Spacing")
                }

                GridLayout {
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    columnSpacing:  _margin
                    rowSpacing:     _margin
                    columns:        2
                    width:          _root.width - _margin*2


                    QGCLabel { text: qsTr("Settle Velocity") }
                    FactTextField {
                        fact:                   _missionItem.velocity
                        Layout.fillWidth:       true
                        onUpdated:              velocitySlider.value = _missionItem.velocity.value
                        visible:                false
                    }
                    QGCLabel {
                        text:               _missionItem.velocity.value.toFixed(1) + " " + _missionItem.velocity.units
    //                        font.pointSize:     ScreenTools.mediumFontPointSize
                        Layout.alignment:  Qt.AlignRight
                    }

                    QGCSlider {
                        id:                     presetsVelocitySlider
                        minimumValue:           4.0
                        maximumValue:           7.0
                        stepSize:               0.1
                        tickmarksEnabled:       false
                        Layout.fillWidth:       true
                        Layout.columnSpan:      2
                        Layout.leftMargin: _marginSlider
                        Layout.rightMargin: _marginSlider
                        Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 1.5
                        value:                  _missionItem.velocity.value
                        onValueChanged:         _missionItem.velocity.value = value
                        Component.onCompleted:  value = _missionItem.velocity.value
                        updateValueWhileDragging: true
                    }
                }

                SectionHeader {
                    id:                 presectsTransectsHeader
                    anchors.left:       parent.left
                    anchors.right:      parent.right
                    text:               qsTr("Transects")
                }

                GridLayout {
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    columnSpacing:  _margin
                    rowSpacing:     _margin
                    columns:        2
                    visible:        transectsHeader.checked
                    width:          _root.width - _margin*2

                    FactTextField {
                        fact:                   _missionItem.firstLaneOffset
                        Layout.fillWidth:       true
                        onUpdated:              firstLaneOffset.value = _missionItem.firstLaneOffset.value
                        visible:                false
                    }
                    QGCLabel { text: qsTr("1st lane offset"); }
                    QGCLabel {
                        text:                   (_missionItem.firstLaneOffset.value * 100).toFixed() + " %" + _missionItem.firstLaneOffset.units
                        Layout.alignment: Qt.AlignRight
                    }
                    QGCSlider {
                        id:                     presetsFirstLaneOffsetSlider
                        minimumValue:           0.05
                        maximumValue:           1
                        stepSize:               0.05
                        tickmarksEnabled:       false
                        Layout.fillWidth:       true
                        Layout.columnSpan:      2
                        Layout.leftMargin: _marginSlider
                        Layout.rightMargin: _marginSlider
                        Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 1.5
                        value:                  _missionItem.firstLaneOffset.value
                        onValueChanged:         _missionItem.firstLaneOffset.value = value
                        Component.onCompleted:  value = _missionItem.firstLaneOffset.value
                        updateValueWhileDragging: true
                    }

                    QGCCheckBox {
                        text:               qsTr("Ascend transect terminals")
                        checked:            _missionItem.ascendTerminals.value
                        onClicked:          _missionItem.ascendTerminals.value = checked
                        Layout.columnSpan: 2
                    }

                    FactTextField {
                        fact:                   _missionItem.ascendAltitude
                        Layout.fillWidth:       true
                        onUpdated:              ascendAltitude.value = _missionItem.ascendAltitude.value
                        visible:                false
                    }
                    QGCLabel { text: qsTr("Ascent Alt"); }
                    QGCLabel {
                        text:                   _missionItem.ascendAltitude.value.toFixed(1) + " " + _missionItem.ascendAltitude.units
                        Layout.alignment: Qt.AlignRight
                    }
                    QGCSlider {
                        id:                     presetsAscendAltitudeSlider
                        minimumValue:           0
                        maximumValue:           5
                        stepSize:               0.1
                        tickmarksEnabled:       false
                        Layout.fillWidth:       true
                        Layout.columnSpan:      2
                        Layout.leftMargin: _marginSlider
                        Layout.rightMargin: _marginSlider
                        Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 1.5
                        value:                  _missionItem.ascendAltitude.value
                        onValueChanged:         _missionItem.ascendAltitude.value = value
                        Component.onCompleted:  value = _missionItem.ascendAltitude.value
                        updateValueWhileDragging: true
                    }

                    FactTextField {
                        fact:                   _missionItem.ascendLength
                        Layout.fillWidth:       true
                        onUpdated:              ascendLength.value = _missionItem.ascendLength.value
                        visible:                false
                    }
                    QGCLabel { text: qsTr("Ascent Len"); }
                    QGCLabel {
                        text:                   _missionItem.ascendLength.value.toFixed(1) + " " + _missionItem.ascendLength.units
                        Layout.alignment: Qt.AlignRight
                    }
                    QGCSlider {
                        id:                     presetsAscendLengthSlider
                        minimumValue:           0
                        maximumValue:           5
                        stepSize:               0.1
                        tickmarksEnabled:       false
                        Layout.fillWidth:       true
                        Layout.columnSpan:      2
                        Layout.leftMargin: _marginSlider
                        Layout.rightMargin: _marginSlider
                        Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 1.5
                        value:                  _missionItem.ascendLength.value
                        onValueChanged:         _missionItem.ascendLength.value = value
                        Component.onCompleted:  value = _missionItem.ascendLength.value
                        updateValueWhileDragging: true
                    }
                }

            } // Main editing column
        } // Top level  Column

        Component {
            id: savePresetDialog

            QGCViewDialog {
                function accept() {
                    if (presetNameField.text != "") {
                        _missionItem.savePreset(presetNameField.text)
                        hideDialog()
                    }
                }

                ColumnLayout {
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    spacing:        ScreenTools.defaultFontPixelHeight

                    QGCLabel {
                        Layout.fillWidth:   true
                        text:               qsTr("Save the current settings as a named preset.")
                        wrapMode:           Text.WordWrap
                    }

                    QGCLabel {
                        text: qsTr("Preset Name")
                    }

                    QGCTextField {
                        id:                 presetNameField
                        Layout.fillWidth:   true
                    }
                }
            }
        }
    }

    KMLOrSHPFileDialog {
        id:             kmlOrSHPLoadDialog
        title:          qsTr("Select Polygon File")
        selectExisting: true

        onAcceptedForLoad: {
            _missionItem.surveyAreaPolygon.loadKMLOrSHPFile(file)
            _missionItem.resetState = false
            //editorMap.mapFitFunctions.fitMapViewportTo_missionItems()
            close()
        }
    }
} // Rectangle
