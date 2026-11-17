#pragma once

#include <QObject>
#include <QByteArray>
#include <QTcpSocket>
#include <QUrl>
#include <QTimer>

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
    void _sendNmeaGga();
    void _hardwareConnect();
    void _parse(const QByteArray &buffer);
    QByteArray _dechunk(const QByteArray &data);
    void _onTcpConnected();

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

    const NTRIPConfiguration _configuration;
    QTcpSocket*     _socket =   nullptr;
    int             _vrsSendRateMSecs = 3000;

    // Chunked transfer encoding
    bool            _isChunked = false;
    ChunkState      _chunkState = ChunkState::ReadingSize;
    int             _chunkBytesLeft = 0;
    QByteArray      _chunkSizeBuffer;

    // QUrl
    QUrl            _ntripURL;

    QTimer* _vrsSendTimer = nullptr;

    RTCMParsing *_rtcm_parsing = nullptr;
    NTRIPState _state;

    QGCToolbox*  _toolbox = nullptr;
};
