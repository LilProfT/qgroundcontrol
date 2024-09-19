import QGroundControl           1.0
import QGroundControl.Controls  1.0
import QGroundControl.FlightDisplay 1.0

ToolStripAction {
    text:       qsTr("Spray")
    iconSource: "/qmlimages/Flowrate.svg"
    //Should only be visble and enable after landing, before completing the designated mission
    visible:    _activeVehicle && !_activeVehicle.armed
    enabled:    _activeVehicle && !_activeVehicle.armed
    //enabled: true
}
