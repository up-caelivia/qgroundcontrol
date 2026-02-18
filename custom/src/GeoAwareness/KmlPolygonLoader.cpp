#include "KmlPolygonLoader.h"
#include "KmlPolygonObject.h"

#include "QGCApplication.h"
#include "QGCToolbox.h"
#include "GeoFenceController.h"
#include "QGCMapPolygon.h"

#include "Vehicle.h"
#include "MultiVehicleManager.h"

#include "AudioOutput.h"

#include <QFile>
#include <QDomDocument>
#include <QDebug>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "ParameterManager.h"
#include "FactSystem.h"
#include "Fact.h"



KmlPolygonLoader::KmlPolygonLoader(QObject* parent)
    : QObject(parent) {}

KmlPolygonLoader* KmlPolygonLoader::instance() {
    static KmlPolygonLoader* _instance = new KmlPolygonLoader();
    return _instance;
}

bool KmlPolygonLoader::loadFromFile(const QString& filePath, const QVariantList& polygonCoords) {
    int initialSize = _polygonObjects.size();
    bool returnValue = false;
    if (filePath.endsWith(".kml", Qt::CaseInsensitive)) {
        returnValue = loadFromKmlFile(filePath);
    } else if (filePath.endsWith(".json", Qt::CaseInsensitive)) {
        returnValue = loadFromJsonFile(filePath);
    } else if (filePath.endsWith(".geojson", Qt::CaseInsensitive)) {
        returnValue = loadFromJsonFile(filePath);
    } else {
        qWarning() << "Unsupported file format:" << filePath;
        qgcApp()->showAppMessage(QString("Unsupported file format: %1").arg(filePath));
    }
    
    if (!returnValue || polygonCoords.isEmpty()) {
        emit polygonsChanged();
        return returnValue;
    }

    // Convert QVariantList to QList<QGeoCoordinate>
    QList<QGeoCoordinate> boundaryPolygon;
    for (const QVariant& var : polygonCoords) {
        boundaryPolygon.append(var.value<QGeoCoordinate>());
    }

    // Keep polygons that have at least one point inside the boundary
    QList<QObject*> validPolygons;
    for (QObject* obj : _polygonObjects) {
        KmlPolygonObject* polygon = qobject_cast<KmlPolygonObject*>(obj);
        if (!polygon)
            continue;

        const QVariantList& coordsVarList = polygon->coordinates();
        bool hasPointInside = false;

        // 1. Check if at least one point of the polygon is inside the reference area
        for (const QVariant& v : coordsVarList) {
            QGeoCoordinate pt = v.value<QGeoCoordinate>();
            if (isPointInPolygon(pt, boundaryPolygon)) {
                hasPointInside = true;
                break;
            }
        }

        // 2. Or: all reference boundary points are inside this polygon
        bool containsArea = true;
        for (const QGeoCoordinate& pt : boundaryPolygon) {
            if (!polygon->contains(pt)) {
                containsArea = false;
                break;
            }
        }

        if (hasPointInside || containsArea) {
            validPolygons.append(polygon);
        } else {
            polygon->deleteLater();
        }
    }
    qgcApp()->showAppMessage(QString("Read %1 polygon(s), and loaded %2 polygon(s) on the map. Only polygons that fit within the current view are displayed.")
        .arg(_polygonObjects.size() - initialSize)
        .arg(validPolygons.size() - initialSize));

    _polygonObjects = validPolygons;

    emit polygonsChanged();
    return true;
}

