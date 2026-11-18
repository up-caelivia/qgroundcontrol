#pragma once

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QTcpSocket>
#include <QTimer>
#include <QUrl>

#include "Drivers/src/rtcm.h"
#include "NTRIPConfiguration.h"
#include "qobjectdefs.h"

class QGCToolbox;

class NTRIPConnection : public QObject
{
    Q_OBJECT

public:
    explicit NTRIPConnection(const NTRIPConfiguration& configuration, QObject* parent = nullptr);
    ~NTRIPConnection();

    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(bool receivingData READ receivingData NOTIFY receivingDataChanged)

    enum State{
        Disconnected,
        Connecting,
        RequestStream,
        Connected
    };
    Q_ENUM(State)

    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();

    State state() const { return _state; }
    bool receivingData() const { return _receivingData; }

signals:
    void error(const QString errorMsg);
    void RTCMDataUpdate(QByteArray message);
    void closed();

    void stateChanged();
    void receivingDataChanged();

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

    // QML connection state variables
    State _state = State::Disconnected;
    bool _receivingData = false;

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
    NTRIPState _connectionState;
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
