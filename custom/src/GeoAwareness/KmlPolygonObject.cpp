#include "KmlPolygonObject.h"
#include <QVariant>
#include <QPolygonF>
#include <QtMath>

KmlPolygonObject::KmlPolygonObject(const QString& name,
                                   const QList<QGeoCoordinate>& coords,
                                   const QColor& color,
                                   int hmin,
                                   int hmax,
                                   const QString& id,
                                   const QString& description,
                                   const QDateTime& activationDate,
                                   const QDateTime& deactivationDate,
                                   const QString& activationSchedule,
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
    , _activationSchedule(activationSchedule)
{
    if (!coords.isEmpty()) {
        _bboxMinLat = _bboxMaxLat = coords[0].latitude();
        _bboxMinLon = _bboxMaxLon = coords[0].longitude();
        for (const auto& c : coords) {
            _cachedPolygon << QPointF(c.longitude(), c.latitude());
            if (c.latitude()  < _bboxMinLat) _bboxMinLat = c.latitude();
            if (c.latitude()  > _bboxMaxLat) _bboxMaxLat = c.latitude();
            if (c.longitude() < _bboxMinLon) _bboxMinLon = c.longitude();
            if (c.longitude() > _bboxMaxLon) _bboxMaxLon = c.longitude();
        }
    }
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

void KmlPolygonObject::setActivationSchedule(const QString& description) {
    if (description != _activationSchedule) {
        _activationSchedule = description;
        emit activationScheduleChanged();
    }
}

bool KmlPolygonObject::contains(const QGeoCoordinate& coordinate) const {
    return _cachedPolygon.containsPoint(
        QPointF(coordinate.longitude(), coordinate.latitude()), Qt::OddEvenFill);
}

static double _distancePointSegment(double px, double py,
                                    double ax, double ay,
                                    double bx, double by)
{
    double vx = bx - ax;
    double vy = by - ay;
    double wx = px - ax;
    double wy = py - ay;

    double vv = vx*vx + vy*vy;

    if (vv < 1e-12) {
        double dx = px - ax;
        double dy = py - ay;
        return qSqrt(dx*dx + dy*dy);
    }

    double t = (wx*vx + wy*vy) / vv;
    t = qBound(0.0, t, 1.0);

    double cx = ax + t * vx;
    double cy = ay + t * vy;

    double dx = px - cx;
    double dy = py - cy;

    return qSqrt(dx*dx + dy*dy);
}

double KmlPolygonObject::minDistanceToEdges(const QGeoCoordinate& point) const
{
    int n = _coordinates.size();
    if (n < 2)
        return std::numeric_limits<double>::infinity();

    // conversione semplice gradi -> metri (ottima per geofence locali)
    const double metersLat = 111320.0;
    const double metersLon =
        111320.0 * qCos(qDegreesToRadians(point.latitude()));

    auto toLocal = [&](const QGeoCoordinate& c, double& x, double& y) {
        x = (c.longitude() - point.longitude()) * metersLon;
        y = (c.latitude()  - point.latitude())  * metersLat;
    };

    double minD = std::numeric_limits<double>::infinity();

    for (int i = 0; i < n; ++i) {

        double ax, ay, bx, by;

        toLocal(_coordinates[i], ax, ay);
        toLocal(_coordinates[(i + 1) % n], bx, by);

        double d = _distancePointSegment(
            0.0, 0.0,
            ax, ay,
            bx, by
        );

        if (d < minD)
            minD = d;
    }

    return minD;
}
bool KmlPolygonObject::containsOrNear(const QGeoCoordinate& coordinate,
                                      double thresholdMeters) const
{
    if (contains(coordinate))
        return true;

    return minDistanceToEdges(coordinate) <= thresholdMeters;
}

