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
    property real   _smallRadiusRaw:    Math.ceil((ScreenTools.defaultFontPixelHeight * ScreenTools.smallFontPointRatio) / 2)
    property real   _smallRadius:       _smallRadiusRaw + ((_smallRadiusRaw % 2 == 0) ? 1 : 0) + 2 // odd number for better centering
    property real   _normalRadiusRaw:   Math.ceil(ScreenTools.defaultFontPixelHeight * 0.66)
    property real   _normalRadius:      _normalRadiusRaw + ((_normalRadiusRaw % 2 == 0) ? 1 : 0)

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
            width: (modelData.label == "MAR" || modelData.label == "SG") ? _smallRadius * 3: _normalRadius * 3

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
