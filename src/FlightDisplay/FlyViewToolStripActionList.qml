/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQml.Models

import QGroundControl
import QGroundControl.Controls

ToolStripActionList {
    id: _root

    signal displayPreFlightChecklist

    model: [
        ToolStripAction {
            property bool _is3DViewOpen:            viewer3DWindow.isOpen
            property bool   _viewer3DEnabled:       QGroundControl.settingsManager.viewer3DSettings.enabled.rawValue

            id: view3DIcon
            visible: _viewer3DEnabled
            text:           qsTr("3D View")
            iconSource:     "/qmlimages/Viewer3D/City3DMapIcon.svg"
            onTriggered:{
                if(_is3DViewOpen === false){
                    viewer3DWindow.open()
                }else{
                    viewer3DWindow.close()
                }
            }

            on_Is3DViewOpenChanged: {
                if(_is3DViewOpen === true){
                    view3DIcon.iconSource =     "/qmlimages/PaperPlane.svg"
                    text=           qsTr("Fly")
                }else{
                    iconSource =     "/qmlimages/Viewer3D/City3DMapIcon.svg"
                    text =           qsTr("3D View")
                }
            }
        },
        PreFlightCheckListShowAction { onTriggered: displayPreFlightChecklist() },
        GuidedActionTakeoff { },
        GuidedActionLand { },
        GuidedActionRTL { },
        GuidedActionPause { },
        FlyViewAdditionalActionsButton { },
        GuidedActionGripper { },
        FlyViewRestartMissionButton{ },

        ToolStripAction {
            property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
            id:         calibPTZCameraIcon
            text:       qsTr("Calib Camera")
            iconSource: "/qmlimages/camera_photo.svg"
            visible:    true
            enabled:    true
            onTriggered:{
                _activeVehicle.setZeroCameraAngle(_activeVehicle.vcu.cameraDeltaYaw.rawValue)
            }
        },
        ToolStripAction {
            id:         customButton
            text:       qsTr("Custom")
            iconSource: "/qmlimages/Gears.svg"
            visible:    true
            enabled:    true
            onTriggered:{

            }
        }
    ]
}
