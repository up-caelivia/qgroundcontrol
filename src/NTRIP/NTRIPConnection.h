#pragma once

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QTcpSocket>
#include <QTimer>
#include <QUrl>

#include "Drivers/src/rtcm.h"
#include "NTRIPConfiguration.h"

class QGCToolbox;

class NTRIPConnection : public QObject
{
    Q_OBJECT

public:
    explicit NTRIPConnection(const NTRIPConfiguration& configuration, QObject* parent = nullptr);
    ~NTRIPConnection();

    void start();
    void stop();

signals:
    void error(const QString errorMsg);
    void RTCMDataUpdate(QByteArray message);
    void closed();

private slots:
    void _readBytes();

private:
    enum class NTRIPState {
        uninitialised,
        waiting_for_http_response,
        consuming_http_headers,
        waiting_for_rtcm_header,
        accumulating_rtcm_packet,
    };

    enum class ChunkState {
        ReadingSize,
        ReadingData,
        ReadingTrailer
    };

    // TCP connection and NTRIP request
    void _onTcpConnected();
    void _hardwareConnect();

    // Incoming correction data
    void _parse(const QByteArray& buffer);
    QByteArray _dechunk(const QByteArray& data);

    // Outgoing vehicle position
    void _sendNmeaGga();

    const NTRIPConfiguration _configuration;
    QGCToolbox* _toolbox = nullptr;

    // Connection and session state
    QTcpSocket* _socket = nullptr;
    QUrl _ntripURL;
    NTRIPState _state;
    bool _closed = false;

    // RTCM packet assembly
    RTCMParsing* _rtcm_parsing = nullptr;

    // HTTP chunk decoding
    bool _isChunked = false;
    ChunkState _chunkState = ChunkState::ReadingSize;
    int _chunkBytesLeft = 0;
    QByteArray _chunkSizeBuffer;

    // Periodic NMEA GGA transmission
    QTimer* _vrsSendTimer = nullptr;
    int _vrsSendRateMSecs = 3000;
};
