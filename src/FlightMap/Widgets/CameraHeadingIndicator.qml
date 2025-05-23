import QtQuick
import QGroundControl
import QGroundControl.Controls
import QGroundControl.ScreenTools
import QGroundControl.Vehicle
import QGroundControl.Palette

Canvas {
    id:                 control
    anchors.centerIn:   parent
    width:              compassSize
    height:             width

    property real compassSize
    property real cameraHeading
    property bool simplified:    false
    property real cameraFOV
    property var _qgcPal: QGroundControl.globalPalette

    Connections {
        target:                 _qgcPal
        onGlobalThemeChanged:   control.requestPaint()
    }

    onPaint: {
        var ctx = getContext("2d")
        var cX = width/2
        var cY = height/2
        var radius = width * 0.5
        var startAngle = (cameraHeading - 90 - cameraFOV/2 ) * Math.PI/180 //radian
        var endAngle = (cameraHeading - 90 + cameraFOV/2) * Math.PI/180

        ctx.clearRect(0, 0, width, height) // Clear canvas
        ctx.fillStyle = "#2E90D0" // Light green fill
        ctx.strokeStyle = _qgcPal.text // Theme-aware outline

        ctx.beginPath()
        ctx.lineWidth = 2
        ctx.arc(cX, cY, radius, startAngle, endAngle, false) // Draw camera FOV arc
        ctx.lineTo(cX, cY) // Connect to center for filled sector
        ctx.closePath()
        ctx.fill()
        ctx.stroke()
    }

    transform: Rotation {
        origin.x:   control.width / 2
        origin.y:   control.height / 2
        angle:     cameraHeading
    }
}
