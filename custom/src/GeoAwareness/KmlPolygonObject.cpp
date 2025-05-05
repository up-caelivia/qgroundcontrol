#include "KmlPolygonObject.h"
#include <QVariant>
#include <QPolygonF>

KmlPolygonObject::KmlPolygonObject(const QString& name,
                                   const QList<QGeoCoordinate>& coords,
                                   const QColor& color,
                                   int hmin,
                                   int hmax,
                                   const QString& id,
                                   const QString& description,
                                   const QDateTime& activationDate,
                                   const QDateTime& deactivationDate,
                                   QObject* parent)
    : QObject(parent)
    , _name(name)
    , _coordinates(coords)
    , _color(color)
    , _hmin(hmin)
    , _hmax(hmax)
    , _id(id)
    , _description(description)
    , _activationDate(activationDate)
    , _deactivationDate(deactivationDate)
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

void KmlPolygonObject::setColor(const QColor& color) {
    if (color != _color) {
        _color = color;
        emit colorChanged();
    }
}

void KmlPolygonObject::setHmin(int hmin) {
    if (hmin != _hmin) {
        _hmin = hmin;
        emit hminChanged();
    }
}

void KmlPolygonObject::setHmax(int hmax) {
    if (hmax != _hmax) {
        _hmax = hmax;
        emit hmaxChanged();
    }
}

void KmlPolygonObject::setId(const QString& id) {
    if (id != _id) {
        _id = id;
        emit idChanged();
    }
}

void KmlPolygonObject::setDescription(const QString& description) {
    if (description != _description) {
        _description = description;
        emit descriptionChanged();
    }
}

void KmlPolygonObject::setActivationDate(const QDateTime& date) {
    if (date != _activationDate) {
        _activationDate = date;
        emit activationDateChanged();
    }
}

void KmlPolygonObject::setDeactivationDate(const QDateTime& date) {
    if (date != _deactivationDate) {
        _deactivationDate = date;
        emit deactivationDateChanged();
    }
}

bool KmlPolygonObject::contains(const QGeoCoordinate& coordinate) const {
    QPolygonF polygon;
    for (const auto& coord : _coordinates) {
        polygon << QPointF(coord.longitude(), coord.latitude());
    }
    QPointF point(coordinate.longitude(), coordinate.latitude());
    return polygon.containsPoint(point, Qt::OddEvenFill);
}
