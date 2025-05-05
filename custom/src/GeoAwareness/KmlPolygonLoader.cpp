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
    if (filePath.endsWith(".kml", Qt::CaseInsensitive)) {
        return loadFromKmlFile(filePath);
    } else if (filePath.endsWith(".json", Qt::CaseInsensitive)) {
        return loadFromJsonFile(filePath);
    } else {
        qWarning() << "Unsupported file format:" << filePath;
        return false;
    }
}

bool KmlPolygonLoader::loadFromKmlFile(const QString& filePath) {
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
    parseSwissCyprusKml(doc);

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

    QString title = root.value("title").toString("");
    if (title.contains("ITA ZoneVersion") || title.contains("Finnish UASZoneVersion")) {
        parseItalyFinnishJson(root);
    } else if (root.contains("0")) {
        parseBelgiumJson(root);
    } else
        qWarning() << "Unknow JSON file.";

    emit polygonsChanged();
    return true;
}

void KmlPolygonLoader::parseSwissCyprusKml(const QDomDocument& doc) {
    QDomNodeList placemarksList = doc.elementsByTagName("Placemark");
    for (int i = 0; i < placemarksList.count(); ++i) {
        QDomElement placemark =placemarksList.at(i).toElement();
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
                else if (key == "description") message = value;
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
        else {
            QDomElement descElement = placemark.firstChildElement("description");
            if (!descElement.isNull()) {
                description = descElement.text();
            }
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
        } else if (hmin > 0) {
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
}

void KmlPolygonLoader::parseItalyFinnishJson(const QJsonObject& root) {
    QJsonArray features = root["features"].toArray();
    for (const QJsonValue& val : features) {
        QJsonObject feature = val.toObject();
        QString id = feature.value("identifier").toString();
        QString name = feature.value("name").toString();
        QString message = feature.value("message").toString("-");
        QString restriction = feature.value("restriction").toString();

        // Reason
        QString reason = "-";
        QJsonArray reasonsArray = feature.value("reason").toArray();
        if (!reasonsArray.isEmpty()) {
            QStringList reasonList;
            for (const auto& r : reasonsArray)
                reasonList << r.toString();
            reason = reasonList.join(", ");
        }

        // Zone Authority
        QString service = "-", authority = "-", email = "-", contact = "-";
        QJsonArray zoneAuthArray = feature.value("zoneAuthority").toArray();
        if (!zoneAuthArray.isEmpty()) {
            QJsonObject auth = zoneAuthArray.first().toObject();
            service = auth.value("service").toString("-");
            authority = auth.value("name").toString("-");
            email = auth.value("email").toString("-");
            contact = auth.value("contactName").toString("-");
        }

        // Applicability Dates
        QDateTime activationDate, deactivationDate;
        QJsonArray applicabilityArray = feature.value("applicability").toArray();
        if (!applicabilityArray.isEmpty()) {
            QJsonObject applicability = applicabilityArray.first().toObject();
            if (applicability.contains("startDateTime"))
                activationDate = QDateTime::fromString(applicability["startDateTime"].toString(), Qt::ISODate);
            if (applicability.contains("endDateTime"))
                deactivationDate = QDateTime::fromString(applicability["endDateTime"].toString(), Qt::ISODate);
        }

        // Geometry: assume primo elemento
        int hmin = 0, hmax = 9999;
        QList<QGeoCoordinate> coordinates;
        QJsonArray geometryArray = feature.value("geometry").toArray();
        if (!geometryArray.isEmpty()) {
            QJsonObject geometry = geometryArray.first().toObject();
            hmin = geometry.value("lowerLimit").toInt(0);
            hmax = geometry.value("upperLimit").toInt(9999);

            QJsonObject projection = geometry.value("horizontalProjection").toObject();
            QJsonArray polygons = projection.value("coordinates").toArray();
            for (const auto& poly : polygons) {
                QJsonArray ring = poly.toArray();
                for (const auto& coordPair : ring) {
                    QJsonArray coords = coordPair.toArray();
                    if (coords.size() == 2) {
                        double lon = coords[0].toDouble();
                        double lat = coords[1].toDouble();
                        coordinates.append(QGeoCoordinate(lat, lon));
                    }
                }
            }
        }

        QColor color = Qt::red;
        if (hmin >=120) {
            color = Qt::green;
        } else if (hmin > 0) {
            color = QColor("orange");
        } else {
            color = Qt::red;
        }
        QString description = message + "\n" + restriction + "\n\nREASON:\n" + reason + "\n\nSERVICE:\n" + service + "\n\nAUTHORITY:\n" + authority + "\n\nCONTACT:\n" + contact + "\n" + email;

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
}


void KmlPolygonLoader::parseBelgiumJson(const QJsonObject& root) {
    QJsonObject obj0 = root["0"].toObject();
    QJsonArray features = obj0["features"].toArray(); 
    for (const QJsonValue& val : features) {
        int hmin = 0, hmax = 9999;
        QString email = "-";
        QJsonObject feature = val.toObject();
        QJsonObject properties = feature["properties"].toObject();
        QString id = properties.value("code").toString();
        QString name = properties.value("name").toString();
        QString categoryType = properties.value("categoryType").toString();
        hmin = properties.value("lowerLimit").toInt(0);
        hmax = properties.value("upperLimit").toInt(9999);
        email = properties.value("email").toString("-");

        // Reason
        QString reason = "-";
        QJsonArray reasonsArray = properties.value("reason").toArray();
        if (!reasonsArray.isEmpty()) {
            QStringList reasonList;
            for (const auto& r : reasonsArray)
                reasonList << r.toString();
            reason = reasonList.join(", ");
        }

        // Applicability Dates
        QDateTime activationDate, deactivationDate;
        QJsonObject obj1 = root["1"].toObject();
        QJsonArray features1 = obj1["features"].toArray(); 
        for (const QJsonValue& val : features1) {
            QJsonObject feature1 = val.toObject();
            QJsonObject properties1 = feature1["properties"].toObject();
            QString name1 = properties1.value("name").toString();
            if (name1 == name) {
                if (properties1.contains("startDateTime"))
                    activationDate = QDateTime::fromString(properties1.value("startDateTime").toString(), Qt::ISODate);
                if (properties1.contains("endDateTime"))
                    deactivationDate = QDateTime::fromString(properties1.value("endDateTime").toString(), Qt::ISODate);
                break;
            }
        }

        //messages
        QString message = "";
        QJsonObject obj3 = root["3"].toObject();
        QJsonArray features3 = obj3["features"].toArray(); 
        for (const QJsonValue& val : features3) {
            QJsonObject feature3 = val.toObject();
            QJsonObject properties3 = feature3["properties"].toObject();
            QString categoryType3 = properties3.value("Generieke_categorie_geozone").toString();
            if (categoryType == categoryType3) {
                if (properties3.contains("condition_en"))
                    message += properties3.value("condition_en").toString() + "\n";
            }
        }

        // Geometry: 
        QList<QGeoCoordinate> coordinates;
        QJsonObject geometry = feature.value("geometry").toObject();
        QJsonArray outerArray = geometry.value("coordinates").toArray();
        if (!outerArray.isEmpty()) {
            QJsonArray ring = outerArray.at(0).toArray();
            for (const auto& coordPair : ring) {
                QJsonArray coords = coordPair.toArray();
                if (coords.size() == 2) {
                    double lon = coords[0].toDouble();
                    double lat = coords[1].toDouble();
                    coordinates.append(QGeoCoordinate(lat, lon));
                }
            }
        }

        QColor color = Qt::red;
        if (hmin >=120) {
            color = Qt::green;
        } else if (hmin > 0) {
            color = QColor("orange");
        } else {
            color = Qt::red;
        }
        QString description = message + "\n\nREASON:\n" + reason + "\n\nCONTACT:\n" + email;

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