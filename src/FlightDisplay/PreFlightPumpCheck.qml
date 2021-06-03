import QtQuick 2.3

import QGroundControl           1.0
import QGroundControl.Controls  1.0
import QGroundControl.Vehicle   1.0

PreFlightCheckButton {
    name:                           qsTr("Nozzle")
    telemetryFailure:               _flowcheck
    telemetryTextFailure:           qsTr("Either tip is blocked or not checked, skip when surveying")
    allowTelemetryFailureOverride:  allowFailurePercentOverride

    property int    failurePercent:                 5
    property bool   allowFailurePercentOverride:    true
    property var    _batteryGroup:                  globals.activeVehicle && globals.activeVehicle.batteries.count ? globals.activeVehicle.batteries.get(2) : undefined
    property var    _flowValue:                     _batteryGroup ? _batteryGroup.current.value : 0
    property int    _flowCurrentMax:                0
    property var    _flowCurrent:                   isNaN(_flowValue) ? 0 : _flowValue

    on_FlowCurrentChanged: compareflow()

    property bool   _flowcheck:                     _flowCurrentMax < failurePercent

    function compareflow(){
        if (_flowCurrent > _flowCurrentMax) {
            _flowCurrentMax = _flowCurrent
        }
    }
}

