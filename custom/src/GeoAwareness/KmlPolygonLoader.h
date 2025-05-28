#pragma once

#include <QObject>
#include <QList>
#include <QGeoCoordinate>
#include <QDomDocument>

class KmlPolygonObject;

class KmlPolygonLoader : public QObject {
    Q_OBJECT
    Q_PROPERTY(QList<QObject*> polygons READ polygons NOTIFY polygonsChanged)
    Q_PROPERTY(QObject* selectedPolygon READ selectedPolygon NOTIFY selectedPolygonChanged)

public:
    static KmlPolygonLoader* instance();
    explicit KmlPolygonLoader(QObject* parent = nullptr);
    Q_INVOKABLE bool loadFromFile(const QString& filePath, const QVariantList& extraPolygonCoords);
    QList<QObject*> polygons() const;
    Q_INVOKABLE void clearPolygons();
    Q_INVOKABLE void removePolygon(QObject* polygon, bool forceUnselect = false);
    Q_INVOKABLE void selectPolygon(QObject* polygon, bool forceSelect = false);
    QObject* selectedPolygon() const;
    Q_INVOKABLE bool checkDronePosition();
    Q_INVOKABLE QList<QObject*> checkPolygonsInPoint(const QGeoCoordinate& clickCoord);
    Q_INVOKABLE bool exportToKmlFile(const QString& filePath);

signals:
    void polygonsChanged();
    void selectedPolygonChanged();

private:
    QList<QObject*> _polygonObjects;
    QObject* _selectedPolygon = nullptr;
    QObject* _selectedPolygonFence = nullptr;

    bool loadFromKmlFile(const QString& filePath);
    void parseSwissCyprusKml(const QDomDocument& doc);
    void parseUpCaeliViaKml(const QDomDocument& doc);

    bool loadFromJsonFile(const QString& filePath);
    bool parseItalyFinnishGermanJson(const QJsonObject& root);
    void parseBelgiumJson(const QJsonObject& root);

    bool parseSingleFeature(const QJsonObject& feature);
    bool parseED269Json(const QJsonObject& root);

    bool isPointInPolygon(const QGeoCoordinate& point, const QList<QGeoCoordinate>& polygon);
};
