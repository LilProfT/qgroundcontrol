/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick                  2.3
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.4
import QtQuick.Controls.Private 1.0

import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0
import QtQuick.Layouts  1.2

Slider {
    id:             _root
    implicitHeight: !ScreenTools.isMobile ? ScreenTools.implicitSliderHeight : ScreenTools.implicitSliderHeight / 2

    // Value indicator starts display from zero instead of min value
    property bool zeroCentered: false
    property bool displayValue: false
    property int _margin : Math.round(ScreenTools.defaultFontPixelWidth *  (ScreenTools.isMobile ? 1.2 : 1.1))
    property int _sizeButtonWidth : Math.round(ScreenTools.defaultFontPixelWidth *  (ScreenTools.isMobile ? 3.5 : 3.0))
    property real _sizeButtonHeight:             Math.round(ScreenTools.defaultFontPixelHeight * (ScreenTools.isMobile ? 1.4 : 1.2))

    style: SliderStyle {
        groove: Item {
            anchors.verticalCenter: parent.verticalCenter
            implicitWidth:          Math.round(ScreenTools.defaultFontPixelHeight * 4.5)
            implicitHeight:         Math.round(ScreenTools.defaultFontPixelHeight * 0.3)

            Rectangle {
                id:     slidebar
                radius:         height / 2
                anchors.fill:   parent
                color:          qgcPal.button
                border.width:   1
                border.color:   qgcPal.buttonText
                anchors.leftMargin: _margin
                anchors.rightMargin: _margin
            }

            Timer {
                id: updateTimerMinus
                interval: 1
                running: false
                repeat: true
                onTriggered: {
                    // only update if scroll bar is at the bottom
                    _root.value = _root.value - _root.stepSize
                }
            }

            Timer {
                id: updateTimerPlus
                interval: 1
                running: false
                repeat: true
                onTriggered: {
                    // only update if scroll bar is at the bottom
                    _root.value = _root.value + _root.stepSize
                }
            }

            QGCButton {
                anchors.right: parent.left
                anchors.verticalCenter: parent.verticalCenter
                width: _sizeButtonWidth
                height: _sizeButtonHeight
                text: qsTr("-")
                onClicked: {
                    _root.value = _root.value - _root.stepSize
                }
                onPressAndHold: updateTimerMinus.start()
                onReleased: updateTimerMinus.stop()
            }

            Item {
                id:     indicatorBar
                clip:   true
                x:      _root.zeroCentered ? zeroCenteredIndicatorStart : 0
                width:  _root.zeroCentered ? centerIndicatorWidth : styleData.handlePosition
                height: parent.height
                anchors.leftMargin: _margin
                anchors.rightMargin: _margin

                property real zeroValuePosition:            (Math.abs(control.minimumValue) / (control.maximumValue - control.minimumValue)) * parent.width
                property real zeroCenteredIndicatorStart:   Math.min(styleData.handlePosition, zeroValuePosition)
                property real zeroCenteredIndicatorStop:    Math.max(styleData.handlePosition, zeroValuePosition)
                property real centerIndicatorWidth:         zeroCenteredIndicatorStop - zeroCenteredIndicatorStart

                Rectangle {
                    anchors.fill:   parent
                    anchors.leftMargin: _margin
                    anchors.rightMargin: _margin
                    color:          qgcPal.colorBlue
                    border.color:   Qt.darker(color, 1.2)
                    radius:         height/2
                }
            }

            QGCButton {
                anchors.left: parent.right
                anchors.verticalCenter: parent.verticalCenter
                width: _sizeButtonWidth
                height: _sizeButtonHeight
                text: qsTr("+")
                onClicked: {
                    _root.value = _root.value + _root.stepSize
                }
                onPressAndHold: updateTimerPlus.start()
                onReleased: updateTimerPlus.stop()
            }
        }

        handle: Rectangle {
            anchors.centerIn: parent
            color:          qgcPal.button
            border.color:   qgcPal.buttonText
            border.width:   1
            implicitWidth:  _radius * 2
            implicitHeight: _radius * 2
            radius:         _radius
            anchors.leftMargin: _margin
            anchors.rightMargin: _margin

            property real _radius: Math.round(_root.implicitHeight / 2)

            Label {
                text:               _root.value.toFixed( _root.maximumValue <= 1 ? 1 : 0)
                visible:            _root.displayValue
                anchors.centerIn:   parent
                font.family:        ScreenTools.normalFontFamily
                font.pointSize:     ScreenTools.smallFontPointSize
                color:              qgcPal.buttonText
                anchors.leftMargin: _margin
                anchors.rightMargin: _margin
            }
        }
    }
}
