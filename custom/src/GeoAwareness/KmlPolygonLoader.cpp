#include "KmlPolygonLoader.h"
#include "KmlPolygonObject.h"

#include "QGCApplication.h"
#include "QGCToolbox.h"
#include "GeoFenceController.h"
#include "QGCMapPolygon.h"
#include "GeoAwarenessFencePolygon.h"

#include <QFile>
#include <QDomDocument>
#include <QDebug>

KmlPolygonLoader::KmlPolygonLoader(QObject* parent)
    : QObject(parent) {}

KmlPolygonLoader* KmlPolygonLoader::instance() {
    static KmlPolygonLoader* _instance = new KmlPolygonLoader();
    return _instance;
}

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

void KmlPolygonLoader::applyToGeoFence(QObject* controllerObj) {
    GeoFenceController* controller = qobject_cast<GeoFenceController*>(controllerObj);
    if (!controller) {
        qWarning() << "Invalid GeoFenceController passed to applyToGeoFence";
        return;
    }

    if (_polygonObjects.isEmpty()) {
        qWarning() << "No polygons to apply";
        return;
    }

    QmlObjectListModel* polygonList = controller->polygons();
    if (!polygonList) {
        qWarning() << "GeoFenceController has no valid polygon list.";
        return;
    }

    // Pulisci i poligoni esistenti se vuoi sovrascrivere
    polygonList->clear();

    for (QObject* obj : _polygonObjects) {
        KmlPolygonObject* polyObj = qobject_cast<KmlPolygonObject*>(obj);
        if (!polyObj) continue;
    
        GeoAwarenessFencePolygon* newPolygon = new GeoAwarenessFencePolygon(false, this);
        newPolygon->setIsKml(true);
        newPolygon->setPath(polyObj->coordinates());
        polygonList->append(newPolygon);
    }

    qDebug() << "Added" << polygonList->count() << "polygon(s) to GeoFenceController";
}

QList<QObject*> KmlPolygonLoader::polygons() const {
    return _polygonObjects;
}

void KmlPolygonLoader::clearPolygons() {
    qDeleteAll(_polygonObjects);
    _polygonObjects.clear();
    emit polygonsChanged();
}

void KmlPolygonLoader::removePolygon(QObject* polygon) {
    int index = _polygonObjects.indexOf(polygon);
    if (index >= 0) {
        _polygonObjects.removeAt(index);
        emit polygonsChanged(); // se è una Q_PROPERTY
    }
}