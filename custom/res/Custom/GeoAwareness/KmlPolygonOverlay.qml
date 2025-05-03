import QtQuick 2.15
import QtLocation 5.15

MapItemView {
    id: polygonOverlay

    // Il modello da usare (può essere esterno o di default)
    property var polygonModel: []

    model: polygonModel

    delegate: MapPolygon {
        path: modelData.coordinates
        color: "lightblue"
        border.color: "red"
        border.width: 2
        opacity: 1
        z: 100

        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            onClicked: {
                console.log("Name ", modelData.name)
                console.log("Polygon drawn with", modelData.coordinates.length, "points")
                console.log("Description ", modelData.description)
                console.log("hmin ", modelData.hmin)
                console.log("hmax ", modelData.hmax)
                console.log("activationDate ", modelData.activationDate)
                console.log("deactivationDate ", modelData.deactivationDate)
            }
        }
    }
}   