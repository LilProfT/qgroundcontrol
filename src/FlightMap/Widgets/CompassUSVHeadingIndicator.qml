/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick

import QGroundControl
import QGroundControl.Controls
import QGroundControl.ScreenTools
import QGroundControl.Vehicle
import QGroundControl.Palette

Canvas {
    id:                 control
    anchors.centerIn:   parent
    width:              compassSize * 0.4
    height:             width

    property real compassSize
    property real heading
    property bool simplified:    false

    property var _qgcPal: QGroundControl.globalPalette

    Connections {
        target:                 _qgcPal
        onGlobalThemeChanged:   control.requestPaint()
    }

    onPaint: {
        var ctx = getContext("2d")
        var cX = width/2
        var cY = height/2
        context.clearRect(0, 0, width, height);

        ctx.strokeStyle = simplified ? "#104781" : _qgcPal.text
        ctx.fillStyle = "#104781"
        ctx.lineWidth = 1

        //Boat shape
        ctx.beginPath()
        ctx.moveTo(cX - 0.01*width , 0)
        ctx.lineTo(cX - 0.117*width, height*0.318)
        ctx.lineTo(cX - 0.153*width, height*0.54)
        ctx.lineTo(cX - 0.165*width, height*0.704)
        ctx.lineTo(cX - 0.165*width, height*0.948)
        ctx.lineTo(cX - 0.11*width, height)
        ctx.lineTo(cX + 0.11*width, height)
        ctx.lineTo(cX + 0.165*width, height*0.948)
        ctx.lineTo(cX + 0.165*width, height*0.704)
        ctx.lineTo(cX + 0.153*width, height*0.54)
        ctx.lineTo(cX + 0.117*width, height*0.318)
        ctx.lineTo(cX +0.01*width , 0)
        ctx.closePath()
        ctx.fill()
        ctx.stroke()

        //Bottom rectangle (Antenna)
        ctx.fillStyle = "#D9D8E6"
        ctx.beginPath()
        ctx.moveTo(cX - 0.1*width ,height*0.7)
        ctx.lineTo(cX - 0.1*width, height*0.89)
        ctx.lineTo(cX + 0.1*width, height*0.89)
        ctx.lineTo(cX + 0.1*width, height*0.7)
        ctx.closePath()
        ctx.fill()
        ctx.stroke()

        //Top arc (Camera)
        ctx.beginPath()
        ctx.arc(cX, height*0.3, width*0.06, 0, 2*Math.PI,false)
        ctx.fill()
        ctx.stroke()
    }

    transform: Rotation {
        origin.x:   control.width / 2
        origin.y:   control.height / 2
        angle:      heading
    }
}
