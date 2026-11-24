/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "NTRIPConnection.h"
#include "NmeaMessage.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "QGCToolbox.h"

QGC_LOGGING_CATEGORY(NTRIPConnectionLog, "NTRIPConnectionLog")

NTRIPConnection::NTRIPConnection(
  const NTRIPConfiguration& configuration,
  QObject* parent)
    : QObject(parent),
      _configuration(configuration),
      _toolbox(qgcApp()->toolbox()) {

  if (!_rtcm_parsing) {
    _rtcm_parsing = new RTCMParsing();
  }
  _rtcm_parsing->reset();
  _connectionState = NTRIPState::uninitialised;
  _state = State::Disconnected;

  _socket = new QTcpSocket(this);

  _vrsSendTimer = new QTimer(this);
  _vrsSendTimer->setInterval(_vrsSendRateMSecs);
  _vrsSendTimer->setSingleShot(false);

  _attemptTimer = new QTimer(this);
  _attemptTimer->setSingleShot(true);
  _attemptTimer->setInterval(10000);
  connect(
    _attemptTimer,
    &QTimer::timeout,
    this,
    [this]() {
      const QString message = _state == State::Connected
        ? tr("Timed out waiting for RTCM corrections.")
        : tr("Timed out waiting for the NTRIP caster.");
      _terminateTransport(EndReason::Retryable, message);
    }
  );

  _connectionDropTimer = new QTimer(this);
  _connectionDropTimer->setSingleShot(true);
  _connectionDropTimer->setInterval(10000);
  connect(
    _connectionDropTimer,
    &QTimer::timeout,
    this,
    [this]() {
      _receivingData = false;
      emit receivingDataChanged();
    }
  );

  connect(_socket, &QTcpSocket::connected,
          this, &NTRIPConnection::_onTcpConnected);

  connect(_socket, &QTcpSocket::readyRead,
          this, &NTRIPConnection::_readBytes);

  connect(_socket, &QTcpSocket::errorOccurred,
          this, [this](QAbstractSocket::SocketError error){
      const bool fatal = error == QAbstractSocket::SocketAccessError
        || error == QAbstractSocket::UnsupportedSocketOperationError;
      _terminateTransport(fatal ? EndReason::Fatal : EndReason::Retryable, _socket->errorString());
  });

  connect(_socket, &QTcpSocket::disconnected,
          this, [this]() {
      _terminateTransport(EndReason::Retryable, tr("The NTRIP caster disconnected."));
  });

  connect(_vrsSendTimer, &QTimer::timeout,
          this, &NTRIPConnection::_sendNmeaGga);
}

NTRIPConnection::~NTRIPConnection()
{
  delete _rtcm_parsing;
}

// Connection to the host server
void NTRIPConnection::start()
{
  if (_closed ||
    _state != State::Disconnected ||
    _socket->state() != QAbstractSocket::UnconnectedState) {
    return;
  }
  qCDebug(NTRIPConnectionLog)
    << "Connecting to NTRIP Server: "
    << _configuration.hostAddress
    << ":" << _configuration.port;

  _state = State::Connecting;
  emit stateChanged();

  if (_closed) {
    return;
  }

  _attemptTimer->start();
  _socket->connectToHost(
    _configuration.hostAddress,
    _configuration.port);
}

void NTRIPConnection::stop()
{
  _terminateTransport(EndReason::Requested, {});
}

void NTRIPConnection::_terminateTransport(NTRIPConnection::EndReason reason, const QString& message)
{
  if (_closed) {
    return;
  }

  _closed = true;
  _attemptTimer->stop();
  _connectionDropTimer->stop();
  _vrsSendTimer->stop();
  _connectionState = NTRIPState::uninitialised;
  _socket->abort();

  if (_state != State::Disconnected) {
    _state = State::Disconnected;
    emit stateChanged();
  }

  if (_receivingData) {
    _receivingData = false;
    emit receivingDataChanged();
  }

  emit connectionEnded(reason, message);
}

void NTRIPConnection::_onTcpConnected() {
  qCDebug(NTRIPConnectionLog) << "NTRIP TCP connected";

  if (!_configuration.mountpoint.isEmpty()) {
    _state = State::RequestStream;
    emit stateChanged();

    if (_closed) {
      return;
    }
  }

  _hardwareConnect();

  if (!_closed && _configuration.enableVRS) {
    _vrsSendTimer->start();
  }
}

