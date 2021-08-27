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
import QtQuick.Layouts  1.2

import QGroundControl               1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.FactControls  1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controllers   1.0

QGCViewDialog {
    property alias coordinate: controller.coordinate

    property real   _margin:        ScreenTools.defaultFontPixelWidth / 2
    property real   _fieldWidth:    ScreenTools.defaultFontPixelWidth * 10.5

    EditPositionDialogController {
        id: controller

        Component.onCompleted: initValues()
    }

    QGCFlickable {
        anchors.fill:   parent
        contentHeight:  column.height

        Column {
            id:             column
            anchors.left:   parent.left
            anchors.right:  parent.right
            spacing:        ScreenTools.defaultFontPixelHeight

            GridLayout {
                anchors.left:   parent.left
                anchors.right:  parent.right
                columnSpacing:  _margin
                rowSpacing:     _margin
                columns:        2

                QGCLabel {
                    visible: false
                    text: qsTr("Latitude")
                }
                FactTextField {
                    visible: false
                    fact:               controller.latitude
                    Layout.fillWidth:   true
                }

                QGCLabel {
                    visible: false
                    text: qsTr("Longitude")
                }
                FactTextField {
                    visible: false
                    fact:               controller.longitude
                    Layout.fillWidth:   true
                }

                QGCButton {
                    visible: false
                    text:               qsTr("Set Geographic")
                    Layout.alignment:   Qt.AlignRight
                    Layout.columnSpan:  2
                    onClicked: {
                        controller.setFromGeo()
                        reject()
                    }
                }

                Item { width: 1; height: ScreenTools.defaultFontPixelHeight; Layout.columnSpan: 2; visible: false}

                QGCLabel {
                    visible: false
                    text: qsTr("Zone")
                }
                FactTextField {
                    visible: false
                    fact:               controller.zone
                    Layout.fillWidth:   true
                }

                QGCLabel {
                    visible: false
                    text: qsTr("Hemisphere")
                }
                FactComboBox {
                    visible: false
                    fact:               controller.hemisphere
                    indexModel:         false
                    Layout.fillWidth:   true
                }

                QGCLabel {
                    visible: false
                    text: qsTr("Easting")
                }
                FactTextField {
                    visible: false
                    fact:               controller.easting
                    Layout.fillWidth:   true
                }

                QGCLabel {
                    visible: false
                    text: qsTr("Northing")
                }
                FactTextField {
                    visible: false
                    fact:               controller.northing
                    Layout.fillWidth:   true
                }

                QGCButton {
                    text:               qsTr("Set UTM")
                    Layout.alignment:   Qt.AlignRight
                    Layout.columnSpan:  2
                    visible: false
                    onClicked: {
                        controller.setFromUTM()
                        reject()
                    }
                }

                Item { width: 1; height: ScreenTools.defaultFontPixelHeight; Layout.columnSpan: 2; visible: false}

                QGCLabel {
                    text:              qsTr("MGRS")
                    visible: false
                }
                FactTextField {
                    fact:              controller.mgrs
                    Layout.fillWidth:  true
                    visible: false
                }

                QGCButton {
                    text:              qsTr("Set MGRS")
                    Layout.alignment:  Qt.AlignRight
                    Layout.columnSpan: 2
                    visible: false
                    onClicked: {
                        controller.setFromMGRS()
                        reject()
                    }
                }

                Item { width: 1; height: ScreenTools.defaultFontPixelHeight; Layout.columnSpan: 2; visible: false}

                QGCButton {
                    text:              qsTr("Set From Vehicle Position")
                    //visible:           QGroundControl.multiVehicleManager.activeVehicle && QGroundControl.multiVehicleManager.activeVehicle.coordinate.isValid
                    visible: false
                    Layout.alignment:  Qt.AlignRight
                    Layout.columnSpan: 2
                    onClicked: {
                        controller.setFromVehicle()
                        reject()
                    }
                }
                Item { width: 1; height: ScreenTools.defaultFontPixelHeight; Layout.columnSpan: 2}

//                QGCButton {
//                    text:              qsTr("Up")
//                    Layout.alignment:  Qt.AlignCenter
//                    Layout.columnSpan: 2
//                    Layout.fillWidth: true
//                    onClicked: {
//                        controller.moveVertexUp()
//                    }
//                }

                GridLayout {
                    Layout.alignment:  Qt.AlignCenter
                    Layout.columnSpan: 2
                    Layout.fillWidth:  true

                    rowSpacing:     _margin
                    rows: 1

//                    QGCLabel {
//                        text:              qsTr(".")
//                        Layout.fillWidth: true

//                    }

//                    QGCButton {
//                        text:              qsTr("Left")
//                        Layout.fillWidth: true

//                        onClicked: {
//                            controller.moveVertexLeft()
//                        }
//                    }

                    QGCButton {
                        text:              qsTr("Up")
                        Layout.fillWidth: true
                        Layout.maximumWidth : 200

                        onClicked: {
                            controller.moveVertexUp()
                        }
                    }

//                    QGCButton {
//                        text:              qsTr("Right")
//                        Layout.fillWidth: true

//                        onClicked: {
//                            controller.moveVertexRight()
//                        }
//                    }
//                    QGCLabel {
//                        text:              qsTr(".")
//                        Layout.fillWidth: true

//                    }
                }



                GridLayout {
                    Layout.alignment:  Qt.AlignCenter
                    Layout.columnSpan: 2
                    Layout.fillWidth:  true

                    rowSpacing:     _margin
                    rows: 1

                    QGCButton {
                        text:              qsTr("Left")
                        Layout.fillWidth: true

                        onClicked: {
                            controller.moveVertexLeft()
                        }
                    }

                    QGCButton {
                        text:              qsTr("Undo")
                        Layout.fillWidth: true

                        onClicked: {
                            controller.undoMoveVertex()
                        }
                    }

                    QGCButton {
                        text:              qsTr("Right")
                        Layout.fillWidth: true

                        onClicked: {
                            controller.moveVertexRight()
                        }
                    }
                }

                GridLayout {
                    Layout.alignment:  Qt.AlignCenter
                    Layout.columnSpan: 2
                    Layout.fillWidth:  true

                    rowSpacing:     _margin
                    rows: 1

//                    QGCLabel {
//                        text:              qsTr(".")
//                        Layout.fillWidth: true

//                    }

//                    QGCButton {
//                        text:              qsTr("Left")
//                        Layout.fillWidth: true

//                        onClicked: {
//                            controller.moveVertexLeft()
//                        }
//                    }

                    QGCButton {
                        text:              qsTr("Down")
                        Layout.alignment:  Qt.AlignCenter
                        Layout.fillWidth: true
                        Layout.maximumWidth : 200

                        Layout.columnSpan: 2
                        onClicked: {
                            controller.moveVertexDown()
                        }
                    }

//                    QGCButton {
//                        text:              qsTr("Right")
//                        Layout.fillWidth: true

//                        onClicked: {
//                            controller.moveVertexRight()
//                        }
//                    }
//                    QGCLabel {
//                        text:              qsTr(".")
//                        Layout.fillWidth: true

//                    }
                }

                Item { width: 1; height: ScreenTools.defaultFontPixelHeight; Layout.columnSpan: 2}


                QGCButton {
                    text:              qsTr("Reset")
                    Layout.alignment:  Qt.AlignRight
                    Layout.minimumWidth: 30

                    Layout.columnSpan: 2
                    onClicked: {
                        controller.resetMoveVertex()
                    }
                }

            }
        } // Column
    } // QGCFlickable
} // QGCViewDialog
