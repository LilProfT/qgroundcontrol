/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.12
import QtQuick.Layouts  1.12

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FlightMap     1.0
import QGroundControl.FlightDisplay 1.0
import QGroundControl.Palette       1.0

ColumnLayout {
    id:         root
    spacing:    ScreenTools.defaultFontPixelHeight / 4

    property real   _innerRadius:           (width - (_topBottomMargin * 3)) / 4
    property real   _outerRadius:           _innerRadius + _topBottomMargin
    property real   _spacing:               ScreenTools.defaultFontPixelHeight * 0.33
    property real   _topBottomMargin:       (width * 0.05) / 2
    property bool   _showAttitude:           true
    property real   _scale:                 4
    property bool   _show:                  true

    QGCPalette { id: qgcPal }

    Item {
        Layout.fillWidth:   true
        Layout.alignment:   Qt.AlignHCenter

        Rectangle {
            id:                 visualInstrument
            height:             _outerRadius * 2
            width:              _showAttitude? _outerRadius * 4 : _outerRadius * 2
            radius:             _outerRadius
            anchors.right:      parent.right
            color:              qgcPal.window
            visible:            _show

            MouseArea {
                anchors.fill:   parent
                onClicked:      _show = false
            }

            QGCAttitudeWidget {
                id:                     attitude
                anchors.leftMargin:    _topBottomMargin
                anchors.left:           parent.left
                size:                   _innerRadius * 2
                vehicle:                globals.activeVehicle
                anchors.verticalCenter: parent.verticalCenter
                visible:                _showAttitude
            }

            QGCCompassWidget {
                id:                     compass
                //anchors.leftMargin:    _spacing
                //anchors.left:           attitude.right
                anchors.right:          parent.right
                anchors.rightMargin:    _topBottomMargin
                size:                   _innerRadius * 2
                vehicle:                globals.activeVehicle
                anchors.verticalCenter: parent.verticalCenter
            }
        }
        Rectangle {
            id:                     compassVisual
            height:                 _outerRadius / 2
            width:                  height
            radius:                 height / 2
            anchors.right:          visualInstrument.left
            anchors.verticalCenter: visualInstrument.verticalCenter
            anchors.rightMargin:    _spacing
            color:                  qgcPal.window
            visible:                _show

            QGCColoredImage {
                anchors.margins:    parent.height / 8
                anchors.fill:       parent
                source:             "/res/compass.svg"
                fillMode:           Image.PreserveAspectFit
                smooth:             true
                color:              qgcPal.text
            }

            MouseArea {
                anchors.fill:   parent
                onClicked:      _showAttitude? _showAttitude = false : _showAttitude = true
            }
        }

        Rectangle {
            id:                 icon
            height:             visualInstrument.height / _scale
            width:              visualInstrument.width  / _scale
            radius:             visualInstrument.radius / _scale
            anchors.right:      parent.right
            color:              qgcPal.window
            visible:            !_show

            Image {
                source:             "/qmlimages/compassSmall.png"
                smooth:             true
                mipmap:             true
                antialiasing:       true
                fillMode:           Image.PreserveAspectFit
                anchors.fill:       parent
            }
            MouseArea {
                anchors.fill:   parent
                onClicked:      _show = true
            }
        }
    }
}
