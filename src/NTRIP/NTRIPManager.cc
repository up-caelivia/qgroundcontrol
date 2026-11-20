/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "NTRIPManager.h"

#include "Configuration/ConfigurationIoExecutor.h"
#include "Configuration/ConfigurationStoreInterface.h"
#include "NTRIPConfigurationCodec.h"
#include "NTRIPConfigurationMigration.h"
#include "NTRIPConfigurationQmlAdapter.h"
#include "NTRIPConfigurationValidation.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "RTCM/RTCMMavlink.h"
#include "SettingsManager.h"

#include <QUuid>

#include <utility>

QGC_LOGGING_CATEGORY(NTRIPManagerLog, "NTRIPManagerLog")

NTRIPManager::NTRIPManager(
    QGCApplication* app,
    QGCToolbox* toolbox,
    std::shared_ptr<ConfigurationStoreInterface> store)
    : QGCTool(app, toolbox)
    , _store(std::move(store))
{
}

void NTRIPManager::setToolbox(QGCToolbox* toolbox)
{
    QGCTool::setToolbox(toolbox);

    _rtcmMavlink = new RTCMMavlink(*toolbox);
    _rtcmMavlink->setParent(this);

    _io = toolbox->settingsManager()->configurationIo();

    if (!_store) {
        _store = toolbox->settingsManager()->configurationStore();
    }

    if (qgcApp()->runningUnitTests() && !_store) {
        return;
    }

    reloadConfigurations();
}

QVariantList NTRIPManager::configurations() const
{
    QVariantList result;
    result.reserve(_configurations.size());

    for (const NTRIPConfiguration& configuration : _configurations) {
        result.append(QVariantMap {
            {QStringLiteral("id"), configuration.id},
            {QStringLiteral("name"), configuration.name},
            {QStringLiteral("hostAddress"), configuration.hostAddress},
            {QStringLiteral("port"), static_cast<int>(configuration.port)}
        });
    }

    return result;
}

QVariantMap NTRIPManager::configuration(const QString& id) const
{
    const auto it = _configurations.constFind(id);

    return it == _configurations.cend()
        ? QVariantMap{}
        : NTRIPConfigurationQmlAdapter::toValues(it.value());
}

bool NTRIPManager::shutdownRequested() const
{
    return _shutdownState != ShutdownState::Running;
}

NTRIPManager::MutationResult NTRIPManager::_checkMutationAllowed()
{
    if (shutdownRequested() || (_io && !_io->isAccepting())) {
        return _reject(ShuttingDown, tr("Configuration storage is shutting down."));
    }

    if (!_store || !_io || !_storageReady) {
        return _reject(NotReady, tr("NTRIP configurations have not been loaded."));
    }

    if (_storageBusy) {
        return _reject(Busy, tr("An NTRIP configuration save is already in progress."));
    }

    return Accepted;
}

NTRIPManager::MutationResult NTRIPManager::_reject(
    MutationResult reason,
    const QString& message)
{
    emit operationRejected(reason, message);
    return reason;
}

bool NTRIPManager::_reportError(const QString& message)
{
    emit errorOccurred(message);

    if (!shutdownRequested()) {
        qgcApp()->showAppMessage(message);
    }

    return false;
}

bool NTRIPManager::reloadConfigurations()
{
    if (shutdownRequested() || (_io && !_io->isAccepting())) {
        _reject(ShuttingDown, tr("Configuration storage is shutting down."));
        return false;
    }

    if (!_store || !_io) {
        return _reportError(tr("The configuration store is unavailable."));
    }

    if (_storageBusy) {
        _reject(Busy, tr("An NTRIP configuration operation is already in progress."));
        return false;
    }

    if (_storageReady || _tcpLink) {
        return false;
    }

    const auto store = _store;
    const bool allowMigration = !qgcApp()->runningUnitTests();
    _storageBusy = true;

    const bool accepted = _io->submit(
        this,
        [store, allowMigration]() {
            ConfigurationIoResult result;

            const auto status = store->read(
                QStringLiteral("ntrip"), result.document, result.error);

            if (status == ConfigurationStoreInterface::ReadResult::Error) {
                return result;
            }

            if (status == ConfigurationStoreInterface::ReadResult::Loaded) {
                result.success = true;
                return result;
            }

            // Only confirmed absence permits legacy migration.
            NTRIPConfigurations imported;

            if (allowMigration
                && !NTRIPConfigurationMigration::readLegacy(imported, result.error)) {
                return result;
            }

            result.document = NTRIPConfigurationCodec::encode(imported);

            // An empty document also marks migration as complete.
            result.success = store->write(
                QStringLiteral("ntrip"), result.document, result.error);

            return result;
        },
        [this](const ConfigurationIoResult& result) {
            NTRIPConfigurations candidate;
            QString error = result.error;

            const bool success = result.success
                && NTRIPConfigurationCodec::decode(result.document, candidate, error);

            if (success) {
                _configurations = candidate;
                _storageReady = true;
                emit storageReadyChanged();
                emit configurationsChanged();
            } else {
                if (error.isEmpty()) {
                    error = tr("Cannot load NTRIP configurations.");
                }

                _reportError(error);
            }

            _finishStorage(success, error);
        });

    if (!accepted) {
        _storageBusy = false;
        _reject(ShuttingDown, tr("Configuration storage is shutting down."));
        return false;
    }

    emit storageBusyChanged();
    return true;
}

