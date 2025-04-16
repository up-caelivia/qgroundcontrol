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
    explicit KmlPolygonLoader(QObject* parent = nullptr);
    Q_INVOKABLE bool loadFromFile(const QString& filePath);
    QList<QObject*> polygons() const;

signals:
    void polygonsChanged();

private:
    QList<QObject*> _polygonObjects;
    void parsePlacemark(const QDomElement& placemark);
};
