import QtQuick          2.3
import QtQuick.Controls 1.2
import QtQuick.Layouts  1.2
import QtQuick.Dialogs  1.2

import QGroundControl                   1.0
import QGroundControl.ScreenTools       1.0
import QGroundControl.Controls          1.0
import QGroundControl.FactControls      1.0
import QGroundControl.Palette           1.0

// Toolbar for Plan View
Item {
    width: missionStats.width + _margins

    property var    _planMasterController:      globals.planMasterControllerPlanView
    property var    _currentMissionItem:        globals.currentPlanMissionItem          ///< Mission item to display status for

    property var    missionItems:               _controllerValid ? _planMasterController.missionController.visualItems : undefined
    property real   missionDistance:            _controllerValid ? _planMasterController.missionController.missionDistance : NaN
    property real   missionTime:                _controllerValid ? _planMasterController.missionController.missionTime : NaN
    property real   area:                       _controllerValid ? _planMasterController.area : NaN
    property real   applicationRate:            (_controllerValid && _planMasterController.applicationRate) ? _planMasterController.applicationRate.value : NaN
    property real   flowRate:                   (_controllerValid && _planMasterController.flowRate) ? _planMasterController.flowRate.value : NaN
    property real   spacing:                    (_controllerValid && _planMasterController.spacing) ? _planMasterController.spacing.value : NaN
    property real   velocity:                   (_controllerValid && _planMasterController.velocity) ? _planMasterController.velocity.value : NaN
    property real   missionMaxTelemetry:        _controllerValid ? _planMasterController.missionController.missionMaxTelemetry : NaN
    property bool   missionDirty:               _controllerValid ? _planMasterController.missionController.dirty : false

    property bool   _controllerValid:           _planMasterController !== undefined && _planMasterController !== null
    property bool   _controllerOffline:         _controllerValid ? _planMasterController.offline : true
    property var    _controllerDirty:           _controllerValid ? _planMasterController.dirty : false
    property var    _controllerSyncInProgress:  _controllerValid ? _planMasterController.syncInProgress : false
    property var    _controllerIsSourcePlan:    _controllerValid ? _planMasterController.isSourcePlan : false

    property bool   _currentMissionItemValid:   _currentMissionItem && _currentMissionItem !== undefined && _currentMissionItem !== null
    property bool   _curreItemIsFlyThrough:     _currentMissionItemValid && _currentMissionItem.specifiesCoordinate && !_currentMissionItem.isStandaloneCoordinate
    property bool   _currentItemIsVTOLTakeoff:  _currentMissionItemValid && _currentMissionItem.command == 84
    property bool   _missionValid:              missionItems !== undefined

    property real   _dataFontSize:              ScreenTools.defaultFontPointSize
    property real   _largeValueWidth:           ScreenTools.defaultFontPixelWidth * 8
    property real   _mediumValueWidth:          ScreenTools.defaultFontPixelWidth * 4
    property real   _smallValueWidth:           ScreenTools.defaultFontPixelWidth * 3
    property real   _labelToValueSpacing:       ScreenTools.defaultFontPixelWidth
    property real   _rowSpacing:                ScreenTools.isMobile ? 1 : 0
    property real   _distance:                  _currentMissionItemValid ? _currentMissionItem.distance : NaN
    property real   _altDifference:             _currentMissionItemValid ? _currentMissionItem.altDifference : NaN
    property real   _azimuth:                   _currentMissionItemValid ? _currentMissionItem.azimuth : NaN
    property real   _heading:                   _currentMissionItemValid ? _currentMissionItem.missionVehicleYaw : NaN
    property real   _missionDistance:           _missionValid ? missionDistance : NaN
    property real   _missionMaxTelemetry:       _missionValid ? missionMaxTelemetry : NaN
    property real   _missionTime:               _missionValid ? missionTime : 0
    property int    _batteryChangePoint:        _controllerValid ? _planMasterController.missionController.batteryChangePoint : -1
    property int    _batteriesRequired:         _controllerValid ? _planMasterController.missionController.batteriesRequired : -1
    property bool   _batteryInfoAvailable:      _batteryChangePoint >= 0 || _batteriesRequired >= 0
    property real   _controllerProgressPct:     _controllerValid ? _planMasterController.missionController.progressPct : 0
    property bool   _syncInProgress:            _controllerValid ? _planMasterController.missionController.syncInProgress : false
    property real   _gradient:                  _currentMissionItemValid && _currentMissionItem.distance > 0 ?
                                                    (_currentItemIsVTOLTakeoff ?
                                                         0 :
                                                         (Math.atan(_currentMissionItem.altDifference / _currentMissionItem.distance) * (180.0/Math.PI)))
                                                  : NaN

    property string _distanceText:              isNaN(_distance) ?              "-.-" : QGroundControl.unitsConversion.metersToAppSettingsHorizontalDistanceUnits(_distance).toFixed(1) + " " + QGroundControl.unitsConversion.appSettingsHorizontalDistanceUnitsString
    property string _altDifferenceText:         isNaN(_altDifference) ?         "-.-" : QGroundControl.unitsConversion.metersToAppSettingsHorizontalDistanceUnits(_altDifference).toFixed(1) + " " + QGroundControl.unitsConversion.appSettingsHorizontalDistanceUnitsString
    property string _gradientText:              isNaN(_gradient) ?              "-.-" : _gradient.toFixed(0) + " deg"
    property string _azimuthText:               isNaN(_azimuth) ?               "-.-" : Math.round(_azimuth) % 360
    property string _headingText:               isNaN(_azimuth) ?               "-.-" : Math.round(_heading) % 360
    property string _missionDistanceText:       isNaN(_missionDistance) ?       "-.-" : QGroundControl.unitsConversion.metersToAppSettingsHorizontalDistanceUnits(_missionDistance).toFixed(0) + " " + QGroundControl.unitsConversion.appSettingsHorizontalDistanceUnitsString
    property string _missionMaxTelemetryText:   isNaN(_missionMaxTelemetry) ?   "-.-" : QGroundControl.unitsConversion.metersToAppSettingsHorizontalDistanceUnits(_missionMaxTelemetry).toFixed(0) + " " + QGroundControl.unitsConversion.appSettingsHorizontalDistanceUnitsString
    property string _batteryChangePointText:    _batteryChangePoint < 0 ?       "N/A" : _batteryChangePoint
    property string _batteriesRequiredText:     _batteriesRequired < 0 ?        "N/A" : _batteriesRequired

    //Mismart: Icons for the QGCLabels instead
    property real   _iconWidth:                 _smallValueWidth / 2

    readonly property real _margins: ScreenTools.defaultFontPixelWidth

   function getMissionTime() {
        if (!_missionTime) {
            return "00:00:00"
        }
        var t = new Date(2021, 0, 0, 0, 0, Number(_missionTime))
        var days = Qt.formatDateTime(t, 'dd')
        var complete

        if (days == 31) {
            days = '0'
            complete = Qt.formatTime(t, 'hh:mm:ss')
        } else {
            complete = days + " days " + Qt.formatTime(t, 'hh:mm:ss')
        }
        return complete
    }


    function getArea() {
        var num
        if (isNaN(area)) {
            num = "0"
        } else {
            num = QGroundControl.unitsConversion.squareMetersToAppSettingsAreaUnits(area).toFixed(2)
        }
        return num + " " + QGroundControl.unitsConversion.appSettingsAreaUnitsString
    }

    function getSpacing() {
        var num
        if (isNaN(spacing)) {
            return "?"
        } else {
            num = QGroundControl.unitsConversion.metersToAppSettingsHorizontalDistanceUnits(spacing).toFixed(2)
        }
        return num + " " + QGroundControl.unitsConversion.appSettingsHorizontalDistanceUnitsString
    }

    // Progress bar
    Connections {
        target: _controllerValid ? _planMasterController.missionController : null
        onProgressPctChanged: {
            if (_controllerProgressPct === 1) {
                missionStats.visible = false
                uploadCompleteText.visible = true
                progressBar.visible = false
                resetProgressTimer.start()
            } else if (_controllerProgressPct > 0) {
                progressBar.visible = true
            }
        }
    }

    Timer {
        id:             resetProgressTimer
        interval:       5000
        onTriggered: {
            missionStats.visible = true
            uploadCompleteText.visible = false
        }
    }

    QGCLabel {
        id:                     uploadCompleteText
        anchors.fill:           parent
        font.pointSize:         ScreenTools.largeFontPointSize
        horizontalAlignment:    Text.AlignHCenter
        verticalAlignment:      Text.AlignVCenter
        text:                   "Done"
        visible:                false
    }

    GridLayout {
        id:                     missionStats
        anchors.top:            parent.top
        anchors.bottom:         parent.bottom
        anchors.leftMargin:     _margins
        anchors.left:           parent.left
        columnSpacing:          0
        columns:                4

        GridLayout {
            visible: false
            columns:                8
            rowSpacing:             _rowSpacing
            columnSpacing:          _labelToValueSpacing
            Layout.alignment:       Qt.AlignVCenter | Qt.AlignHCenter

            QGCLabel {
                text:               qsTr("Selected Waypoint")
                Layout.columnSpan:  8
                font.pointSize:     ScreenTools.smallFontPointSize
            }

            QGCLabel { text: qsTr("Alt diff:"); font.pointSize: _dataFontSize; }
            QGCLabel {
                text:                   _altDifferenceText
                font.pointSize:         _dataFontSize
                Layout.minimumWidth:    _mediumValueWidth
            }

            Item { width: 1; height: 1 }

            QGCLabel { text: qsTr("Azimuth:"); font.pointSize: _dataFontSize; }
            QGCLabel {
                text:                   _azimuthText
                font.pointSize:         _dataFontSize
                Layout.minimumWidth:    _smallValueWidth
            }

            Item { width: 1; height: 1 }

            QGCLabel { text: qsTr("Distance:"); font.pointSize: _dataFontSize; }
            QGCLabel {
                text:                   _distanceText
                font.pointSize:         _dataFontSize
                Layout.minimumWidth:    _largeValueWidth
            }

            QGCLabel { text: qsTr("Gradient:"); font.pointSize: _dataFontSize; }
            QGCLabel {
                text:                   _gradientText
                font.pointSize:         _dataFontSize
                Layout.minimumWidth:    _mediumValueWidth
            }

            Item { width: 1; height: 1 }

            QGCLabel { text: qsTr("Heading:"); font.pointSize: _dataFontSize; }
            QGCLabel {
                text:                   _headingText
                font.pointSize:         _dataFontSize
                Layout.minimumWidth:    _smallValueWidth
            }
        }

        GridLayout {
            columns:                11
            rowSpacing:             _rowSpacing
            columnSpacing:          _labelToValueSpacing
            Layout.alignment:       Qt.AlignVCenter | Qt.AlignHCenter

            QGCLabel {
                text:               qsTr("Total Mission")
                Layout.columnSpan:  11
                font.pointSize:     ScreenTools.smallFontPointSize
            }

            //QGCLabel { text: qsTr("Distance:"); font.pointSize: _dataFontSize; }
            QGCColoredImage {
                width:              _iconWidth
                height:             width
                source:             "/InstrumentValueIcons/travel-walk.svg"
                fillMode:           Image.PreserveAspectFit
                sourceSize.height:  height
                color:              qgcPal.text
            }

            QGCLabel {
                text:                   _missionDistanceText
                font.pointSize:         _dataFontSize
                Layout.minimumWidth:    _largeValueWidth
            }

            Item { width: 1; height: 1 }

            //QGCLabel { text: qsTr("Max telem dist:"); font.pointSize: _dataFontSize; }
            QGCColoredImage {
                width:              _iconWidth
                height:             width
                source:             "/InstrumentValueIcons/station.svg"
                fillMode:           Image.PreserveAspectFit
                sourceSize.height:  height
                color:              qgcPal.text
            }

            QGCLabel {
                text:                   _missionMaxTelemetryText
                font.pointSize:         _dataFontSize
                Layout.minimumWidth:    _largeValueWidth
            }

            Item { width: 1; height: 1 }

            //QGCLabel { text: qsTr("Application Rate:"); font.pointSize: _dataFontSize; }
            QGCColoredImage {
                width:              _iconWidth
                height:             width
                source:             "/InstrumentValueIcons/tuning.svg"
                fillMode:           Image.PreserveAspectFit
                sourceSize.height:  height
                color:              qgcPal.text
            }

            QGCLabel {
                text:                   applicationRate ? applicationRate.toFixed(2) + " l/ha" : "?"
                font.pointSize:         _dataFontSize
                Layout.minimumWidth:    _largeValueWidth
            }

            Item { width: 1; height: 1 }

            //QGCLabel { text: qsTr("Spacing:"); font.pointSize: _dataFontSize; }
            QGCColoredImage {
                width:              _iconWidth
                height:             width
                source:             "/qmlimages/PatternGrid.png"
                fillMode:           Image.PreserveAspectFit
                sourceSize.height:  height
                color:              qgcPal.text
            }

            QGCLabel {
                text:                   getSpacing()
                font.pointSize:         _dataFontSize
                Layout.minimumWidth:    _largeValueWidth
            }

            //QGCLabel { text: qsTr("Time:"); font.pointSize: _dataFontSize; }
            QGCColoredImage {
                width:              _iconWidth
                height:             width
                source:             "/InstrumentValueIcons/timer.svg"
                fillMode:           Image.PreserveAspectFit
                sourceSize.height:  height
                color:              qgcPal.text
            }

            QGCLabel {
                text:                   getMissionTime()
                font.pointSize:         _dataFontSize
                Layout.minimumWidth:    _largeValueWidth
            }

            Item { width: 1; height: 1 }

            //QGCLabel { text: qsTr("Area:"); font.pointSize: _dataFontSize; }
            QGCColoredImage {
                width:              _iconWidth
                height:             width
                source:             "/InstrumentValueIcons/map.svg"
                fillMode:           Image.PreserveAspectFit
                sourceSize.height:  height
                color:              qgcPal.text
            }

            QGCLabel {
                text:                   getArea()
                font.pointSize:         _dataFontSize
                Layout.minimumWidth:    _largeValueWidth
            }

            Item { width: 1; height: 1 }

            //QGCLabel { text: qsTr("Flow Rate:"); font.pointSize: _dataFontSize; }
            QGCColoredImage {
                width:              _iconWidth
                height:             width
                source:             "/InstrumentValueIcons/flowrate.svg"
                fillMode:           Image.PreserveAspectFit
                sourceSize.height:  height
                color:              qgcPal.text
            }

            QGCLabel {
                text:                   flowRate ? flowRate.toFixed(2) + " l/min" : "?"
                font.pointSize:         _dataFontSize
                Layout.minimumWidth:    _largeValueWidth
            }

            Item { width: 1; height: 1 }

            //QGCLabel { text: qsTr("Velocity:"); font.pointSize: _dataFontSize; }
            QGCColoredImage {
                width:              _iconWidth
                height:             width
                source:             "/InstrumentValueIcons/arrow-simple-right.svg"
                fillMode:           Image.PreserveAspectFit
                sourceSize.height:  height
                color:              qgcPal.text
            }

            QGCLabel {
                text:                   velocity ? velocity.toFixed(2) + " m/s" : "?"
                font.pointSize:         _dataFontSize
                Layout.minimumWidth:    _largeValueWidth
            }
        }

        GridLayout {
            columns:                3
            rowSpacing:             _rowSpacing
            columnSpacing:          _labelToValueSpacing
            Layout.alignment:       Qt.AlignVCenter | Qt.AlignHCenter
            visible:                _batteryInfoAvailable

            QGCLabel {
                text:               qsTr("Battery")
                Layout.columnSpan:  3
                font.pointSize:     ScreenTools.smallFontPointSize
            }

            QGCLabel { text: qsTr("Batteries required:"); font.pointSize: _dataFontSize; }
            QGCLabel {
                text:                   _batteriesRequiredText
                font.pointSize:         _dataFontSize
                Layout.minimumWidth:    _mediumValueWidth
            }

            Item { width: 1; height: 1 }
        }

        QGCButton {
            id:          uploadButton
            text:        _controllerDirty ? qsTr("Upload Required") : qsTr("Upload")
            enabled:     !_controllerSyncInProgress
            visible:     _controllerIsSourcePlan && !_controllerOffline && !_controllerSyncInProgress && !uploadCompleteText.visible
            primary:     _controllerDirty
            onClicked:   _planMasterController.upload()

            PropertyAnimation on opacity {
                easing.type:    Easing.OutQuart
                from:           0.5
                to:             1
                loops:          Animation.Infinite
                running:        _controllerDirty && !_controllerSyncInProgress
                alwaysRunToEnd: true
                duration:       2000
            }
        }
    }

    // Small mission download progress bar
    Rectangle {
        id:             progressBar
        anchors.left:   parent.left
        anchors.bottom: parent.bottom
        height:         4
        width:          _controllerProgressPct * parent.width
        color:          qgcPal.colorGreen
        visible:        false

        onVisibleChanged: {
            if (visible) {
                largeProgressBar._userHide = false
            }
        }
    }

    // Large mission download progress bar
    Rectangle {
        id:             largeProgressBar
        anchors.bottom: parent.bottom
        anchors.left:   parent.left
        anchors.right:  parent.right
        height:         parent.height
        color:          qgcPal.window
        visible:        _showLargeProgress

        property bool _userHide:                false
        property bool _showLargeProgress:       progressBar.visible && !_userHide && qgcPal.globalTheme === QGCPalette.Light

        Connections {
            target:                 QGroundControl.multiVehicleManager
            onActiveVehicleChanged: largeProgressBar._userHide = false
        }

        Rectangle {
            anchors.top:    parent.top
            anchors.bottom: parent.bottom
            width:          _controllerProgressPct * parent.width
            color:          qgcPal.colorGreen
        }

        QGCLabel {
            anchors.centerIn:   parent
            text:               qsTr("Syncing Mission")
            font.pointSize:     ScreenTools.largeFontPointSize
        }

        QGCLabel {
            anchors.margins:    _margin
            anchors.right:      parent.right
            anchors.bottom:     parent.bottom
            text:               qsTr("Click anywhere to hide")

            property real _margin: ScreenTools.defaultFontPixelWidth / 2
        }

        MouseArea {
            anchors.fill:   parent
            onClicked:      largeProgressBar._userHide = true
        }
    }
}