NTRIPManager::MutationResult NTRIPManager::addConfiguration(
    const QVariantMap& values)
{
    const MutationResult availability = _checkMutationAllowed();

    if (availability != Accepted) {
        return availability;
    }

    QString id;

    do {
        id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    } while (_configurations.contains(id));

    return _prepareAndSave(id, values, nullptr, QStringLiteral("add"));
}

NTRIPManager::MutationResult NTRIPManager::updateConfiguration(
    const QString& id,
    const QVariantMap& values)
{
    const MutationResult availability = _checkMutationAllowed();

    if (availability != Accepted) {
        return availability;
    }

    const auto it = _configurations.constFind(id);

    if (it == _configurations.cend()) {
        return _reject(NotFound, tr("The selected NTRIP configuration does not exist."));
    }

    return _prepareAndSave(id, values, &it.value(), QStringLiteral("update"));
}

NTRIPManager::MutationResult NTRIPManager::_prepareAndSave(
    const QString& id,
    const QVariantMap& values,
    const NTRIPConfiguration* existing,
    const QString& operation)
{
    NTRIPConfigurationQmlAdapter::Input input;
    QString error;

    if (!NTRIPConfigurationQmlAdapter::fromValues(values, input, error)) {
        return _reject(Invalid, error);
    }

    NTRIPConfiguration configuration = input.values;
    configuration.id = id;

    switch (input.passwordAction) {
    case NTRIPConfigurationQmlAdapter::PasswordAction::Keep:
        configuration.password = existing ? existing->password : QString{};
        break;

    case NTRIPConfigurationQmlAdapter::PasswordAction::Replace:
        configuration.password = input.values.password;
        break;

    case NTRIPConfigurationQmlAdapter::PasswordAction::Clear:
        configuration.password.clear();
        break;
    }

    NTRIPConfigurations candidate = _configurations;
    candidate.insert(id, configuration);

    if (!NTRIPConfigurationValidation::validate(candidate, error)) {
        return _reject(Invalid, error);
    }

    return _save(candidate, operation, id);
}

NTRIPManager::MutationResult NTRIPManager::removeConfiguration(const QString& id)
{
    const MutationResult availability = _checkMutationAllowed();

    if (availability != Accepted) {
        return availability;
    }

    if (!_configurations.contains(id)) {
        return _reject(NotFound, tr("The selected NTRIP configuration does not exist."));
    }

    NTRIPConfigurations candidate = _configurations;
    candidate.remove(id);

    return _save(candidate, QStringLiteral("remove"), id);
}

NTRIPManager::MutationResult NTRIPManager::_save(
    const NTRIPConfigurations& candidate,
    const QString& operation,
    const QString& id)
{
    Q_ASSERT(_store && _io && _storageReady && !_storageBusy);

    const QVariantMap document = NTRIPConfigurationCodec::encode(candidate);
    const auto store = _store;
    _storageBusy = true;

    const bool accepted = _io->submit(
        this,
        [store, document]() {
            ConfigurationIoResult result;
            result.success = store->write(
                QStringLiteral("ntrip"), document, result.error);
            return result;
        },
        [this, candidate, operation, id](const ConfigurationIoResult& result) {
            QString error = result.success ? QString{} : result.error;

            if (result.success) {
                // Publish only the snapshot acknowledged by successful storage.
                _configurations = candidate;
                emit configurationsChanged();
            } else {
                if (error.isEmpty()) {
                    error = tr("Cannot save NTRIP configurations.");
                }

                emit errorOccurred(error);
            }

            emit configurationOperationFinished(operation, id, result.success, error);
            _finishStorage(result.success, error);
        });

    if (!accepted) {
        _storageBusy = false;
        return _reject(ShuttingDown, tr("Configuration storage is shutting down."));
    }

    emit storageBusyChanged();
    return Accepted;
}

