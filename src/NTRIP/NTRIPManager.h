/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QByteArray>
#include <QMap>
#include <QString>

#include "QGCToolbox.h"
#include "NTRIPConnection.h"
#include "NTRIPConfiguration.h"

class RTCMMavlink;

class NTRIPManager : public QGCTool {
    Q_OBJECT

public:
    NTRIPManager(QGCApplication* app, QGCToolbox* toolbox);

    Q_PROPERTY(NTRIPConnection::State connectionState READ connectionState NOTIFY connectionStateChanged)
    Q_PROPERTY(bool receivingData READ receivingData NOTIFY receivingDataChanged)
    Q_PROPERTY(QString connectionId READ connectionId NOTIFY connectionIdChanged)

    Q_INVOKABLE bool connectConfiguration(const QString& connection_id);
    Q_INVOKABLE void disconnectConnection();

    // QML methods
    NTRIPConnection::State connectionState() const;
    bool receivingData() const;
    QString connectionId() const;

    // QGCTool overrides
    void setToolbox(QGCToolbox* toolbox) final;

signals:
    void rtcmDataReceived(QByteArray message);
    void errorOccurred(const QString& message);

    void connectionStateChanged();
    void receivingDataChanged();
    void connectionIdChanged();

public slots:
    void tcpError          (const QString errorMsg);

private:
    QMap<QString, NTRIPConfiguration> _configurations;

    NTRIPConnection* _tcpLink = nullptr;
    RTCMMavlink* _rtcmMavlink = nullptr;

    QString _activeConnectionId;
};