void NTRIPConnection::_hardwareConnect() {
  if (!_configuration.mountpoint.isEmpty()) {
    qCDebug(NTRIPConnectionLog) << "Sending HTTP request, using mount point: " << _configuration.mountpoint;

    QString auth;
    if (!_configuration.username.isEmpty() || !_configuration.password.isEmpty()) {
      const QByteArray credentials = (_configuration.username + QLatin1Char(':') + _configuration.password).toUtf8().toBase64();
      auth = QStringLiteral("Authorization: Basic %1\r\n").arg(QString::fromLatin1(credentials));
    }
    QString query;
    if (_configuration.forceNtripV1) {
      query = "GET /%1 HTTP/1.0\r\n"
              "User-Agent: NTRIP QGroundControl\r\n"
              "%2"
              "Connection: close\r\n\r\n";
      query = query.arg(_configuration.mountpoint, auth);
    } else {
      query = "GET /%1 HTTP/1.1\r\n"
              "User-Agent: NTRIP QGroundControl\r\n"
              "Ntrip-Version: Ntrip/2.0\r\n"
              "Host: %2\r\n"
              "%3"
              "Connection: close\r\n\r\n";
      QString host = _configuration.hostAddress;
      if (host.contains(QLatin1Char(':'))) {
        host = QStringLiteral("[%1]").arg(host);
      }

      const QString entity = QStringLiteral("%1:%2").arg(host, QString::number(_configuration.port));
      query = query.arg(_configuration.mountpoint, entity, auth);
    }

    _connectionState = NTRIPState::waiting_for_http_response;
    _socket->write(query.toUtf8());
  } else {
    _beginRtcmStream();
  }

  qCDebug(NTRIPConnectionLog) << "NTRIP Socket connected";
}

void NTRIPConnection::_parse(const QByteArray &buffer) {
  qCDebug(NTRIPConnectionLog) << "Parsing " << buffer.size() << " bytes";
  qCDebug(NTRIPConnectionLog) << "Buffer: " << QString(buffer);
  for (const uint8_t byte : buffer) {
    if (_closed) {
      return;
    }
    if (_connectionState == NTRIPState::waiting_for_rtcm_header) {
      if (byte != RTCM3_PREAMBLE) {
        qCDebug(NTRIPConnectionLog) << "NOT RTCM 3 preamble, ignoring byte " << byte;
        continue;
      }
      _connectionState = NTRIPState::accumulating_rtcm_packet;
    }

    if (_rtcm_parsing->addByte(byte)) {
      _attemptTimer->start(10000);
      _connectionState = NTRIPState::waiting_for_rtcm_header;
      QByteArray message((char *)_rtcm_parsing->message(),
                         static_cast<int>(_rtcm_parsing->messageLength()));
      uint16_t id = _rtcm_parsing->messageId();
      qCDebug(NTRIPConnectionLog) << "RTCM message ID " << id;
      qCDebug(NTRIPConnectionLog) << "RTCM message size " << message.size();

      if (_configuration.whitelist.empty() || _configuration.whitelist.contains(id)) {
        qCDebug(NTRIPConnectionLog) << "Sending message ID [" << id << "] of size "
                          << message.length();
        _connectionDropTimer->start();
        if (!_receivingData) {
          _receivingData = true;
          emit receivingDataChanged();
        }
        if (_closed) {
          return;
        }

        emit RTCMDataUpdate(message);
      } else {
        qCDebug(NTRIPConnectionLog) << "Ignoring " << id;
      }
      _rtcm_parsing->reset();
    }
  }
}

void NTRIPConnection::_readBytes(void) {
  qCDebug(NTRIPConnectionLog) << "Reading bytes";
  if (_closed || !_socket) {
    return;
  }

  if (_connectionState == NTRIPState::waiting_for_http_response) {
    QByteArray line;
    if (!_readHandshakeLine(line)) {
      return;
    }

    const QByteArray response = line.trimmed();
    const QList<QByteArray> fields = response.simplified().split(' ');

    qCDebug(NTRIPConnectionLog) << "Server responded with " << response;

    if (response.startsWith("SOURCETABLE")) {
      _connectionState = NTRIPState::uninitialised;
      _terminateTransport(EndReason::Fatal, tr("The caster returned source table. Check the mountpoint"));
      return;
    }

    const bool supportedResponse =
      fields.size() >= 2 && (
        fields[0] == "HTTP/1.0" ||
        fields[0] == "HTTP/1.1" ||
        fields[0] == "ICY");

    if (!supportedResponse || fields[1] != "200") {
      const int status = supportedResponse ? fields[1].toInt() : 0;
      const bool retryable = status == 408 || (status >= 500 && status <= 599);
      const QString message = status == 401
        ? tr("NTRIP Server responded with 401 Unauthorized")
        : tr("NTRIP request failed: %1").arg(QString::fromLatin1(response));

      _terminateTransport(retryable ? EndReason::Retryable : EndReason::Fatal, message);
      return;
    }
    if (fields[0] == "ICY") {
      _beginRtcmStream();
    } else {
      _connectionState = NTRIPState::consuming_http_headers;
    }
  }

  if (_connectionState == NTRIPState::consuming_http_headers) {
    QByteArray line;
    while (_readHandshakeLine(line)) {
      qCDebug(NTRIPConnectionLog) << "HTTP header: " << line.trimmed();
      const QByteArray lowerLine = line.toLower();
      if (lowerLine.contains("transfer-encoding") && lowerLine.contains("chunked")) {
        _isChunked = true;
        qCDebug(NTRIPConnectionLog) << "Server uses chunked transfer encoding";
      }
      if (line == "\r\n" || line == "\n") {
        qCDebug(NTRIPConnectionLog) << "HTTP headers consumed, starting RTCM parsing";
        _beginRtcmStream();
        break;
      }
    }
  }

  if (_closed || _connectionState == NTRIPState::consuming_http_headers) {
    return;
  }

  if (_connectionState == NTRIPState::uninitialised) {
    qCDebug(NTRIPConnectionLog) << "NTRIP State is uninitialised. Discarding bytes";
    _socket->readAll();
    return;
  }

  QByteArray bytes = _socket->readAll();
  if (_isChunked) {
    bytes = _dechunk(bytes);
  }
  qCDebug(NTRIPConnectionLog) << "Received" << bytes.size() << "bytes:" << bytes.toHex(' ');
  _parse(bytes);
}

