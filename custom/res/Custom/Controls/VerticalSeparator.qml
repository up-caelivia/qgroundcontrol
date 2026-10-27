import QtQuick 2.12
import QtQuick.Layouts 1.11

Item {
    id: root

    property real referenceHeight: 0
    property real separatorSpacing: 0

    implicitWidth: 1 + separatorSpacing * 2
    implicitHeight: referenceHeight * 0.6

    Layout.alignment: Qt.AlignVCenter

    Rectangle {
        anchors.centerIn: parent
        width: 1
        height: parent.height
        color: Qt.rgba(
            qgcPal.text.r,
            qgcPal.text.g,
            qgcPal.text.b,
            0.3
        )
    }
}
