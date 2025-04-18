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

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

KmlPolygonLoader::KmlPolygonLoader(QObject* parent)
    : QObject(parent) {}

KmlPolygonLoader* KmlPolygonLoader::instance() {
    static KmlPolygonLoader* _instance = new KmlPolygonLoader();
    return _instance;
}

bool KmlPolygonLoader::loadFromFile(const QString& filePath) {
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


bool KmlPolygonLoader::loadFromJsonFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Cannot open JSON file:" << filePath;
        return false;
    }

    QByteArray jsonData = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(jsonData);

    if (!doc.isObject()) {
        qWarning() << "Invalid JSON structure.";
        return false;
    }

    const QJsonObject root = doc.object();
    for (const QString& key : root.keys()) {
        const QJsonObject featureCollection = root[key].toObject();
        const QJsonArray features = featureCollection["features"].toArray();

        for (const QJsonValue& f : features) {
            const QJsonObject feature = f.toObject();
            const QJsonObject properties = feature["properties"].toObject();
            const QJsonObject geometry = feature["geometry"].toObject();

            QString name = properties["name"].toString();
            QString id = feature["id"].toVariant().toString();
            QString description = properties["additionalInfo"].toString();
            QColor color = Qt::red; // default or map from category
            int hmin = static_cast<int>(properties["lowestpoint"].toString().toDouble());
            int hmax = static_cast<int>(properties["highestpoint"].toString().toDouble());

            int allowedAltitude = static_cast<int>(properties["upperLimit"].toDouble());

            QDateTime activationDate;  // puoi modificarlo se c'è un campo tipo `validFrom`
            QDateTime deactivationDate; // idem per `validUntil`

            QList<QGeoCoordinate> coordinates;

            const QString type = geometry["type"].toString();
            if (type == "Polygon") {
                const QJsonArray rings = geometry["coordinates"].toArray();
                if (!rings.isEmpty()) {
                    const QJsonArray coordArray = rings[0].toArray();
                    for (const QJsonValue& coordVal : coordArray) {
                        const QJsonArray latlon = coordVal.toArray();
                        if (latlon.size() >= 2) {
                            coordinates.append(QGeoCoordinate(latlon[1].toDouble(), latlon[0].toDouble()));
                        }
                    }
                }
            } else if (type == "MultiPolygon") {
                const QJsonArray polygons = geometry["coordinates"].toArray();
                if (!polygons.isEmpty()) {
                    const QJsonArray firstPolygon = polygons[0].toArray();
                    if (!firstPolygon.isEmpty()) {
                        const QJsonArray coordArray = firstPolygon[0].toArray();
                        for (const QJsonValue& coordVal : coordArray) {
                            const QJsonArray latlon = coordVal.toArray();
                            if (latlon.size() >= 2) {
                                coordinates.append(QGeoCoordinate(latlon[1].toDouble(), latlon[0].toDouble()));
                            }
                        }
                    }
                }
            }

            _polygonObjects.append(new KmlPolygonObject(
                name, allowedAltitude, coordinates, color, hmin, hmax, id, description, activationDate, deactivationDate, this
            ));
        }
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