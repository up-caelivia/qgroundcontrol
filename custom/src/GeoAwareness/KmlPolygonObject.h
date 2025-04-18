#pragma once

#include <QObject>
#include <QGeoCoordinate>
#include <QVariant>

class KmlPolygonObject : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(int allowedAltitude READ allowedAltitude CONSTANT)
    Q_PROPERTY(QVariantList coordinates READ coordinates NOTIFY coordinatesChanged)

public:
    KmlPolygonObject(const QString& name, int allowedAltitude, const QList<QGeoCoordinate>& coords, QObject* parent = nullptr);

    QString name() const { return _name; }
    void setName(const QString& name);

    int allowedAltitude() const { return _allowedAltitude; }
    QVariantList coordinates() const;

signals:
    void nameChanged();
    void coordinatesChanged();

private:
    QString _name;
    int _allowedAltitude;
    QList<QGeoCoordinate> _coordinates;
};
