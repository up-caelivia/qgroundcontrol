#include "GeoAwarenessFencePolygon.h"

GeoAwarenessFencePolygon::GeoAwarenessFencePolygon(bool inclusion, QObject* parent)
    : QGCFencePolygon(inclusion, parent)
{
}

void GeoAwarenessFencePolygon::setIsKml(bool kml)
{
    if (_isKml != kml) {
        _isKml = kml;
        emit isKmlChanged();
    }
}