bool KmlPolygonLoader::loadFromKmlFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Cannot open KML file:" << filePath;
        qgcApp()->showAppMessage(QString("Cannot open KML file: %1").arg(filePath));
        return false;
    }

    QDomDocument doc;
    if (!doc.setContent(&file)) {
        qWarning() << "Failed to parse KML.";
        qgcApp()->showAppMessage(QString("Failed to parse KML"));
        return false;
    }

    QDomElement root = doc.documentElement();
    QDomElement documentElem = root.firstChildElement("Document");
    if (!documentElem.isNull()) {
        QDomElement nameElem = documentElem.firstChildElement("name");
        if (!nameElem.isNull()) {
            QString docName = nameElem.text();
            if (docName.contains("UP caeli via exported database", Qt::CaseSensitive)) {
                parseUpCaeliViaKml(doc);
                return true;                
            }
        }
    }

    parseSwissCyprusKml(doc);

    return true;
}

bool KmlPolygonLoader::loadFromJsonFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Cannot open JSON file:" << filePath;
        qgcApp()->showAppMessage(QString("Cannot open JSON file: %1").arg(filePath));
        return false;
    }

    QByteArray jsonData = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(jsonData);

    if (doc.isObject()) {
        const QJsonObject root = doc.object();

        QString title = root.value("title").toString("");
        bool res = false;
        if (title.contains("ITA ZoneVersion") || title.contains("Finnish UASZoneVersion")) {
            res = parseItalyFinnishGermanJson(root);
        } else if (root.contains("0")) {
            parseBelgiumJson(root);
            res = true;
        } else {
            res = parseED269Json(root);
            if (!res) {
                res = parseED318Json(root);
            }
        }
        if (!res) {
            qWarning() << "Unknow JSON file.";
            qgcApp()->showAppMessage(QString("Unknow JSON file."));
        }
    } else if (doc.isArray()) {
        QJsonArray arr = doc.array();
        if (!arr.isEmpty()) {
            QJsonObject first = arr.first().toObject();
            QString country = first.value("country").toString();
            if (country == "DEU") {
                QJsonObject wrapper;
                wrapper.insert("features", arr);
                parseItalyFinnishGermanJson(wrapper);
            } else {
                qWarning() << "Unsupported country:" << country;
                qgcApp()->showAppMessage(QString("Unsupported country: %1").arg(country));
            }
        } else {
            qWarning() << "Unknow JSON file.";
            qgcApp()->showAppMessage(QString("Unknow JSON file."));
        }
    } else {        
        qWarning() << "Invalid JSON structure.";
        qgcApp()->showAppMessage(QString("Invalid JSON structure."));
        return false;
    }

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

        if (!activationDate.isNull() && !deactivationDate.isNull()) {
            if (activationDate > QDateTime::currentDateTime() || deactivationDate < QDateTime::currentDateTime()) {
                color = QColor("blue");
            } 
        }
        
        if (coordinates.size() > 0) {
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
                "",
                this
                // Add holes as needed in your KmlPolygonObject class
            ));
            qDebug() << "Loaded polygon:" << name << "with" << coordinates.size() << "coordinates.";
        }
    }
}

void KmlPolygonLoader::parseUpCaeliViaKml(const QDomDocument& doc) {
    QDomNodeList placemarksList = doc.elementsByTagName("Placemark");
    for (int i = 0; i < placemarksList.count(); ++i) {
        QDomElement placemark =placemarksList.at(i).toElement();
        QString name = placemark.firstChildElement("name").text().trimmed();
        QString id, description;
        QDateTime activationDate, deactivationDate;
        int hmin = 0;
        int hmax = 9999;
        QColor color = Qt::red;
        QList<QGeoCoordinate> coordinates;

        // Parse <ExtendedData>
        QDomElement extData = placemark.firstChildElement("ExtendedData");
        if (!extData.isNull()) {
            QDomNodeList simpleDataList = extData.elementsByTagName("SimpleData");
            for (int i = 0; i < simpleDataList.count(); ++i) {
                QDomElement dataElem = simpleDataList.at(i).toElement();
                QString key = dataElem.attribute("name");
                QString value = dataElem.text().trimmed();

                if (key == "Identifier") id = value;
                else if (key == "Message") description = value.replace("&#10;", "\n");
                else if (key == "StartDate") activationDate = QDateTime::fromString(value, Qt::ISODate);
                else if (key == "EndDate") deactivationDate = QDateTime::fromString(value, Qt::ISODate);
                else if (key == "LowerLimit") hmin = value.toInt();
                else if (key == "UpperLimit") hmax = value.toInt();
                else if (key == "Name_en") name = value.trimmed();
                else if (key == "Color") color = QColor(value);
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
        }

        // Create and store the polygon object
        if (coordinates.size() > 0) {
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
                "",
                this
            ));
        }
    }
}


