import QtQuick 2.3

import QGroundControl           1.0
import QGroundControl.Controls  1.0
import QGroundControl.Vehicle   1.0

PreFlightCheckButton {
    name:                           qsTr("Battery Cell")
    telemetryFailure:               _highImbalance
    telemetryTextFailure:           qsTr("Cell imbalance is too high")
    allowTelemetryFailureOverride:  allowFailurePercentOverride

    property int    failurePercent:                 80
    property bool   allowFailurePercentOverride:    true
    property var    _batteryGroup:                  globals.activeVehicle && globals.activeVehicle.batteries.count ? globals.activeVehicle.batteries.get(3) : undefined
    property var    _batteryValue:                  _batteryGroup ? _batteryGroup.percentRemaining.value : 0
    property var    _battImbalance:                 isNaN(_batteryValue) ? 0 : _batteryValue
    property bool   _highImbalance:                 _battImbalance < failurePercent
}