QByteArray NTRIPConnection::_dechunk(const QByteArray &data) {
  QByteArray result;
  for (int i = 0; i < data.size(); i++) {
    uint8_t byte = data[i];
    switch (_chunkState) {
      case ChunkState::ReadingSize:
        if (byte == '\r') break;
        if (byte == '\n') {
          bool ok;
          int size = _chunkSizeBuffer.toInt(&ok, 16);
          _chunkSizeBuffer.clear();
          if (!ok || size == 0) return result;
          _chunkBytesLeft = size;
          _chunkState = ChunkState::ReadingData;
        } else {
          _chunkSizeBuffer.append(static_cast<char>(byte));
        }
        break;
      case ChunkState::ReadingData:
        result.append(static_cast<char>(byte));
        if (--_chunkBytesLeft == 0) {
          _chunkState = ChunkState::ReadingTrailer;
        }
        break;
      case ChunkState::ReadingTrailer:
        if (byte == '\n') {
          _chunkState = ChunkState::ReadingSize;
        }
        break;
    }
  }
  return result;
}

void NTRIPConnection::_sendNmeaGga() {
  qCDebug(NTRIPConnectionLog) << "Sending NMEA GGA";

  if (!_toolbox->multiVehicleManager()->activeVehicleAvailable()) {
    qCDebug(NTRIPConnectionLog) << "No active vehicle";
    return;
  }
  qCDebug(NTRIPConnectionLog) << "Active vehicle found. Using vehicle position";

  if (_toolbox->multiVehicleManager()->vehicles()->count() > 1) {
    // TODO(bzd): Consider how to handle multiple vehicles - best option would be
    // send separate RTCM messages for each vehicle or at least
    // calculate the average position of all vehicles and use one RTCM for all
    qCDebug(NTRIPConnectionLog) << "More than one vehicle found. Using active vehicle for correction";
  }

  Vehicle *vehicle = _toolbox->multiVehicleManager()->activeVehicle();
  NmeaMessage nmeaMessage(vehicle->coordinate());
  // Write NMEA message
  if (_socket) {
    const QString &gga = nmeaMessage.getGGA();
    _socket->write(gga.toUtf8());
    qCDebug(NTRIPConnectionLog) << "NMEA GGA Message : " << gga;
  }
}

void NTRIPConnection::_beginRtcmStream() {
  // Give the caster time to send the first RTCM frame.
  _attemptTimer->start(30000);
  _connectionState = NTRIPState::waiting_for_rtcm_header;
  _state = State::Connected;
  emit stateChanged();
}

bool NTRIPConnection::_readHandshakeLine(QByteArray& line)
{
  constexpr qint64 maxHandshakeBytes = 16 * 1024;
  const qint64 remaining = maxHandshakeBytes - _handshakeBytesRead;

  if (remaining > 0 && _socket->canReadLine()) {
      const qint64 bytesToRead = qMin(remaining, _socket->bytesAvailable());
      line = _socket->readLine(bytesToRead + 1);
      if (line.endsWith('\n')) {
          _handshakeBytesRead += line.size();
          return true;
      }
  } else if (_socket->bytesAvailable() < remaining) {
      return false;
  }

  _terminateTransport(
    EndReason::Fatal,
    tr("The NTRIP caster response header is too large."));
  return false;
}
