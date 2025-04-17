import QtQuick 2.15
import QtLocation 5.15

MapItemView {
    model: KmlPolygonLoader.polygons

    delegate: MapPolygon {
        path: modelData.coordinates
        color: "lightblue"
        border.color: "blue"
        border.width: 2
        opacity: 1

        Component.onCompleted: {
            console.log("Polygon drawn with", modelData.coordinates.length, "points")
        }
    }
}