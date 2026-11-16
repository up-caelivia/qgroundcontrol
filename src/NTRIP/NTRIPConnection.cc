#include "NTRIPConnection.h"
#include "NmeaMessage.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "QGCToolbox.h"

QGC_LOGGING_CATEGORY(NTRIPConnectionLog, "NTRIPConnectionLog")

NTRIPConnection::NTRIPConnection(const QString &hostAddress, int port,
                           const QString &username, const QString &password,
                           const QString &mountpoint, const QString &whitelist,
                           const bool &enableVRS)
    : QThread(), _hostAddress(hostAddress), _port(port), _username(username),
      _password(password), _mountpoint(mountpoint), _isVRSEnable(enableVRS),
      _toolbox(qgcApp()->toolbox()) {
  for (const auto &msg : whitelist.split(',')) {
    int msg_int = msg.toInt();
    if (msg_int) {
      _whitelist.insert(msg_int);
    }
  }
  qCDebug(NTRIPConnectionLog) << "whitelist: " << _whitelist;
  if (!_rtcm_parsing) {
    _rtcm_parsing = new RTCMParsing();
  }
  _rtcm_parsing->reset();
  _state = NTRIPState::uninitialised;

  // Start TCP Socket
  moveToThread(this);
  start();
}

NTRIPConnection::~NTRIPConnection(void) {
  qCDebug(NTRIPConnectionLog) << "NTRIP Thread stopped";
  if (_socket) {
    if (_isVRSEnable) {
      _vrsSendTimer->stop();
      QObject::disconnect(_vrsSendTimer, &QTimer::timeout, this,
                          &NTRIPConnection::_sendNmeaGga);
      delete _vrsSendTimer;
      _vrsSendTimer = nullptr;
    }
    QObject::disconnect(_socket, &QTcpSocket::readyRead, this,
                        &NTRIPConnection::_readBytes);
    _socket->disconnectFromHost();
    _socket->deleteLater();
    _socket = nullptr;

    // Delete Rtcm Parsing instance
    delete (_rtcm_parsing);
    _rtcm_parsing = nullptr;
  }
  quit();
  wait();
}

void NTRIPConnection::run(void) {
  qCDebug(NTRIPConnectionLog) << "NTRIP Thread started";
  _hardwareConnect();

  // Init VRS Timer
  if (_isVRSEnable) {
    _vrsSendTimer = new QTimer();
    _vrsSendTimer->setInterval(_vrsSendRateMSecs);
    _vrsSendTimer->setSingleShot(false);
    QObject::connect(_vrsSendTimer, &QTimer::timeout, this,
                     &NTRIPConnection::_sendNmeaGga);
    _vrsSendTimer->start();
  }

  exec();
}

void NTRIPConnection::_hardwareConnect() {
  qCDebug(NTRIPConnectionLog) << "Connecting to NTRIP Server: " << _hostAddress << ":"
                    << _port;
  _socket = new QTcpSocket();
  QObject::connect(_socket, &QTcpSocket::readyRead, this,
                   &NTRIPConnection::_readBytes);
  _socket->connectToHost(_hostAddress, static_cast<quint16>(_port));

  // Give the socket a second to connect to the other side otherwise error out
  if (!_socket->waitForConnected(1000)) {
    qCDebug(NTRIPConnectionLog) << "NTRIP Socket failed to connect";
    emit error(_socket->errorString());
    delete _socket;
    _socket = nullptr;
    return;
  }

  // If mountpoint is specified, send an http get request for data
  if (!_mountpoint.isEmpty()) {
    qCDebug(NTRIPConnectionLog) << "Sending HTTP request, using mount point: "
                      << _mountpoint;

    QString digest = QString(_username + ":" + _password).toUtf8().toBase64();
    QString auth = QString("Authorization: Basic %1\r\n").arg(digest);
    QString query;
    if (_ntripForceV1) {
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

    _socket->write(query.arg(_mountpoint).arg(auth).toUtf8());
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

      if (_whitelist.empty() || _whitelist.contains(id)) {
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
    QString line = _socket->readLine();
    qCDebug(NTRIPConnectionLog) << "Server responded with " << line;
    if (line.contains("200")) {
      if (line.contains("SOURCETABLE")) {
        qCDebug(NTRIPConnectionLog) << "Server responded with SOURCETABLE, not supported";
        emit error("NTRIP Server responded with SOURCETABLE. Bad mountpoint?");
        _state = NTRIPState::uninitialised;
      } else {
        // Got 200 OK, now consume remaining HTTP headers until empty line
        _state = NTRIPState::consuming_http_headers;
      }
    } else if (line.contains("401")) {
      qCWarning(NTRIPConnectionLog) << "Server responded with " << line;
      emit error("NTRIP Server responded with 401 Unauthorized");
      _state = NTRIPState::uninitialised;
    } else {
      qCWarning(NTRIPConnectionLog) << "Server responded with " << line;
      _state = NTRIPState::consuming_http_headers;
    }
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
