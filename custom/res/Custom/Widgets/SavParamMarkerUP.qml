import QtQuick 2.3
import QtLocation 5.3
import QtPositioning 5.3
import QGroundControl 1.0
import QGroundControl.ScreenTools 1.0
import QGroundControl.Controls 1.0


MapItemView {
    id: root

    // Inserisci qui la lista di elementi con proprietà `coordinate` e `label`
    property var modelDataList: []

    model: modelDataList

    delegate: MapQuickItem {
        coordinate: QtPositioning.coordinate(
                        parseFloat(modelData.latitude),
                        parseFloat(modelData.longitude)
                    )
        anchorPoint.x: icon.width / 2
        anchorPoint.y: icon.height
        visible: true

        sourceItem: Image {
            id: icon
            source: "/custom/img/waypoint.svg"
            fillMode: Image.PreserveAspectFit
            width: ScreenTools.defaultFontPixelHeight * 2.5

            QGCColoredImage {
                anchors.fill: parent
                anchors.bottomMargin: 15
                source: modelData.label == "MAR" ? "/res/Sea_WP.svg" : (modelData.label == "SG" ? "/res/SeaSide_WP.svg" : "/res/Camera_WP.svg")
                visible: true
                fillMode: Image.PreserveAspectFit
                color: "white"
            }

            QGCLabel {
                anchors.fill: parent
                anchors.bottomMargin: 15
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                color: "black"
                font.pointSize: ScreenTools.defaultFontPointSize
                fontSizeMode: Text.Fit
                font.bold: true
                text: modelData.label
                visible: modelData.label != "MAR" && modelData.label != "SG"
            }
        }

        z: QGroundControl.zOrderWaypointLines + 1
    }
}
