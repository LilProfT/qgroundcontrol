/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick              2.3
import QtQuick.Controls     2.4
import QtQuick.Layouts      1.2

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0

/// Toolbar used for things like Polygon editing tools
Item {
    property bool vertical: false

    width:  Math.min(toolsGridLayout.width + (_margins * 2), availableWidth)
    height: toolsFlickable.y + toolsFlickable.height + _margins
    z:      QGroundControl.zOrderMapItems + 2

    property real availableWidth

    property real _radius:  ScreenTools.defaultFontPixelWidth / 2
    property real _margins: ScreenTools.defaultFontPixelWidth / 2

    Component.onCompleted: {
        // Move the child controls from consumer into the layout control
        var moveList = []
        var i
        for (i = 2; i < children.length; i++) {
            moveList.push(children[i])
        }
        for (i = 0; i < moveList.length; i++) {
            moveList[i].parent = toolsGridLayout
        }
        instructionComponent.createObject(toolsRowLayout)
    }

    Rectangle {
        anchors.fill:    parent
        radius:         _radius
        color:          QGroundControl.globalPalette.toolbarBackground
    }

    QGCFlickable {
        id:                 toolsFlickable
        anchors.margins:    _margins
        anchors.top:        parent.top
        anchors.left:       parent.left
        anchors.right:      parent.right
        height:             toolsGridLayout.height
        clip:               true
        flickableDirection: vertical ? Flickable.VerticalFlick : Flickable.HorizontalFlick
        contentWidth:       toolsGridLayout.width

        GridLayout {
            id:                 toolsGridLayout
            rowSpacing:         _margins
            columnSpacing:      _margins
            rows:               vertical ? undefined : 1
            columns:            vertical ? 1 : undefined
            flow:               vertical ? GridLayout.TopToBottom : GridLayout.LeftToRight
        }
    }

    Component {
        id: instructionComponent

        QGCLabel {
            id:             instructionLabel
            text:           _instructionText
            visible:        false
        }
    }
}