bool KmlPolygonLoader::parseItalyFinnishGermanJson(const QJsonObject& root) {
    QJsonArray features = root["features"].toArray();
    bool ret = false;
    for (const QJsonValue& val : features) {
        QJsonObject feature = val.toObject();
        ret |= parseSingleFeatureED269(feature); // Funzione helper
    }
    return ret;
}


bool KmlPolygonLoader::parseED269Json(const QJsonObject& root) {
    if (root.contains("features") && root.value("features").isArray()) {
        QJsonArray features = root["features"].toArray();
        bool ret = false;
        for (const QJsonValue& val : features) {
            QJsonObject feature = val.toObject();
            ret |= parseSingleFeatureED269(feature);
        }
        return ret;
    } else {
        return parseSingleFeatureED269(root);
    }
}

bool KmlPolygonLoader::parseSingleFeatureED269(const QJsonObject& feature) {
    QString id = feature.value("identifier").toString();
    QString name = feature.value("name").toString();
    QString message = feature.value("message").toString("-");
    QString restriction = feature.value("restriction").toString();
    QString activationSchedule = "";

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

        if (applicability.contains("schedule")) {
            QJsonArray scheduleArray = applicability["schedule"].toArray();
            QStringList scheduleLines;
            for (const auto& s : scheduleArray) {
                QJsonObject sched = s.toObject();

                // Giorni (può essere array di stringhe)
                QString days;
                if (sched.contains("day") && sched["day"].isArray()) {
                    QJsonArray dayArray = sched["day"].toArray();
                    QStringList dayList;
                    for (const auto& d : dayArray) {
                        dayList << d.toString();
                    }
                    days = dayList.join(", ");
                }

                // Start/End time
                QString startTime = sched.value("startTime").toString("-");
                QString endTime = sched.value("endTime").toString("-");

                // Costruisci la riga
                QString line = QString("day(s) %1: %2 - %3").arg(days, startTime, endTime);
                scheduleLines << line;
            }

            if (!scheduleLines.isEmpty()) {
                activationSchedule = "Schedule:\n" + scheduleLines.join("\n");
            }
        }
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

    if (!activationDate.isNull() && !deactivationDate.isNull()) {
        if (activationDate > QDateTime::currentDateTime() || deactivationDate < QDateTime::currentDateTime()) {
            color = QColor("blue");
        }
    }

    QString description = message + "\n" + restriction + "\n\nREASON:\n" + reason + "\n\nSERVICE:\n" + service + "\n\nAUTHORITY:\n" + authority + "\n\nCONTACT:\n" + contact + "\n" + email;
    
    // Create and store the polygon object
    if (coordinates.size() > 0) {
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
            activationSchedule,
            this
        ));
        return true;
    }
    return false;
}


bool KmlPolygonLoader::parseED318Json(const QJsonObject& root) {
    if (root.contains("features") && root.value("features").isArray()) {
        QJsonArray features = root["features"].toArray();
        bool ret = false;
        for (const QJsonValue& val : features) {
            QJsonObject feature = val.toObject();
            ret |= parseSingleFeatureED318(feature);
        }
        return ret;
    } else {
        return parseSingleFeatureED318(root);
    }
}


