#pragma once

#include <QObject>
#include <QPointer>
#include <QMetaObject>

// Forward Declaration
class ParameterManager;
class Vehicle;

class SpeedSelectorController : public QObject
{
    Q_OBJECT
public:
    explicit SpeedSelectorController(QObject* parent = nullptr);

    // Property needed to trace the vehicle status
    Q_PROPERTY(Vehicle* vehicle     READ vehicle    WRITE setVehicle    NOTIFY vehicleChanged)
    Q_PROPERTY(bool ready           READ ready                          NOTIFY readyChanged)

    // Possible speed modes
    enum SpeedMode {
        NormalSpeed,
        HighSpeed,
        LowSpeed
    };
    Q_ENUM(SpeedMode)

    // Needed Methods
    Vehicle* vehicle() const;
    void setVehicle(Vehicle* vehicle);

    bool ready() const { return _ready; }

    // Invokable method in qml
    Q_INVOKABLE bool requestSpeedMode(SpeedMode mode);

signals:
    void vehicleChanged();
    void readyChanged();

private:
    void _disconnectVehicle();
    void _updateReady();
    void _setReady(bool ready);

    QMetaObject::Connection _parametersReadyConnection;
    QMetaObject::Connection _parametersManagerDestroyedConnection;
    QMetaObject::Connection _vehicleDeletionConnection;
    QMetaObject::Connection _vehicleDestroyedConnection;

    QPointer<Vehicle> _vehicle;
    QPointer<ParameterManager> _parameterManager;

    bool _ready = false;
    // Incremental counter - checks if the vehicle changed
    qint64 _vehicleGeneration = 0;
};
