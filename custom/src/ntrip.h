/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QThread>
#include <QTcpSocket>
#include <QGeoCoordinate>
#include <QUrl>

#include "Drivers/src/rtcm.h"
#include "RTCM/RTCMMavlink.h"
#include "constants.h"

struct LatLongAlt {
    double latitude;
    double longitude;
    double altitude;
    double antennaHeight;
    uint16_t referenceStationId;
    uint16_t messageType;
    bool isValid;
};



class NTRIPSettings;

class NTRIPTCPLink : public QThread
{
    Q_OBJECT

public:
    NTRIPTCPLink(const QString& hostAddress,
                 int port,
                 const QString& username,
                 const QString& password,
                 const QString& mountpoint,
                 const QString& whitelist,
                 const bool&    enableVRS);
    ~NTRIPTCPLink();

signals:
    void error(const QString errorMsg);
    void RTCMDataUpdate(QByteArray message);

protected:
    void run() final;

private slots:
    void _readBytes();

private:
    enum class NTRIPState {
        uninitialised,
        waiting_for_http_response,
        waiting_for_rtcm_header,
        accumulating_rtcm_packet,
    };

    void _hardwareConnect(void);
    void _parse(const QByteArray &buffer);
    void startTimer(void);
    Constants* constants = Constants::getInstance();
    LatLongAlt decode_type1005_1006(const QByteArray &data);


    QTcpSocket*     _socket =   nullptr;

    QString         _hostAddress;
    int             _port;
    QString         _username;
    QString         _password;
    QString         _mountpoint;
    QSet<int>       _whitelist;
    bool            _isVRSEnable;

    // QUrl
    QUrl            _ntripURL;

    // Send NMEA
    void    _sendNMEA();
    QString _getCheckSum(QString line);

    // VRS Timer
    QTimer*          _vrsSendTimer;
    QTimer*          _requestTimer;

    static const int _reqSendRateMSecs = 12000;
    static const int _vrsSendRateMSecs = 5000;

    RTCMParsing *_rtcm_parsing{nullptr};
    NTRIPState _state;

    QGCToolbox*  _toolbox = nullptr;
};






class NTRIP : public QGCTool {
    Q_OBJECT

public:
    NTRIP(QGCApplication* app, QGCToolbox* toolbox);

    // QGCTool overrides
    void setToolbox(QGCToolbox* toolbox) final;

public slots:
    void _tcpError          (const QString errorMsg);

private slots:

private:
    NTRIPTCPLink*                    _tcpLink = nullptr;
    RTCMMavlink*                     _rtcmMavlink = nullptr;
};
