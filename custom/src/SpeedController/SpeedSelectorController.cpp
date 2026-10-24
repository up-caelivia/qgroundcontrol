#include "SpeedSelectorController.h"

#include "Fact.h"
#include "FactSystem.h"
#include "ParameterManager.h"
#include "Vehicle.h"
#include "constants.h"

SpeedSelectorController::SpeedSelectorController(QObject* parent)
    : QObject(parent)
{
}

Vehicle* SpeedSelectorController::vehicle() const
{
    return _vehicle.data();
}

void SpeedSelectorController::setVehicle(Vehicle *vehicle)
{
    QPointer<Vehicle> newVehicle = vehicle;

    if (newVehicle && newVehicle->isBeingDeleted()) {
        newVehicle.clear();
    }

    if (_vehicle == newVehicle) {
        return;
    }

    _disconnectVehicle();

    // Double check to assure that the new vehicle is not being removed
    if (newVehicle && newVehicle->isBeingDeleted()) {
        newVehicle.clear();
    }

    _vehicle = newVehicle;

    if (_vehicle) {
        _parameterManager = _vehicle->parameterManager();
        const qint64 generation = _vehicleGeneration;
        _parametersReadyConnection = connect(
            _parameterManager.data(),
            &ParameterManager::parametersReadyChanged,
            this,
            [this, generation](bool) {
                if (generation == _vehicleGeneration) {
                    _updateReady();
                }
            }
        );

        _parametersManagerDestroyedConnection = connect(
            _parameterManager.data(),
            &QObject::destroyed,
            this,
            [this, generation](QObject*) {
                if (generation == _vehicleGeneration) {
                    _updateReady();
                }
            }
        );

        // helper function to clear the vehicle instance
        const auto clearVehicle = [this, generation]() {
            if (generation == _vehicleGeneration) {
                _disconnectVehicle();
                emit vehicleChanged();
            }

        };

        _vehicleDeletionConnection = connect(
            _vehicle.data(),
            &Vehicle::aboutToBeDeleted,
            this,
            clearVehicle
        );

        _vehicleDestroyedConnection = connect(
            _vehicle.data(),
            &QObject::destroyed,
            this,
            clearVehicle
        );
    }

    emit vehicleChanged();
    _updateReady();
}

void SpeedSelectorController::_disconnectVehicle()
{
    // Destroy all the connection relative to a certain vehicle
    QObject::disconnect(_parametersReadyConnection);
    QObject::disconnect(_parametersManagerDestroyedConnection);
    QObject::disconnect(_vehicleDeletionConnection);
    QObject::disconnect(_vehicleDestroyedConnection);

    // increase the generation counter
    ++_vehicleGeneration;

    _vehicle.clear();
    _parameterManager.clear();
    _setReady(false);
}

void SpeedSelectorController::_updateReady()
{
    _setReady(
        _vehicle &&
        _parameterManager &&
        !_vehicle->isBeingDeleted() &&
        _parameterManager->parametersReady()
    );
}

void SpeedSelectorController::_setReady(bool ready)
{
    if (ready != _ready) {
        _ready = ready;
        emit readyChanged();
    }
}

bool SpeedSelectorController::requestSpeedMode(SpeedMode mode)
{
    // Inconsistent situation
    if (!_ready ||
        !_vehicle ||
        !_parameterManager ||
        _vehicle->isBeingDeleted() ||
        !_parameterManager->parametersReady()) {
        return false;
    }

    Constants* constants = Constants::getInstance();
    QVector<int> values;

    switch (mode) {
    case NormalSpeed:
        values = constants->normalSpeed();
        break;
    case HighSpeed:
        values = constants->highSpeed();
        break;
    case LowSpeed:
        if (_vehicle->flightMode() != QStringLiteral("Loiter")) {
            return false;
        }
        values = constants->lowSpeed();
        break;
    default:
        return false;
    }

    QVector<QString> names = constants->factSpeedNames();
    if (names.size() != values.size()) {
        return false;
    }

    const qint64 generation = _vehicleGeneration;
    bool requested = false;

    for (int i = 0; i < names.size(); ++i) {
        if (generation != _vehicleGeneration ||
            !_ready ||
            !_vehicle ||
            !_parameterManager) {
            return false;
        }

        if (!_parameterManager->parameterExists(
                FactSystem::defaultComponentId, names.at(i))) {
            continue;
        }

        Fact* fact = _parameterManager->getParameter(
            FactSystem::defaultComponentId, names.at(i));
        fact->setCookedValue(values.at(i));

        requested = true;
    }

    return requested;
}
