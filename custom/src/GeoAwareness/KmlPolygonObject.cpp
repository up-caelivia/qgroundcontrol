#include "KmlPolygonObject.h"
#include <QVariant>

KmlPolygonObject::KmlPolygonObject(const QString& name, int allowedAltitude, const QList<QGeoCoordinate>& coords, QObject* parent)
    : QObject(parent)
    , _name(name)
    , _allowedAltitude(allowedAltitude)
    , _coordinates(coords)
{
}

QVariantList KmlPolygonObject::coordinates() const {
    QVariantList list;
    for (const QGeoCoordinate& c : _coordinates) {
        list.append(QVariant::fromValue(c));
    }
    return list;
}

void KmlPolygonObject::setName(const QString& name) {
    if (name != _name) {
        _name = name;
        emit nameChanged();
    }
}
