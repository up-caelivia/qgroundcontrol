#include "BatteryTimeController.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "Fact.h"
#include "QmlObjectListModel.h"
#include "VehicleBatteryFactGroup.h"

BatteryTimeController::BatteryTimeController(QObject* parent)
    : QObject(parent)
{
}

Vehicle* BatteryTimeController::vehicle() const
{
    return _vehicle.data();
}

void BatteryTimeController::setVehicle(Vehicle* vehicle)
{
    QPointer<Vehicle> newVehicle = vehicle;

    if (newVehicle && newVehicle->isBeingDeleted()) {
        newVehicle.clear();
    }

    if (_vehicle == newVehicle) {
        return;
    }

    _disconnectVehicle();

    if (newVehicle && newVehicle->isBeingDeleted()) {
        newVehicle.clear();
    }

    _vehicle = newVehicle;

    if (_vehicle) {
        const auto clearVehicle = [this]() {
            _disconnectVehicle();
            emit vehicleChanged();
        };

        _vehicleDeletionConnection = connect(
            _vehicle.data(), &Vehicle::aboutToBeDeleted,
            this, clearVehicle);

        _vehicleDestroyedConnection = connect(
            _vehicle.data(), &QObject::destroyed,
            this, clearVehicle);
    }

    _rebuildBatteryConnections();
    emit vehicleChanged();
}

void BatteryTimeController::_disconnectVehicle()
{
    QObject::disconnect(_vehicleDeletionConnection);
    QObject::disconnect(_vehicleDestroyedConnection);

    for (const auto& connection : _batteryConnections) {
        QObject::disconnect(connection);
    }
    _batteryConnections.clear();

    _vehicle.clear();
    _updateBatteryState();
}

void BatteryTimeController::_rebuildBatteryConnections()
{
    for (const auto& connection : _batteryConnections) {
        QObject::disconnect(connection);
    }
    _batteryConnections.clear();

    if (!_vehicle || _vehicle->isBeingDeleted()) {
        _updateBatteryState();
        return;
    }

    QmlObjectListModel* batteries = _vehicle->batteries();

    _batteryConnections.append(connect(
        batteries, &QAbstractItemModel::rowsInserted,
        this, &BatteryTimeController::_rebuildBatteryConnections));

    _batteryConnections.append(connect(
        batteries, &QAbstractItemModel::rowsRemoved,
        this, &BatteryTimeController::_rebuildBatteryConnections));

    _batteryConnections.append(connect(
        batteries, &QAbstractItemModel::modelReset,
        this, &BatteryTimeController::_rebuildBatteryConnections));

    for (int index = 0; index < batteries->count(); ++index) {
        auto* battery =
                qobject_cast<VehicleBatteryFactGroup*>(batteries->get(index));

        if (!battery) {
            continue;
        }

        for (Fact* fact : {
                 battery->function(),
                 battery->timeRemaining(),
                 battery->chargeState()}) {
            _batteryConnections.append(connect(
                fact, &Fact::rawValueChanged,
                this, &BatteryTimeController::_updateBatteryState));
        }

        _batteryConnections.append(connect(
            battery, &VehicleBatteryFactGroup::telemetryAvailableChanged,
            this, &BatteryTimeController::_updateBatteryState));
    }

    _updateBatteryState();
}

void BatteryTimeController::_updateBatteryState()
{
    bool hasFlightBattery = false;
    bool estimatesComplete = true;
    bool hasUnknownStatus = false;
    double remainingSeconds = std::numeric_limits<double>::infinity();
    BatteryStatus combinedStatus = Normal;

    if (_vehicle && !_vehicle->isBeingDeleted()) {
        QmlObjectListModel* batteries = _vehicle->batteries();

        for (int index = 0; index < batteries->count(); ++index) {
            auto* battery = qobject_cast<VehicleBatteryFactGroup*>(batteries->get(index));

            if (!battery) {
                continue;
            }

            const int function = battery->function()->rawValue().toInt();

            // Unspecified batteries are treated as required flight batteries.
            if (function != MAV_BATTERY_FUNCTION_PROPULSION &&
                    function != MAV_BATTERY_FUNCTION_ALL &&
                    function != MAV_BATTERY_FUNCTION_UNKNOWN) {
                continue;
            }

            hasFlightBattery = true;
            const bool telemetryAvailable = battery->telemetryAvailable();

            bool conversionSucceeded = false;
            const double seconds =
                    battery->timeRemaining()->rawValue().toDouble(&conversionSucceeded);

            if (telemetryAvailable && conversionSucceeded &&
                    std::isfinite(seconds) && seconds > 0.0) {
                remainingSeconds = std::min(remainingSeconds, seconds);
            } else {
                estimatesComplete = false;
            }

            BatteryStatus batteryStatus = Unknown;

            if (telemetryAvailable) {
                switch (battery->chargeState()->rawValue().toInt()) {
                case MAV_BATTERY_CHARGE_STATE_OK:
                    batteryStatus = Normal;
                    break;
                case MAV_BATTERY_CHARGE_STATE_LOW:
                    batteryStatus = Low;
                    break;
                case MAV_BATTERY_CHARGE_STATE_CRITICAL:
                case MAV_BATTERY_CHARGE_STATE_EMERGENCY:
                case MAV_BATTERY_CHARGE_STATE_FAILED:
                case MAV_BATTERY_CHARGE_STATE_UNHEALTHY:
                    batteryStatus = Critical;
                    break;
                default:
                    break;
                }
            }

            hasUnknownStatus = hasUnknownStatus || batteryStatus == Unknown;

            if (batteryStatus > combinedStatus) {
                combinedStatus = batteryStatus;
            }
        }
    }

    const bool available = hasFlightBattery && estimatesComplete;

    if (!hasFlightBattery || (combinedStatus == Normal && hasUnknownStatus)) {
        combinedStatus = Unknown;
    }

    _publishState(available, available ? remainingSeconds : 0.0, combinedStatus);
}

void BatteryTimeController::_publishState(
        bool available, double seconds, BatteryStatus status)
{
    if (_remainingTimeAvailable == available &&
            _remainingSeconds == seconds &&
            _status == status) {
        return;
    }

    _remainingTimeAvailable = available;
    _remainingSeconds = seconds;
    _status = status;

    emit batteryStateChanged();
}