bool KmlPolygonLoader::parseSingleFeatureED318(const QJsonObject& feature) {
    // 1. Properties
    QJsonObject properties = feature.value("properties").toObject();
    QString id = properties.value("identifier").toString();

    // name: array di oggetti {text, lang}
    QString name = "-";
    QJsonArray nameArray = properties.value("name").toArray();
    if (!nameArray.isEmpty()) {
        QJsonObject nameObj = nameArray.first().toObject();
        name = nameObj.value("text").toString("-");
    }

    QString message = "-";
    // In questo schema non c’è "message", quindi lascialo "-"

    // restriction/variant/type (adatta in base ai dati disponibili)
    QString restriction = properties.value("type").toString();

    // Reason: array di stringhe
    QString reason = "-";
    QJsonArray reasonsArray = properties.value("reason").toArray();
    if (!reasonsArray.isEmpty()) {
        QStringList reasonList;
        for (const auto& r : reasonsArray)
            reasonList << r.toString();
        reason = reasonList.join(", ");
    }

    // otherReasonInfo: array di oggetti {text, lang}
    QString otherReasonInfo = "-";
    QJsonArray otherReasonArray = properties.value("otherReasonInfo").toArray();
    if (!otherReasonArray.isEmpty()) {
        QStringList otherReasonList;
        for (const auto& obj : otherReasonArray)
            otherReasonList << obj.toObject().value("text").toString();
        otherReasonInfo = otherReasonList.join(", ");
    }

    // Zone Authority: array di oggetti
    QString service = "-", authority = "-", email = "-", contact = "-";
    QJsonArray zoneAuthArray = properties.value("zoneAuthority").toArray();
    if (!zoneAuthArray.isEmpty()) {
        QJsonObject auth = zoneAuthArray.first().toObject();

        // authority (name) è array di oggetti {text, lang}
        QJsonArray authorityNameArray = auth.value("name").toArray();
        if (!authorityNameArray.isEmpty())
            authority = authorityNameArray.first().toObject().value("text").toString("-");
        email = auth.value("email").toString("-");
        // contactName non c’è, ma puoi usare email o altro se vuoi
    }

    // Applicability Dates
    QDateTime activationDate, deactivationDate;
    QString activationSchedule;
    QJsonArray applicabilityArray = properties.value("limitedApplicability").toArray();
    if (!applicabilityArray.isEmpty()) {
        QJsonObject applicability = applicabilityArray.first().toObject();
        if (applicability.contains("startDateTime"))
            activationDate = QDateTime::fromString(applicability["startDateTime"].toString(), Qt::ISODate);
        if (applicability.contains("endDateTime"))
            deactivationDate = QDateTime::fromString(applicability["endDateTime"].toString(), Qt::ISODate);

        if (applicability.contains("schedule")) {
            QJsonArray scheduleArray = applicability["schedule"].toArray();
            QStringList scheduleLines;
            for (const auto& s : scheduleArray) {
                QJsonObject sched = s.toObject();

                // Giorni
                QString days;
                if (sched.contains("day") && sched["day"].isArray()) {
                    QJsonArray dayArray = sched["day"].toArray();
                    QStringList dayList;
                    for (const auto& d : dayArray)
                        dayList << d.toString();
                    days = dayList.join(", ");
                }

                // Start/End time
                QString startTime = sched.value("startTime").toString("-");
                QString endTime = sched.value("endTime").toString("-");

                QString line = QString("day(s) %1: %2 - %3").arg(days, startTime, endTime);
                scheduleLines << line;
            }
            if (!scheduleLines.isEmpty())
                activationSchedule = "Schedule:\n" + scheduleLines.join("\n");
        }
    }

    // 2. Geometry
    QList<QGeoCoordinate> coordinates;
    int hmin = 0, hmax = 9999;

    QJsonObject geometry = feature.value("geometry").toObject();
    // Polygon coordinates
    if (geometry.value("type").toString() == "Polygon") {
        QJsonArray polygons = geometry.value("coordinates").toArray();
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

    // Altitude: geometry.layer
    QJsonObject layer = geometry.value("layer").toObject();
    hmin = layer.value("lower").toInt(0);
    hmax = layer.value("upper").toInt(9999);

    // Color logic: come prima
    QColor color = Qt::red;
    if (hmin >= 120) {
        color = Qt::green;
    } else if (hmin > 0) {
        color = QColor("orange");
    } else {
        color = Qt::red;
    }
    if (!activationDate.isNull() && !deactivationDate.isNull()) {
        if (activationDate > QDateTime::currentDateTime() || deactivationDate < QDateTime::currentDateTime()) {
            color = QColor("blue");
        }
    }

    QString description = restriction + "\n" + otherReasonInfo + "\n\nREASON:\n" + reason + "\n\nSERVICE:\n" + service + "\n\nAUTHORITY:\n" + authority + "\n\nCONTACT:\n" + contact + "\n" + email;

    if (coordinates.size() > 0) {
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
            activationSchedule,
            this
        ));
        return true;
    }
    return false;
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
        QString activationSchedule = "";

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
        
        if (!activationDate.isNull() && !deactivationDate.isNull()) {
            if (activationDate > QDateTime::currentDateTime() || deactivationDate < QDateTime::currentDateTime()) {
                color = QColor("blue");
            } 
        }

        QString description = message + "\n\nREASON:\n" + reason + "\n\nCONTACT:\n" + email;

        // Create and store the polygon object
        if (coordinates.size() > 0) {
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
                activationSchedule,
                this
                // Add holes as needed in your KmlPolygonObject class
            ));
        }
    }
}


