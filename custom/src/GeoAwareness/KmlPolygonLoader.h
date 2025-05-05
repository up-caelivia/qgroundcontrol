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
    Q_INVOKABLE bool loadFromFile(const QString& filePath);
    QList<QObject*> polygons() const;
    Q_INVOKABLE void clearPolygons();
    Q_INVOKABLE void removePolygon(QObject* polygon);
    Q_INVOKABLE void selectPolygon(QObject* polygon);
    QObject* selectedPolygon() const;

signals:
    void polygonsChanged();
    void selectedPolygonChanged();

private:
    QList<QObject*> _polygonObjects;
    QObject* _selectedPolygon = nullptr;

    bool loadFromKmlFile(const QString& filePath);
    void parseSwissCyprusKml(const QDomDocument& doc);
    
    bool loadFromJsonFile(const QString& filePath);
    void parseItalyFinnishJson(const QJsonObject& root);
    void parseBelgiumJson(const QJsonObject& root);
};
