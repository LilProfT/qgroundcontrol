import QtQuick                      2.11
import QtQuick.Controls             2.4
import QtQuick.Controls.Styles      1.4
import QtQuick.Layouts              1.11

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Vehicle       1.0
import QGroundControl.Controls      1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Palette       1.0

// Editor for Simple mission items
Rectangle {
    width:  availableWidth
    height: editorColumn.height + (_margin * 2)
    color:  qgcPal.windowShadeDark
    radius: _radius

    // property bool _specifiesAltitude:       missionItem.specifiesAltitude
    property real _margin:                  ScreenTools.defaultFontPixelHeight / 2
    property real _altRectMargin:           ScreenTools.defaultFontPixelWidth / 2
    property var  _controllerVehicle:       missionItem.masterController.controllerVehicle
    property bool _supportsTerrainFrame:    _controllerVehicle.supportsTerrainFrame
    property int  _globalAltMode:           missionItem.masterController.missionController.globalAltitudeMode
    property bool _globalAltModeIsMixed:    _globalAltMode == QGroundControl.AltitudeModeMixed
    property real _radius:                  ScreenTools.defaultFontPixelWidth / 2

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    Column {
        id:                 editorColumn
        anchors.margins:    _margin
        anchors.left:       parent.left
        anchors.right:      parent.right
        anchors.top:        parent.top
        spacing:            _margin

        Column {
            anchors.left:       parent.left
            anchors.right:      parent.right
            spacing:            _altRectMargin
            visible:            !missionItem.wizardMode

            GridLayout {
                anchors.left:   parent.left
                anchors.right:  parent.right
                rows:           4
                columns:        2

                QGCLabel { text:  missionItem.volume.name }

                FactTextField {
                    showUnits:          true
                    fact:               missionItem.volume
                    Layout.fillWidth:   true
                }

                QGCLabel { text:  missionItem.flowRate.name }

                FactTextField {
                    showUnits:          true
                    fact:               missionItem.flowRate
                    Layout.fillWidth:   true
                }

                QGCLabel { text: qsTr("Spray Time") }

                QGCLabel {
                    text:               missionItem.sprayTime + " s"
                    font.pointSize:     ScreenTools.largeFontPointSize
                    color:              qgcPal.globalTheme !== QGCPalette.Light ? "deepskyblue" : "forestgreen"
                    Layout.alignment:   Qt.AlignHCenter
                }

                QGCButton {
                    text:               qsTr("Reset Default")
                    Layout.fillWidth:   true
                    Layout.columnSpan:  2
                    onClicked:          missionItem.resetDefault();
                }
            }
        }
    }
}