QList<QObject*> KmlPolygonLoader::polygons() const {
    return _polygonObjects;
}

void KmlPolygonLoader::clearPolygons() {
    _selectedPolygon = nullptr;
    _polygonObjects.clear();
    emit polygonsChanged();
    emit selectedPolygonChanged(); 
}

void KmlPolygonLoader::removePolygon(QObject* polygon, bool forceUnselect) {
    int index = _polygonObjects.indexOf(polygon);
    if (index >= 0) {
        if ((_selectedPolygon == polygon) || (forceUnselect)) {
            _selectedPolygon = nullptr;
            emit selectedPolygonChanged(); 
        }
        _polygonObjects.removeAt(index);
        emit polygonsChanged(); 
    }
}

void KmlPolygonLoader::selectPolygon(QObject* polygon, bool forceSelect) {
    if ((_selectedPolygon != polygon) || forceSelect) {
        _selectedPolygon = polygon;        
    } else {
        _selectedPolygon = nullptr;
    }
    emit selectedPolygonChanged(); 
}

QObject* KmlPolygonLoader::selectedPolygon() const {
    return _selectedPolygon;
}

bool KmlPolygonLoader::checkDronePosition()
{
    Vehicle* vehicle = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();
    if (!vehicle || !vehicle->coordinate().isValid()) {
        return false;
    }

    const QGeoCoordinate dronePos = vehicle->coordinate();
    const double altRel = vehicle->altitudeRelative()->rawValue().toDouble();

    // --------------------------------------------------------
    // Dynamic "near" buffer computation
    // Requirement: give warning early enough so the drone can stop in < 3s
    // We use a 4s safety buffer.
    // Horizontal: 4s * LOIT_SPEED
    // Vertical:   4s * PILOT_SPEED_UP
    // Note: ArduPilot params are usually in cm/s -> convert to m/s
    // --------------------------------------------------------

    const double bufferSec = 4.0;

    auto* pm = vehicle->parameterManager();

    double loitSpeed_cm_s = 1000.0;     // default fallback
    double pilotSpeedUp_cm_s = 500.0;  // default fallback

    if (pm) {

        if (pm->parameterExists(FactSystem::defaultComponentId, "LOIT_SPEED")){
            Fact* loitFact = pm->getParameter(FactSystem::defaultComponentId, "LOIT_SPEED");
            if (loitFact) { loitSpeed_cm_s = loitFact->rawValue().toDouble(); }
        }

        if (pm->parameterExists(FactSystem::defaultComponentId, "PILOT_SPEED_UP")){
            Fact* pilotUpFact = pm->getParameter(FactSystem::defaultComponentId, "PILOT_SPEED_UP");
            if (pilotUpFact) { pilotSpeedUp_cm_s = pilotUpFact->rawValue().toDouble();}
        }
    }

    const double loitSpeed_m_s    = loitSpeed_cm_s / 100.0;
    const double pilotUp_m_s      = pilotSpeedUp_cm_s / 100.0;

    double nearHorizMeters = bufferSec * loitSpeed_m_s;
    double nearVertMeters  = bufferSec * pilotUp_m_s;

    KmlPolygonObject* violationHit = nullptr;
    KmlPolygonObject* nearHit      = nullptr;

    for (QObject* obj : _polygonObjects) {

        auto* polygon = qobject_cast<KmlPolygonObject*>(obj);
        if (!polygon)
            continue;

        bool inside = polygon->contains(dronePos);

        // --------------------------------------------------------
        // VIOLATION CHECK:
        // Drone is inside the polygon and above minimum altitude
        // --------------------------------------------------------
        if (inside && altRel > polygon->hmin()) {
            violationHit = polygon;
            break; // violation has priority
        } else if (inside && (altRel < polygon->hmin() || altRel > polygon->hmax())) {
            inside = false;
        }

        // --------------------------------------------------------
        // NEAR CHECK:
        // Drone is outside but close to the polygon
        // --------------------------------------------------------

        // Horizontal proximity (distance from polygon edge)
        const bool nearHoriz =
            polygon->containsOrNear(dronePos, nearHorizMeters);

        // Vertical distance from altitude band [hmin, hmax]
        double vertDist = 0.0;

        if (altRel < polygon->hmin()) {
            // Drone below minimum altitude
            vertDist = polygon->hmin() - altRel;
        }

        const bool nearVert = (vertDist <= nearVertMeters);

        // Near condition:
        // - outside polygon
        // - close horizontally
        // - close vertically
        if (!inside && nearHoriz && nearVert) {
            nearHit = polygon;
            break;
        }
    }

    // --------------------------------------------------------
    // ALERT MANAGEMENT
    // Avoid repeating the same audio/message continuously
    // --------------------------------------------------------

    // Violation alert (highest priority)
    if (violationHit) {
        if (_selectedPolygonFence != violationHit || _lastAlertType != 2) {
            if (auto* msgHandler = qgcApp()->toolbox()->uasMessageHandler()) {
                const QString msg =
                    QStringLiteral("drone violated the geo-awareness zone");

                msgHandler->handleTextMessage(1, 1, 2, QTime::currentTime().toString("hh:mm:ss.zzz") + " " + msg, QString());
                qgcApp()->toolbox()->audioOutput()->say("WARNING : " + msg);
            }

            _selectedPolygonFence = violationHit;
            _lastAlertType = 2; // violation state
        }
        return true;
    }

    // Near alert (warning level)
    if (nearHit) {
        if (_selectedPolygonFence != nearHit || _lastAlertType != 1) {
            if (auto* msgHandler = qgcApp()->toolbox()->uasMessageHandler()) {
                const QString msg =
                    QStringLiteral("drone close to a geo-awareness zone");

                msgHandler->handleTextMessage(1, 1, 1, QTime::currentTime().toString("hh:mm:ss.zzz") + " " + msg, QString());
                qgcApp()->toolbox()->audioOutput()->say("CAUTION : " + msg);
            }

            _selectedPolygonFence = nearHit;
            _lastAlertType = 1; // near state
        }
        return false;
    }

    // No active warning or violation
    _selectedPolygonFence = nullptr;
    _lastAlertType = 0;

    return false;
}


