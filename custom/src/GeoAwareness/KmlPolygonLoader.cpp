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
                name, coordinates, color, hmin, hmax, id, description, activationDate, deactivationDate, this
            ));
        }
    }

    emit polygonsChanged();
    return true;
}

void KmlPolygonLoader::parsePlacemark(const QDomElement& placemark) {
    QString name = placemark.firstChildElement("name").text().trimmed();
    QString id, description, message="-", restriction, reason="-", service="-", authority="-", email, contact="-";
    QDateTime activationDate, deactivationDate;
    int hmin = 0;
    int hmax = 9999;
    QColor color = Qt::red;
    QList<QGeoCoordinate> coordinates;
    QList<QList<QGeoCoordinate>> holes;

    // Parse <ExtendedData>
    QDomElement extData = placemark.firstChildElement("ExtendedData");
    if (!extData.isNull()) {
        QDomNodeList simpleDataList = extData.elementsByTagName("SimpleData");
        for (int i = 0; i < simpleDataList.count(); ++i) {
            QDomElement dataElem = simpleDataList.at(i).toElement();
            QString key = dataElem.attribute("name");
            QString value = dataElem.text().trimmed();

            if (key == "Identifier") id = value;
            else if (key == "Message_en") message = value;
            else if (key == "Reason") reason = value;
            else if (key == "Restri_en") restriction = value;
            else if (key == "Contact") contact = value;
            else if (key == "Email")  email = value;
            else if (key == "Service_en") service = value;
            else if (key == "Authori_en") authority = value;
            else if (key == "StartDate") activationDate = QDateTime::fromString(value, Qt::ISODate);
            else if (key == "EndDate") deactivationDate = QDateTime::fromString(value, Qt::ISODate);
            else if (key == "LowerLimit") hmin = value.toInt();
            else if (key == "UpperLimit") hmax = value.toInt();
            else if (key == "Name_en") name = value.trimmed();
        }
        description = message + "\n" + restriction + "\n\nREASON:\n" + reason + "\n\nSERVICE:\n" + service + "\n\nAUTHORITY:\n" + authority + "\n\nCONTACT:\n" + contact + "\n" + email;
    }

    QDomElement polygon = placemark.firstChildElement("Polygon");
    if (!polygon.isNull()) {
        // Outer boundary
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

        // Inner boundaries (holes)
        QDomNodeList innerBoundaries = polygon.elementsByTagName("innerBoundaryIs");
        for (int i = 0; i < innerBoundaries.count(); ++i) {
            QDomElement ring = innerBoundaries.at(i).firstChildElement("LinearRing");
            QDomElement coordsElem = ring.firstChildElement("coordinates");
            QStringList holeCoords = coordsElem.text().trimmed().split(' ', Qt::SkipEmptyParts);

            QList<QGeoCoordinate> hole;
            for (const QString& pair : holeCoords) {
                QStringList latlon = pair.split(',', Qt::SkipEmptyParts);
                if (latlon.size() >= 2) {
                    hole.append(QGeoCoordinate(latlon[1].toDouble(), latlon[0].toDouble()));
                }
            }
            holes.append(hole);
        }
    }

    if (hmin >=120) {
        color = Qt::green;
    } else if (hmin >=120) {
        color = QColor("orange");
    } else {
        color = Qt::red;
    }

    // Create and store the polygon object
    _polygonObjects.append(new KmlPolygonObject(
        name,
        coordinates,
        color,  
        hmin,
        hmax,
        id,
        description,
        activationDate,
        deactivationDate,
        this
        // Add holes as needed in your KmlPolygonObject class
    ));
}



QList<QObject*> KmlPolygonLoader::polygons() const {
    return _polygonObjects;
}

void KmlPolygonLoader::clearPolygons() {
    qDeleteAll(_polygonObjects);
    _polygonObjects.clear();
    _selectedPolygon = nullptr;
    emit polygonsChanged();
    emit selectedPolygonChanged(); 
}

void KmlPolygonLoader::removePolygon(QObject* polygon) {
    int index = _polygonObjects.indexOf(polygon);
    if (index >= 0) {
        _polygonObjects.removeAt(index);
        emit polygonsChanged(); 
        if (_selectedPolygon == polygon) {
            _selectedPolygon = nullptr;
            emit selectedPolygonChanged(); 
        }
    }
}

void KmlPolygonLoader::selectPolygon(QObject* polygon) {
    if (_selectedPolygon != polygon) {
        _selectedPolygon = polygon;
        //qDebug() << "Polygon selected: " << polygon->name;
        emit selectedPolygonChanged(); 
    }
}

QObject* KmlPolygonLoader::selectedPolygon() const {
    return _selectedPolygon;
}