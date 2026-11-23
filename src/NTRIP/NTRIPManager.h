/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "NTRIPConfiguration.h"
#include "NTRIPConnection.h"
#include "QGCToolbox.h"

#include <QByteArray>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

#include <memory>

class ConfigurationIoExecutor;
class ConfigurationStoreInterface;
class RTCMMavlink;

class NTRIPManager : public QGCTool
{
    Q_OBJECT

    Q_PROPERTY(QVariantList configurations READ configurations NOTIFY configurationsChanged)
    Q_PROPERTY(bool storageReady READ storageReady NOTIFY storageReadyChanged)
    Q_PROPERTY(bool storageBusy READ storageBusy NOTIFY storageBusyChanged)
    Q_PROPERTY(bool shutdownRequested READ shutdownRequested NOTIFY shutdownRequestedChanged)
    Q_PROPERTY(NTRIPConnection::State connectionState READ connectionState NOTIFY connectionStateChanged)
    Q_PROPERTY(bool receivingData READ receivingData NOTIFY receivingDataChanged)
    Q_PROPERTY(QString connectionId READ connectionId NOTIFY connectionIdChanged)
    Q_PROPERTY(QString connectionName READ connectionName NOTIFY connectionIdChanged)

public:
    enum MutationResult {
        Accepted,
        Busy,
        NotReady,
        Invalid,
        NotFound,
        ShuttingDown
    };
    Q_ENUM(MutationResult)

    NTRIPManager(
        QGCApplication* app,
        QGCToolbox* toolbox,
        std::shared_ptr<ConfigurationStoreInterface> store = {});

    QVariantList configurations() const;
    bool storageReady() const { return _storageReady; }
    bool storageBusy() const { return _storageBusy; }
    bool shutdownRequested() const;
    NTRIPConnection::State connectionState() const;
    bool receivingData() const;
    QString connectionId() const;
    QString connectionName() const { return _activeConnectionName; }

    Q_INVOKABLE QVariantMap configuration(const QString& id) const;

    // Accepted requests complete asynchronously; rejected requests do not.
    Q_INVOKABLE MutationResult addConfiguration(const QVariantMap& values);
    Q_INVOKABLE MutationResult updateConfiguration(const QString& id, const QVariantMap& values);
    Q_INVOKABLE MutationResult removeConfiguration(const QString& id);

    // Initial load, or retry after an unsuccessful initial load.
    Q_INVOKABLE bool reloadConfigurations();

    Q_INVOKABLE bool connectConfiguration(const QString& id);
    Q_INVOKABLE void disconnectConnection();
    Q_INVOKABLE void requestShutdown();

    void setToolbox(QGCToolbox* toolbox) final;

signals:
    void configurationsChanged();
    void storageReadyChanged();
    void storageBusyChanged();

    void operationRejected(NTRIPManager::MutationResult result, const QString& message);
    void configurationOperationFinished(
        const QString& operation,
        const QString& id,
        bool success,
        const QString& message);

    void shutdownRequestedChanged();
    void shutdownReady();
    void shutdownFailed(const QString& message);

    void connectionStateChanged();
    void receivingDataChanged();
    void connectionIdChanged();

    void rtcmDataReceived(QByteArray message);
    void errorOccurred(const QString& message);

public slots:
    void tcpError(const QString& errorMsg);

private:
    enum class ShutdownState {
        Running,
        Waiting,
        Ready
    };

    MutationResult _checkMutationAllowed();
    MutationResult _reject(MutationResult reason, const QString& message);
    bool _reportError(const QString& message);

    MutationResult _prepareAndSave(
        const QString& id,
        const QVariantMap& values,
        const NTRIPConfiguration* existing,
        const QString& operation);

    MutationResult _save(
        const NTRIPProfileDocument& candidate,
        const QString& operation,
        const QString& id);

    void _finishStorage(bool success, const QString& message);
    void _notifyShutdownReady();

    NTRIPProfileDocument _profileDocument;

    std::shared_ptr<ConfigurationStoreInterface> _store;
    ConfigurationIoExecutor* _io = nullptr;

    bool _storageReady = false;
    bool _storageBusy = false;
    ShutdownState _shutdownState = ShutdownState::Running;

    NTRIPConnection* _tcpLink = nullptr;
    RTCMMavlink* _rtcmMavlink = nullptr;

    QString _activeConnectionId;
    QString _activeConnectionName;
};