void NTRIPManager::_finishStorage(bool success, const QString& message)
{
    const bool closeFailed = shutdownRequested() && !success;

    if (closeFailed) {
        _shutdownState = ShutdownState::Running;
        emit shutdownRequestedChanged();
    }

    _storageBusy = false;
    emit storageBusyChanged();

    if (closeFailed) {
        emit shutdownFailed(message);
    }

    // Recheck current state: a notification receiver may have requested shutdown.
    _notifyShutdownReady();
}

void NTRIPManager::requestShutdown()
{
    if (shutdownRequested()) {
        return;
    }

    _shutdownState = ShutdownState::Waiting;
    emit shutdownRequestedChanged();
    _notifyShutdownReady();
}

void NTRIPManager::_notifyShutdownReady()
{
    if (_shutdownState == ShutdownState::Waiting && !_storageBusy) {
        _shutdownState = ShutdownState::Ready;
        emit shutdownReady();
    }
}

bool NTRIPManager::connectConfiguration(const QString& id)
{
    if (!_storageReady || shutdownRequested() || (_io && !_io->isAccepting())) {
        return _reportError(
            tr("NTRIP configurations are unavailable or the application is closing."));
    }

    // A pending save does not change the committed connection snapshot.
    if (_tcpLink) {
        return _reportError(tr("Disconnect the current NTRIP connection first."));
    }

    if (!_rtcmMavlink) {
        return _reportError(tr("The NTRIP manager is not ready."));
    }

    const auto it = _configurations.constFind(id);

    if (it == _configurations.cend()) {
        return _reportError(tr("The selected NTRIP configuration does not exist."));
    }

    auto* connection = new NTRIPConnection(it.value(), this);
    _tcpLink = connection;
    _activeConnectionId = id;
    _activeConnectionName = it.value().name;

    connect(connection, &NTRIPConnection::closed, this, [this, connection]() {
        if (_tcpLink == connection) {
            _tcpLink = nullptr;
            _activeConnectionId.clear();
            _activeConnectionName.clear();
            emit connectionIdChanged();
        }

        connection->deleteLater();
    });

    connect(connection, &NTRIPConnection::error,
            this, [this, connection](const QString& message) {
        if (_tcpLink == connection) {
            tcpError(message);
        }
    });

    connect(connection, &NTRIPConnection::RTCMDataUpdate,
            this, [this, connection](const QByteArray& message) {
        if (_tcpLink == connection) {
            _rtcmMavlink->RTCMDataUpdate(message);
        }
    });

    connect(connection, &NTRIPConnection::RTCMDataUpdate,
            this, [this, connection](const QByteArray& message) {
        if (_tcpLink == connection) {
            emit rtcmDataReceived(message);
        }
    });

    connect(connection, &NTRIPConnection::stateChanged,
            this, [this, connection]() {
        if (_tcpLink == connection) {
            emit connectionStateChanged();
        }
    });

    connect(connection, &NTRIPConnection::receivingDataChanged,
            this, [this, connection]() {
        if (_tcpLink == connection) {
            emit receivingDataChanged();
        }
    });

    emit connectionIdChanged();

    if (_tcpLink != connection) {
        return false;
    }

    connection->start();
    return true;
}

void NTRIPManager::disconnectConnection()
{
    if (_tcpLink) {
        _tcpLink->stop();
    }
}

NTRIPConnection::State NTRIPManager::connectionState() const
{
    return _tcpLink ? _tcpLink->state() : NTRIPConnection::Disconnected;
}

bool NTRIPManager::receivingData() const
{
    return _tcpLink && _tcpLink->receivingData();
}

QString NTRIPManager::connectionId() const
{
    return _activeConnectionId;
}

void NTRIPManager::tcpError(const QString& errorMsg)
{
    emit errorOccurred(errorMsg);

    if (!shutdownRequested()) {
        qgcApp()->showAppMessage(tr("NTRIP Server Error: %1").arg(errorMsg));
    }
}
