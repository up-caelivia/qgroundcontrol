import QtQuick          2.3
import QtQuick.Controls 1.2
import QtQuick.Layouts  1.2
import QtPositioning    5.2

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0

QGCFlickable {
    id:             root
    contentHeight:  awarenessViewerRect.height
    clip:           true

    property var    polygonGeoAwareness: []
    property var    kmlPolygonLoader: []
    property var    callback: []

    readonly property real  _editFieldWidth:    Math.min(width - _margin * 2, ScreenTools.defaultFontPixelWidth * 15)
    readonly property real  _margin:            ScreenTools.defaultFontPixelWidth / 2
    readonly property real  _radius:            ScreenTools.defaultFontPixelWidth / 2

    Rectangle {
        id:     awarenessViewerRect
        anchors.left:   parent.left
        anchors.right:  parent.right
        height: awarenessItems.y + awarenessItems.height + (_margin * 2)
        radius: _radius
        color:  qgcPal.missionItemEditor

        QGCLabel {
            id:                 geoFenceLabel
            anchors.margins:    _margin
            anchors.left:       parent.left
            anchors.top:        parent.top
            text:               qsTr("Geoawareness Information")
            anchors.leftMargin: ScreenTools.defaultFontPixelWidth
        }

        Rectangle {
            id:                 awarenessItems
            anchors.margins:    _margin
            anchors.left:       parent.left
            anchors.right:      parent.right
            anchors.top:        geoFenceLabel.bottom
            height:             awarenessColumn.y + awarenessColumn.height + (_margin * 2)
            color:              qgcPal.windowShadeDark
            radius:             _radius

            Column {
                id:                 awarenessColumn
                anchors.margins:    _margin
                anchors.top:        parent.top
                anchors.left:       parent.left
                anchors.right:      parent.right
                spacing:            _margin

                Column {
                    anchors.left:       parent.left
                    anchors.right:      parent.right
                    spacing:            _margin
                    visible:            true

                    SectionHeader {
                        id:             nameSection
                        anchors.left:   parent.left
                        anchors.right:  parent.right
                        text:           polygonGeoAwareness.name
                        wrapMode:       Text.Wrap
                    }

                    GridLayout {
                        anchors.left:   parent.left
                        anchors.right:  parent.right
                        columnSpacing:  ScreenTools.defaultFontPixelWidth
                        rowSpacing:     columnSpacing
                        columns:        2
                        visible:        nameSection.checked

                        QGCLabel {
                            text:               qsTr("Lower limit: ")
                            Layout.fillWidth:   true
                            visible:            polygonGeoAwareness.hmin != undefined   
                        }
                        QGCLabel {
                            text:               polygonGeoAwareness.hmin + " m"
                            Layout.fillWidth:   true
                            visible:            polygonGeoAwareness.hmin != undefined 
                        }

                        QGCLabel {
                            text:               qsTr("Higher limit: ")
                            Layout.fillWidth:   true
                            visible:            polygonGeoAwareness.hmax != undefined 
                        }
                        QGCLabel {
                            text:               polygonGeoAwareness.hmax + " m"
                            Layout.fillWidth:   true
                            visible:            polygonGeoAwareness.hmax != undefined 
                        }

                        QGCLabel {
                            text:               qsTr("ID: ")
                            Layout.fillWidth:   true
                            visible:            polygonGeoAwareness.id || polygonGeoAwareness.id != ""
                        }
                        QGCLabel {
                            text:               polygonGeoAwareness.id
                            visible:            polygonGeoAwareness.id || polygonGeoAwareness.id != ""
                        }

                        QGCLabel {
                            text:               qsTr("Color: ")
                            Layout.fillWidth:   true
                        }
                        Rectangle {
                            width: ScreenTools.defaultFontPixelHeight * 1.2
                            height: width
                            color: polygonGeoAwareness.color ? polygonGeoAwareness.color : "red"
                            border.color: "black"
                            radius: 2
                            Layout.alignment: Qt.AlignVCenter
                        }
                    } // GridLayout

                    SectionHeader {
                        id:             timeValiditySection
                        anchors.left:   parent.left
                        anchors.right:  parent.right
                        text:           qsTr( "Time validity")
                        visible:        !isNaN(polygonGeoAwareness.activationDate.getTime()) || !isNaN(polygonGeoAwareness.deactivationDate.getTime())
                    }

                    GridLayout {
                        anchors.left:   parent.left
                        anchors.right:  parent.right
                        columnSpacing:  ScreenTools.defaultFontPixelWidth
                        rowSpacing:     columnSpacing
                        columns:        2
                        visible:        timeValiditySection.checked && timeValiditySection.visible

                        QGCLabel {
                            text:               qsTr( "activation date: ")
                            Layout.fillWidth:   true
                            visible:            !isNaN(polygonGeoAwareness.activationDate.getTime())
                        }
                        QGCLabel {
                            text:               polygonGeoAwareness.activationDate
                            Layout.fillWidth:   true
                            visible:            !isNaN(polygonGeoAwareness.activationDate.getTime())
                        }

                        QGCLabel {
                            text:               qsTr( "deactivation date: ")
                            Layout.fillWidth:   true
                            visible:            !isNaN(polygonGeoAwareness.deactivationDate.getTime())
                        }
                        QGCLabel {
                            text:               polygonGeoAwareness.deactivationDate
                            Layout.fillWidth:   true
                            visible:            !isNaN(polygonGeoAwareness.deactivationDate.getTime())
                        }
                        QGCLabel {
                            text:               polygonGeoAwareness.activationSchedule
                            Layout.fillWidth:   true
                            Layout.columnSpan:  2
                            visible:            (polygonGeoAwareness.activationSchedule.length > 0)
                        }
                    } // GridLayout

                    SectionHeader {
                        id:             descriptionSection
                        anchors.left:   parent.left
                        anchors.right:  parent.right
                        text:           qsTr( "Description")
                    }

                    QGCLabel {
                        id:                 descriptionLabel
                        anchors.left:       parent.left
                        anchors.right:      parent.right
                        wrapMode:           Text.WordWrap
                        text:               polygonGeoAwareness.description
                        visible:            descriptionSection.checked
                    }

                    SectionHeader {
                        id:             actionSection
                        anchors.left:   parent.left
                        anchors.right:  parent.right
                        text:           qsTr( "Actions")
                    }

                    RowLayout {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        spacing: ScreenTools.defaultFontPixelWidth * 0.5

                        QGCButton {
                            Layout.fillWidth: true
                            text: qsTr("Delete")
                            onClicked: {
                                kmlPolygonLoader.removePolygon(polygonGeoAwareness, true)
                            }
                        }

                        QGCButton {
                            Layout.fillWidth: true
                            text: qsTr("Focus")
                            onClicked: {
                                callback.fitPolygonToMap(polygonGeoAwareness.coordinates)
                            }
                        }
                    }
                }
            }
        }
    } // Rectangle
}
