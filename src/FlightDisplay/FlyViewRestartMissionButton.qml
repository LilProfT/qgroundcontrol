/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlightDisplay
import QGroundControl.Controllers

ToolStripAction {
    id:         action
    text:       qsTr("Restart Mission")
    iconSource: "/res/counter-clockwise-arrow.svg"
    visible:    true
    enabled:    true
    onTriggered:{
        QGroundControl.multiVehicleManager.activeVehicle.restartMission();
    }
}
