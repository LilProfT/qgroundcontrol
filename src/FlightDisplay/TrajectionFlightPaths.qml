/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.3
import QtQuick.Controls 1.2
import QtLocation       5.3
import QtPositioning    5.3

import QGroundControl.ScreenTools   1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0


/// Mission item map visual
Item {
    id: _root
    property var    mapControl
    property var    _trajectoryPolylines: []
    property int    _currentIndex:0
    property bool   _trigger:false

    Component.onCompleted: {
        _currentIndex = 0
        addVisuals()
    }

    Component.onDestruction: {
        //removeVisuals()
        //removeDragHandles()
    }

    function addVisuals() {
        var _ctrajectoryPolyline = ctrajectoryPolyline.createObject(mapControl)
        mapControl.addMapItem(_ctrajectoryPolyline)
        _trajectoryPolylines.push(_ctrajectoryPolyline)

        if (_trigger)
            _trajectoryPolylines[_currentIndex].line.color = "limegreen"
        else
            _trajectoryPolylines[_currentIndex].line.color = "white"
    }

    function activeVehicleChanged(listCoord) {
        _trajectoryPolylines[_currentIndex].path = listCoord
        if (_trigger)
            _trajectoryPolylines[_currentIndex].line.color = "limegreen"
        else
            _trajectoryPolylines[_currentIndex].line.color = "white"
    }

    function pointAdded(coord) {
        _trajectoryPolylines[_currentIndex].addCoordinate(coord)
    }

    function pointSprayTrigger(trigger, coord) {
        _trigger = trigger
        _currentIndex = _currentIndex + 1

        addVisuals()
        _trajectoryPolylines[_currentIndex].addCoordinate(coord)
    }

    function updateLastPoint(coord) {
        _trajectoryPolylines[_currentIndex].replaceCoordinate(_trajectoryPolylines[_currentIndex].pathLength() - 1, coord)
    }

    function pointsCleared() {
        for (let i = 0; i < _trajectoryPolylines.length; i++) {
           _trajectoryPolylines[i].path = []
        }
        _trajectoryPolylines = []
        _currentIndex = 0
        _trigger = false
        addVisuals()
    }

    Component {
        id: ctrajectoryPolyline
        MapPolyline {
            line.width: 4.5
            line.color: "white"
            visible:    true
            z: 48
        }
    }
}
