import QtQuick 2.3

import QGroundControl           1.0
import QGroundControl.Controls  1.0
import QGroundControl.Vehicle   1.0

PreFlightCheckButton {
    name:                           qsTr("Battery Cell")
    telemetryFailure:               _highImbalance
    telemetryTextFailure:           qsTr("Cell imbalance is too high")
    allowTelemetryFailureOverride:  allowFailurePercentOverride

    property var    failureDeltaVoltage:            0.08
    property bool   allowFailurePercentOverride:    true
    property var    _batteryGroup:                  globals.activeVehicle && globals.activeVehicle.batteries.count ? globals.activeVehicle.batteries.get(0) : undefined
    property var    _batteryVoltMax:                _batteryGroup ? _batteryGroup.cellVoltageMax.value : 0
    property var    _batteryVoltMin:                _batteryGroup ? _batteryGroup.cellVoltageMin.value : 0
    property var    _cellDeltaVoltage:              (isNaN(_batteryVoltMax) || isNaN(_batteryVoltMin)) ? 0 : (_batteryVoltMax - _batteryVoltMin)
    property bool   _highImbalance:                 _cellDeltaVoltage > failureDeltaVoltage
}
