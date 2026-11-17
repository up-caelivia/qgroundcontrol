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
  _state = NTRIPState::uninitialised;

  _socket = new QTcpSocket(this);

  _vrsSendTimer = new QTimer(this);
  _vrsSendTimer->setInterval(_vrsSendRateMSecs);
  _vrsSendTimer->setSingleShot(false);

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
  if (_closed || _socket->state() != QAbstractSocket::UnconnectedState) {
    return;
  }
  qCDebug(NTRIPConnectionLog)
    << "Connecting to NTRIP Server: "
    << _configuration.hostAddress
    << ":" << _configuration.port;

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
  _vrsSendTimer->stop();
  _state = NTRIPState::uninitialised;
  _socket->abort();
  emit closed();
}

void NTRIPConnection::_onTcpConnected() {
  qCDebug(NTRIPConnectionLog) << "NTRIP TCP connected";

  _hardwareConnect();

  if (_configuration.enableVRS) {
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
    } else {
      query = "GET /%1 HTTP/1.1\r\n"
              "User-Agent: NTRIP QGroundControl\r\n"
              "Ntrip-Version: Ntrip/2.0\r\n"
              "%2"
              "Connection: close\r\n\r\n";
    }

    _socket->write(query.arg(_configuration.mountpoint).arg(auth).toUtf8());
    _state = NTRIPState::waiting_for_http_response;
  } else {
    // If no mountpoint is set, assume we will just get data from the tcp stream
    _state = NTRIPState::waiting_for_rtcm_header;
  }

  qCDebug(NTRIPConnectionLog) << "NTRIP Socket connected";
}

void NTRIPConnection::_parse(const QByteArray &buffer) {
  qCDebug(NTRIPConnectionLog) << "Parsing " << buffer.size() << " bytes";
  qCDebug(NTRIPConnectionLog) << "Buffer: " << QString(buffer);
  for (const uint8_t byte : buffer) {
    if (_state == NTRIPState::waiting_for_rtcm_header) {
      if (byte != RTCM3_PREAMBLE) {
        qCDebug(NTRIPConnectionLog) << "NOT RTCM 3 preamble, ignoring byte " << byte;
        continue;
      }
      _state = NTRIPState::accumulating_rtcm_packet;
    }

    if (_rtcm_parsing->addByte(byte)) {
      _state = NTRIPState::waiting_for_rtcm_header;
      QByteArray message((char *)_rtcm_parsing->message(),
                         static_cast<int>(_rtcm_parsing->messageLength()));
      uint16_t id = _rtcm_parsing->messageId();
      qCDebug(NTRIPConnectionLog) << "RTCM message ID " << id;
      qCDebug(NTRIPConnectionLog) << "RTCM message size " << message.size();

      if (_configuration.whitelist.empty() || _configuration.whitelist.contains(id)) {
        qCDebug(NTRIPConnectionLog) << "Sending message ID [" << id << "] of size "
                          << message.length();
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
  if (!_socket) {
    return;
  }
  if (_state == NTRIPState::waiting_for_http_response) {
    if (!_socket->canReadLine()) {
      return;
    }

    const QByteArray response = _socket->readLine().trimmed();
    const QList<QByteArray> fields = response.simplified().split(' ');

    qCDebug(NTRIPConnectionLog) << "Server responded with " << response;

    if (response.startsWith("SOURCETABLE")) {
      _state = NTRIPState::uninitialised;
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
      _state = NTRIPState::uninitialised;

      if (supportedResponse && fields[1] == "401") {
        emit error(tr("NTRIP Server responded with 401 Unauthorized"));
      } else {
        emit error(tr("NTRIP request failed: %1").arg(QString::fromLatin1(response)));
      }
      stop();
      return;
    }
    _state = NTRIPState::consuming_http_headers;
  }

  if (_state == NTRIPState::consuming_http_headers) {
    while (_socket->canReadLine()) {
      QString line = _socket->readLine();
      qCDebug(NTRIPConnectionLog) << "HTTP header: " << line.trimmed();
      if (line.toLower().contains("transfer-encoding") && line.toLower().contains("chunked")) {
        _isChunked = true;
        qCDebug(NTRIPConnectionLog) << "Server uses chunked transfer encoding";
      }
      if (line == "\r\n" || line == "\n") {
        // Empty line signals end of HTTP headers, RTCM data starts now
        qCDebug(NTRIPConnectionLog) << "HTTP headers consumed, starting RTCM parsing";
        _state = NTRIPState::waiting_for_rtcm_header;
        break;
      }
    }
  }

  if (_state == NTRIPState::consuming_http_headers) {
    return;
  }

  if (_state == NTRIPState::uninitialised) {
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
