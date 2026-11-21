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

  _handshakeTimer = new QTimer(this);
  _handshakeTimer->setSingleShot(true);
  _handshakeTimer->setInterval(10000);
  connect(
    _handshakeTimer,
    &QTimer::timeout,
    this,
    [this]() {
      emit error(tr("Timed out waiting for the NTRIP caster."));
      stop();
    }
  );

  connect(_socket, &QTcpSocket::connected,
          this, &NTRIPConnection::_onTcpConnected);

  connect(_socket, &QTcpSocket::readyRead,
          this, &NTRIPConnection::_readBytes);

  connect(_socket, &QTcpSocket::errorOccurred,
          this, [this](QAbstractSocket::SocketError){
      emit error(_socket->errorString());
      stop();
  });

  connect(_socket, &QTcpSocket::disconnected,
          this, &NTRIPConnection::stop);

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

  _handshakeTimer->start();
  _socket->connectToHost(
    _configuration.hostAddress,
    _configuration.port);
}

// Stops the connection to the host server
void NTRIPConnection::stop()
{
  if (_closed) {
    return;
  }

  _closed = true;
  _handshakeTimer->stop();
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

  emit closed();
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

    QString digest = QString(_configuration.username + ":" + _configuration.password).toUtf8().toBase64();
    QString auth = QString("Authorization: Basic %1\r\n").arg(digest);
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
      _connectionState = NTRIPState::waiting_for_rtcm_header;
      QByteArray message((char *)_rtcm_parsing->message(),
                         static_cast<int>(_rtcm_parsing->messageLength()));
      uint16_t id = _rtcm_parsing->messageId();
      qCDebug(NTRIPConnectionLog) << "RTCM message ID " << id;
      qCDebug(NTRIPConnectionLog) << "RTCM message size " << message.size();

      if (_configuration.whitelist.empty() || _configuration.whitelist.contains(id)) {
        qCDebug(NTRIPConnectionLog) << "Sending message ID [" << id << "] of size "
                          << message.length();
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
      emit error(tr("The caster returned a source table. Check the mountpoint."));
      stop();
      return;
    }

    const bool supportedResponse =
      fields.size() >= 2 && (
        fields[0] == "HTTP/1.0" ||
        fields[0] == "HTTP/1.1" ||
        fields[0] == "ICY");

    if (!supportedResponse || fields[1] != "200") {
      _connectionState = NTRIPState::uninitialised;

      if (supportedResponse && fields[1] == "401") {
        emit error(tr("NTRIP Server responded with 401 Unauthorized"));
      } else {
        emit error(tr("NTRIP request failed: %1").arg(QString::fromLatin1(response)));
      }
      stop();
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
  _handshakeTimer->stop();
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

  emit error(tr("The NTRIP caster response header is too large."));
  stop();
  return false;
}
