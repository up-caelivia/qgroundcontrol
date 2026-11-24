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

    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(bool receivingData READ receivingData NOTIFY receivingDataChanged)

    enum State{
        Disconnected,
        Connecting,
        RequestStream,
        Reconnecting,
        Connected
    };
    Q_ENUM(State)

    enum class EndReason {
        Requested,
        Retryable,
        Fatal
    };

    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();

    State state() const { return _state; }
    bool receivingData() const { return _receivingData; }

signals:
    void RTCMDataUpdate(QByteArray message);
    void connectionEnded(EndReason, const QString& message);

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
    bool _readHandshakeLine(QByteArray& line);
    qint64 _handshakeBytesRead = 0;
    void _terminateTransport(EndReason reason, const QString& message);

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
    QTimer* _connectionDropTimer = nullptr;

    // RTCM packet assembly
    void _beginRtcmStream();
    RTCMParsing* _rtcm_parsing = nullptr;
    QTimer* _attemptTimer = nullptr;

    // HTTP chunk decoding
    bool _isChunked = false;
    ChunkState _chunkState = ChunkState::ReadingSize;
    int _chunkBytesLeft = 0;
    QByteArray _chunkSizeBuffer;

    // Periodic NMEA GGA transmission
    QTimer* _vrsSendTimer = nullptr;
    int _vrsSendRateMSecs = 3000;
};