bool KmlPolygonLoader::exportToKmlFile(const QString& filePath) {
    QDomDocument doc;
    
    QDomElement kmlElem = doc.createElement("kml");
    kmlElem.setAttribute("xmlns", "http://www.opengis.net/kml/2.2");
    doc.appendChild(kmlElem);

    QDomElement docElem = doc.createElement("Document");
    kmlElem.appendChild(docElem);

    // Titolo del documento
    QDomElement titleElem = doc.createElement("name");
    titleElem.appendChild(doc.createTextNode("UP caeli via exported database"));
    docElem.appendChild(titleElem);

    for (QObject* obj : _polygonObjects) {
        auto* poly = qobject_cast<KmlPolygonObject*>(obj);
        if (!poly) continue;

        QDomElement placemark = doc.createElement("Placemark");

        // <name>
        QDomElement nameElem = doc.createElement("name");
        nameElem.appendChild(doc.createTextNode(poly->name()));
        placemark.appendChild(nameElem);

        // <ExtendedData>
        QDomElement extData = doc.createElement("ExtendedData");

        auto appendSimpleData = [&](const QString& name, const QString& value) {
            QDomElement simple = doc.createElement("SimpleData");
            simple.setAttribute("name", name);
            simple.appendChild(doc.createTextNode(value));
            extData.appendChild(simple);
        };

        appendSimpleData("Identifier", poly->id());
        appendSimpleData("Message", poly->description().replace("\n", "&#10;")); 
        appendSimpleData("LowerLimit", QString::number(poly->hmin()));
        appendSimpleData("UpperLimit", QString::number(poly->hmax()));
        appendSimpleData("StartDate", poly->activationDate().toString(Qt::ISODate));
        appendSimpleData("EndDate", poly->deactivationDate().toString(Qt::ISODate));
        appendSimpleData("Color", poly->color().name());    
        placemark.appendChild(extData);

        // <Polygon>
        QDomElement polygon = doc.createElement("Polygon");

        QDomElement outerBoundary = doc.createElement("outerBoundaryIs");
        QDomElement linearRing = doc.createElement("LinearRing");
        QDomElement coordElem = doc.createElement("coordinates");

        QString coordText;
        for (const QVariant& var : poly->coordinates()) {
            QGeoCoordinate coord = var.value<QGeoCoordinate>();
            coordText += QString::number(coord.longitude(), 'f', 8) + "," +
                         QString::number(coord.latitude(), 'f', 8) + ",0 ";
        }
        coordElem.appendChild(doc.createTextNode(coordText.trimmed()));
        linearRing.appendChild(coordElem);
        outerBoundary.appendChild(linearRing);
        polygon.appendChild(outerBoundary);

        placemark.appendChild(polygon);
        docElem.appendChild(placemark);
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Failed to write KML file:" << filePath;
        return false;
    }

    QTextStream out(&file);
    doc.save(out, 4);
    file.close();

    return true;
}

QList<QObject*> KmlPolygonLoader::checkPolygonsInPoint(const QGeoCoordinate& clickCoord) {
    QList<QObject*> matchingPolygons;
    for (QObject* obj : _polygonObjects) {
        auto* polygon = qobject_cast<KmlPolygonObject*>(obj);
        if (polygon->contains(clickCoord)) {
            matchingPolygons.append(polygon);
        }
    }
    return matchingPolygons;
}

bool KmlPolygonLoader::isPointInPolygon(const QGeoCoordinate& point, const QList<QGeoCoordinate>& polygon)
{
    int intersections = 0;
    int count = polygon.size();
    if (count < 3)
        return false;

    for (int i = 0; i < count; ++i) {
        const QGeoCoordinate& p1 = polygon[i];
        const QGeoCoordinate& p2 = polygon[(i + 1) % count];

        if (((p1.latitude() > point.latitude()) != (p2.latitude() > point.latitude())) &&
            (point.longitude() < (p2.longitude() - p1.longitude()) * (point.latitude() - p1.latitude()) /
                                    (p2.latitude() - p1.latitude()) + p1.longitude())) {
            intersections++;
        }
    }
    return (intersections % 2) != 0;
}
