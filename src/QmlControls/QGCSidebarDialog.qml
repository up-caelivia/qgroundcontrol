import QtQuick                      2.12
import QtQuick.Controls             2.4
import QtQuick.Layouts              1.12
import QtQuick.Dialogs              1.3
import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0

Item {
    id: root
    width: Math.min(parent.width * 0.5, 500)
    height: parent.height
    x: visible ? 0 : -width
    visible: false

    Behavior on x {
        NumberAnimation { duration: 200; easing.type: Easing.InOutQuad }
    }

    property string title: "Settings"
    property alias contentItem: contentLoader.sourceComponent

    function open() {
        visible = true
    }

    function close() {
        visible = false
    }

    Rectangle {
        anchors.fill: parent
        color: _pal.window

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            // Header with title and close button
            Rectangle {
                Layout.fillWidth: true
                height: Math.max(titleLabel.height, closeButton.height) + _contentMargin * 2
                color: _pal.windowShade

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: _contentMargin
                    spacing: _contentMargin

                    QGCLabel {
                        id: titleLabel
                        Layout.fillWidth: true
                        text: root.title
                        font.pointSize: ScreenTools.mediumFontPointSize
                    }

                    QGCButton {
                        id: closeButton
                        text: qsTr("Close")
                        onClicked: root.close()
                    }
                }
            }

            // Content area
            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                Layout.margins: _contentMargin


                Loader {
                    id: contentLoader
                    width: parent.width
                }
            }
        }
    }

    QGCPalette { id: _pal; colorGroupEnabled: root.enabled }
    property real _contentMargin: ScreenTools.defaultFontPixelHeight / 2
}
