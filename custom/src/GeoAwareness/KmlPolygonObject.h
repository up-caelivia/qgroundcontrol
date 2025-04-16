#pragma once

#include <QObject>
#include <QGeoCoordinate>
#include <QVariant>

class KmlPolygonObject : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(int allowedAltitude READ allowedAltitude CONSTANT)
    Q_PROPERTY(QVariantList coordinates READ coordinates CONSTANT)

public:
    KmlPolygonObject(const QString& name, int allowedAltitude, const QList<QGeoCoordinate>& coords, QObject* parent = nullptr);

    QString name() const { return _name; }
    int allowedAltitude() const { return _allowedAltitude; }
    QVariantList coordinates() const;

private:
    QString _name;
    int _allowedAltitude;
    QList<QGeoCoordinate> _coordinates;
};
