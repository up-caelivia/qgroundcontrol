#pragma once

#include "QGCFencePolygon.h"

class GeoAwarenessFencePolygon : public QGCFencePolygon
{
    Q_OBJECT
    Q_PROPERTY(bool isKml READ isKml WRITE setIsKml NOTIFY isKmlChanged)

public:
    GeoAwarenessFencePolygon(bool inclusion, QObject* parent = nullptr);

    bool isKml() const { return _isKml; }
    void setIsKml(bool kml);

signals:
    void isKmlChanged();

private:
    bool _isKml = false;
};
