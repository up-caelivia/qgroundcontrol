#include "KmlPolygonLoader.h"
#include "KmlPolygonObject.h"

#include <QFile>
#include <QDomDocument>
#include <QDebug>

KmlPolygonLoader::KmlPolygonLoader(QObject* parent)
    : QObject(parent) {}

bool KmlPolygonLoader::loadFromFile(const QString& filePath) {
    _polygonObjects.clear();

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Cannot open KML file:" << filePath;
        return false;
    }

    QDomDocument doc;
    if (!doc.setContent(&file)) {
        qWarning() << "Failed to parse KML.";
        return false;
    }

    QDomNodeList placemarks = doc.elementsByTagName("Placemark");
    for (int i = 0; i < placemarks.count(); ++i) {
        parsePlacemark(placemarks.at(i).toElement());
    }

    emit polygonsChanged();
    return true;
}

void KmlPolygonLoader::parsePlacemark(const QDomElement& placemark) {
    QString name = placemark.attribute("id");
    int allowedAltitude = -1;
    QList<QGeoCoordinate> coordinates;

    QDomElement polygon = placemark.firstChildElement("Polygon");
    if (!polygon.isNull()) {
        QDomElement extrudeElement = polygon.firstChildElement("extrude");
        if (!extrudeElement.isNull()) {
            allowedAltitude = extrudeElement.text().toInt();
        }

        QDomElement outerBoundary = polygon.firstChildElement("outerBoundaryIs");
        QDomElement linearRing = outerBoundary.firstChildElement("LinearRing");
        QDomElement coordElement = linearRing.firstChildElement("coordinates");

        QStringList coordPairs = coordElement.text().trimmed().split(' ', Qt::SkipEmptyParts);

        for (const QString& pair : coordPairs) {
            QStringList latlon = pair.split(',', Qt::SkipEmptyParts);

            if (latlon.size() >= 2) {
                coordinates.append(QGeoCoordinate(latlon[1].toDouble(), latlon[0].toDouble()));
            }
        }
    }

    _polygonObjects.append(new KmlPolygonObject(name, allowedAltitude, coordinates, this));
}

QList<QObject*> KmlPolygonLoader::polygons() const {
    return _polygonObjects;
}
