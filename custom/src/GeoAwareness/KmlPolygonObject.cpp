#include "KmlPolygonObject.h"

KmlPolygonObject::KmlPolygonObject(const QString& name, int allowedAltitude, const QList<QGeoCoordinate>& coordinates, QObject* parent)
    : QObject(parent), _name(name), _allowedAltitude(allowedAltitude), _coordinates(coordinates) {}

QString KmlPolygonObject::name() const {
    return _name;
}

int KmlPolygonObject::allowedAltitude() const {
    return _allowedAltitude;
}

QVariantList KmlPolygonObject::coordinates() const {
    QVariantList list;
    for (const auto& coord : _coordinates) {
        list.append(QVariant::fromValue(coord));
    }
    return list;
}

