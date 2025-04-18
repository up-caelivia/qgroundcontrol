#pragma once

#include <QObject>
#include <QList>
#include <QGeoCoordinate>
#include <QDomDocument>

class KmlPolygonObject;

class KmlPolygonLoader : public QObject {
    Q_OBJECT
    Q_PROPERTY(QList<QObject*> polygons READ polygons NOTIFY polygonsChanged)

public:
    static KmlPolygonLoader* instance();
    explicit KmlPolygonLoader(QObject* parent = nullptr);
    Q_INVOKABLE bool loadFromFile(const QString& filePath);
    QList<QObject*> polygons() const;
    Q_INVOKABLE void applyToGeoFence(QObject* controller);
    Q_INVOKABLE void clearPolygons();
    Q_INVOKABLE void removePolygon(QObject* polygon);

signals:
    void polygonsChanged();

private:
    QList<QObject*> _polygonObjects;
    void parsePlacemark(const QDomElement& placemark);
};
