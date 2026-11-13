#pragma once

#include <QObject>
#include <QPointer>
#include <QMetaObject>
#include <QVector>

#include "Vehicle.h"

class BatteryTimeController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(Vehicle* vehicle
               READ vehicle WRITE setVehicle
               NOTIFY vehicleChanged)
    Q_PROPERTY(bool remainingTimeAvailable
               READ remainingTimeAvailable
               NOTIFY batteryStateChanged)
    Q_PROPERTY(double remainingSeconds
               READ remainingSeconds
               NOTIFY batteryStateChanged)
    Q_PROPERTY(BatteryStatus status
               READ status
               NOTIFY batteryStateChanged)

public:
    explicit BatteryTimeController(QObject* parent = nullptr);

    enum BatteryStatus {
        Unknown,
        Normal,
        Low,
        Critical
    };
    Q_ENUM(BatteryStatus)

    Vehicle* vehicle() const;
    void setVehicle(Vehicle* vehicle);

    bool remainingTimeAvailable() const { return _remainingTimeAvailable; }
    double remainingSeconds() const { return _remainingSeconds; }
    BatteryStatus status() const { return _status; }

signals:
    void vehicleChanged();
    void batteryStateChanged();

private:
    void _disconnectVehicle();
    void _rebuildBatteryConnections();
    void _updateBatteryState();
    void _publishState(bool available, double seconds, BatteryStatus status);

    QPointer<Vehicle> _vehicle;
    bool _remainingTimeAvailable = false;
    double _remainingSeconds = 0.0;
    BatteryStatus _status = Unknown;

    QMetaObject::Connection _vehicleDeletionConnection;
    QMetaObject::Connection _vehicleDestroyedConnection;
    QVector<QMetaObject::Connection> _batteryConnections;
};
