import QGroundControl                   1.0
import QGroundControl.Controls          1.0

PlanEditToolbar {
    property var    mapControl
    property var    missionController
    property bool   addWaypointOnClick

    visible:                        missionController.isTreeSprayingMission

    anchors.left:                   mapControl.left
    anchors.leftMargin:             mapControl.centerViewport.left
    y:                              mapControl.centerViewport.top
    availableWidth:                 mapControl.centerViewport.width

    z:                              QGroundControl.zOrderMapItems + 2
    vertical:                       addWaypointOnClick

    function insertTreeSprayingItemsAfterCurrent(coordinate) {
        var nextIndex = missionController.currentPlanViewVIIndex + 1
        missionController.insertTreeSprayingMissionItems(coordinate, nextIndex, true /* makeCurrentItem */)
    }

    QGCButton {
        _horizontalPadding: 0
        text:               qsTr("Set from Vehicle")
        visible:            addWaypointOnClick && QGroundControl.multiVehicleManager.activeVehicle && QGroundControl.multiVehicleManager.activeVehicle.coordinate.isValid
        onClicked:          insertTreeSprayingItemsAfterCurrent(QGroundControl.multiVehicleManager.activeVehicle.coordinate)
    }

    QGCButton {
        _horizontalPadding: 0
        text:               qsTr("Set from GCS")
        visible:            addWaypointOnClick
        onClicked:          insertTreeSprayingItemsAfterCurrent(QGroundControl.qgcPositionManger.gcsPosition)
    }

    QGCButton {
        id: upBtn
        _horizontalPadding: 0
        text:               qsTr("ᐱ")
        onClicked: {
            missionController.setCurrentPlanViewSeqNum(missionController.currentPlanViewSeqNum-1, false)
        }
    }

    QGCButton {
        id: downBtn
        _horizontalPadding: 0
        text:               qsTr("ᐯ")
        onClicked: {
            missionController.setCurrentPlanViewSeqNum(missionController.currentPlanViewSeqNum+1, false)
        }
    }
}
