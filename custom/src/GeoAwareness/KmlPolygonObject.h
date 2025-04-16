#pragma once

#include <QObject>
#include <QGeoCoordinate>
#include <QVariantList>

class KmlPolygonObject : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(int allowedAltitude READ allowedAltitude CONSTANT)
    Q_PROPERTY(QVariantList coordinates READ coordinates CONSTANT)

public:
    explicit KmlPolygonObject(const QString& name,
                              int allowedAltitude,
                              const QList<QGeoCoordinate>& coordinates,
                              QObject* parent = nullptr);

    QString name() const;
    int allowedAltitude() const;
    QVariantList coordinates() const;

private:
    QString _name;
    int _allowedAltitude;
    QList<QGeoCoordinate> _coordinates;
};
