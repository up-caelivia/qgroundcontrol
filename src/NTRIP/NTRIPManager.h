/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QMap>
#include <QString>

#include "QGCToolbox.h"
#include "NTRIPConfiguration.h"

class NTRIPConnection;
class RTCMMavlink;

class NTRIPManager : public QGCTool {
    Q_OBJECT

public:
    NTRIPManager(QGCApplication* app, QGCToolbox* toolbox);

    Q_INVOKABLE bool connectConfiguration(const QString& connection_id);
    Q_INVOKABLE void disconnectConnection();

    // QGCTool overrides
    void setToolbox(QGCToolbox* toolbox) final;

signals:
    void rtcmDataReceived(QByteArray message);
    void errorOccurred(const QString& message);

public slots:
    void tcpError          (const QString errorMsg);

private:
    QMap<QString, NTRIPConfiguration> _configurations;

    NTRIPConnection* _tcpLink = nullptr;
    RTCMMavlink* _rtcmMavlink = nullptr;
};
