import QGroundControl           1.0
import QGroundControl.Controls  1.0
import QGroundControl.FlightDisplay 1.0

ToolStripAction {
    text:       qsTr("Resume")
    iconSource: "/qmlimages/MapSync.svg"
    //Should only be visble and enable after landing, before completing the designated mission
    visible:    _activeVehicle && !_activeVehicle.armed
    enabled:    _activeVehicle && !_activeVehicle.armed && globals.guidedControllerFlyView.showResumeMission

    onTriggered: {
        globals.guidedControllerFlyView.executeAction(globals.guidedControllerFlyView.actionResumeMission, null, null)
        hideDialog()
    }
}
