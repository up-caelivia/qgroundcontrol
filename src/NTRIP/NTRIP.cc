/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "NTRIP.h"
#include "NTRIPSettings.h"
#include "NmeaMessage.h"
#include "PositionManager.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "QGCToolbox.h"
#include "SettingsManager.h"

QGC_LOGGING_CATEGORY(NTRIPLog, "NTRIPLog")

NTRIP::NTRIP(QGCApplication *app, QGCToolbox *toolbox)
    : QGCTool(app, toolbox) {}

void NTRIP::setToolbox(QGCToolbox *toolbox) {
  QGCTool::setToolbox(toolbox);

  NTRIPSettings *settings =
      qgcApp()->toolbox()->settingsManager()->ntripSettings();
  if (settings->ntripServerConnectEnabled()->rawValue().toBool()) {
    qCDebug(NTRIPLog) << settings->ntripEnableVRS()->rawValue().toBool();
    _rtcmMavlink = new RTCMMavlink(*toolbox);

    _tcpLink = new NTRIPTCPLink(
        settings->ntripServerHostAddress()->rawValue().toString(),
        settings->ntripServerPort()->rawValue().toInt(),
        settings->ntripUsername()->rawValue().toString(),
        settings->ntripPassword()->rawValue().toString(),
        settings->ntripMountpoint()->rawValue().toString(),
        settings->ntripWhitelist()->rawValue().toString(),
        settings->ntripEnableVRS()->rawValue().toBool());
    connect(_tcpLink, &NTRIPTCPLink::error, this, &NTRIP::_tcpError,
            Qt::QueuedConnection);
    connect(_tcpLink, &NTRIPTCPLink::RTCMDataUpdate, _rtcmMavlink,
            &RTCMMavlink::RTCMDataUpdate);
  } else {
    qCDebug(NTRIPLog) << "NTRIP Server is not enabled";
  }
}

void NTRIP::_tcpError(const QString errorMsg) {
  qgcApp()->showAppMessage(tr("NTRIP Server Error: %1").arg(errorMsg));
}

NTRIPTCPLink::NTRIPTCPLink(const QString &hostAddress, int port,
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
  qCDebug(NTRIPLog) << "whitelist: " << _whitelist;
  if (!_rtcm_parsing) {
    _rtcm_parsing = new RTCMParsing();
  }
  _rtcm_parsing->reset();
  _state = NTRIPState::uninitialised;

  // Start TCP Socket
  moveToThread(this);
  start();
}

NTRIPTCPLink::~NTRIPTCPLink(void) {
  qCDebug(NTRIPLog) << "NTRIP Thread stopped";
  if (_socket) {
    if (_isVRSEnable) {
      _vrsSendTimer->stop();
      QObject::disconnect(_vrsSendTimer, &QTimer::timeout, this,
                          &NTRIPTCPLink::_sendNmeaGga);
      delete _vrsSendTimer;
      _vrsSendTimer = nullptr;
    }
    QObject::disconnect(_socket, &QTcpSocket::readyRead, this,
                        &NTRIPTCPLink::_readBytes);
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

void NTRIPTCPLink::run(void) {
  qCDebug(NTRIPLog) << "NTRIP Thread started";
  _hardwareConnect();

  // Init VRS Timer
  if (_isVRSEnable) {
    _vrsSendTimer = new QTimer();
    _vrsSendTimer->setInterval(_vrsSendRateMSecs);
    _vrsSendTimer->setSingleShot(false);
    QObject::connect(_vrsSendTimer, &QTimer::timeout, this,
                     &NTRIPTCPLink::_sendNmeaGga);
    _vrsSendTimer->start();
  }

  exec();
}

void NTRIPTCPLink::_hardwareConnect() {
  qCDebug(NTRIPLog) << "Connecting to NTRIP Server: " << _hostAddress << ":"
                    << _port;
  _socket = new QTcpSocket();
  QObject::connect(_socket, &QTcpSocket::readyRead, this,
                   &NTRIPTCPLink::_readBytes);
  _socket->connectToHost(_hostAddress, static_cast<quint16>(_port));

  // Give the socket a second to connect to the other side otherwise error out
  if (!_socket->waitForConnected(1000)) {
    qCDebug(NTRIPLog) << "NTRIP Socket failed to connect";
    emit error(_socket->errorString());
    delete _socket;
    _socket = nullptr;
    return;
  }

  // If mountpoint is specified, send an http get request for data
  if (!_mountpoint.isEmpty()) {
    qCDebug(NTRIPLog) << "Sending HTTP request, using mount point: "
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

  qCDebug(NTRIPLog) << "NTRIP Socket connected";
}

void NTRIPTCPLink::_parse(const QByteArray &buffer) {
  qCDebug(NTRIPLog) << "Parsing " << buffer.size() << " bytes";
  qCDebug(NTRIPLog) << "Buffer: " << QString(buffer);
  for (const uint8_t byte : buffer) {
    if (_state == NTRIPState::waiting_for_rtcm_header) {
      if (byte != RTCM3_PREAMBLE) {
        qCDebug(NTRIPLog) << "NOT RTCM 3 preamble, ignoring byte " << byte;
        continue;
      }
      _state = NTRIPState::accumulating_rtcm_packet;
    }

    if (_rtcm_parsing->addByte(byte)) {
      _state = NTRIPState::waiting_for_rtcm_header;
      QByteArray message((char *)_rtcm_parsing->message(),
                         static_cast<int>(_rtcm_parsing->messageLength()));
      uint16_t id = _rtcm_parsing->messageId();
      qCDebug(NTRIPLog) << "RTCM message ID " << id;
      qCDebug(NTRIPLog) << "RTCM message size " << message.size();

      if (_whitelist.empty() || _whitelist.contains(id)) {
        qCDebug(NTRIPLog) << "Sending message ID [" << id << "] of size "
                          << message.length();
        emit RTCMDataUpdate(message);
      } else {
        qCDebug(NTRIPLog) << "Ignoring " << id;
      }
      _rtcm_parsing->reset();
    }
  }
}

void NTRIPTCPLink::_readBytes(void) {
  qCDebug(NTRIPLog) << "Reading bytes";
  if (!_socket) {
    return;
  }
  if (_state == NTRIPState::waiting_for_http_response) {
    QString line = _socket->readLine();
    qCDebug(NTRIPLog) << "Server responded with " << line;
    if (line.contains("200")) {
      if (line.contains("SOURCETABLE")) {
        qCDebug(NTRIPLog) << "Server responded with SOURCETABLE, not supported";
        emit error("NTRIP Server responded with SOURCETABLE. Bad mountpoint?");
        _state = NTRIPState::uninitialised;
      } else {
        // Got 200 OK, now consume remaining HTTP headers until empty line
        _state = NTRIPState::consuming_http_headers;
      }
    } else if (line.contains("401")) {
      qCWarning(NTRIPLog) << "Server responded with " << line;
      emit error("NTRIP Server responded with 401 Unauthorized");
      _state = NTRIPState::uninitialised;
    } else {
      qCWarning(NTRIPLog) << "Server responded with " << line;
      _state = NTRIPState::consuming_http_headers;
    }
  }

  if (_state == NTRIPState::consuming_http_headers) {
    while (_socket->canReadLine()) {
      QString line = _socket->readLine();
      qCDebug(NTRIPLog) << "HTTP header: " << line.trimmed();
      if (line.toLower().contains("transfer-encoding") && line.toLower().contains("chunked")) {
        _isChunked = true;
        qCDebug(NTRIPLog) << "Server uses chunked transfer encoding";
      }
      if (line == "\r\n" || line == "\n") {
        // Empty line signals end of HTTP headers, RTCM data starts now
        qCDebug(NTRIPLog) << "HTTP headers consumed, starting RTCM parsing";
        _state = NTRIPState::waiting_for_rtcm_header;
        break;
      }
    }
  }

  if (_state == NTRIPState::uninitialised) {
    qCDebug(NTRIPLog) << "NTRIP State is uninitialised. Discarding bytes";
    _socket->readAll();
    return;
  }

  QByteArray bytes = _socket->readAll();
  if (_isChunked) {
    bytes = _dechunk(bytes);
  }
  qCDebug(NTRIPLog) << "Received" << bytes.size() << "bytes:" << bytes.toHex(' ');
  _parse(bytes);
}

QByteArray NTRIPTCPLink::_dechunk(const QByteArray &data) {
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

void NTRIPTCPLink::_sendNmeaGga() {
  qCDebug(NTRIPLog) << "Sending NMEA GGA";

  if (!_toolbox->multiVehicleManager()->activeVehicleAvailable()) {
    qCDebug(NTRIPLog) << "No active vehicle";
    return;
  }
  qCDebug(NTRIPLog) << "Active vehicle found. Using vehicle position";

  if (_toolbox->multiVehicleManager()->vehicles()->count() > 1) {
    // TODO(bzd): Consider how to handle multiple vehicles - best option would be
    // send separate RTCM messages for each vehicle or at least
    // calculate the average position of all vehicles and use one RTCM for all
    qCDebug(NTRIPLog) << "More than one vehicle found. Using active vehicle for correction";
  }

  Vehicle *vehicle = _toolbox->multiVehicleManager()->activeVehicle();
  NmeaMessage nmeaMessage(vehicle->coordinate());
  // Write NMEA message
  if (_socket) {
    const QString &gga = nmeaMessage.getGGA();
    _socket->write(gga.toUtf8());
    qCDebug(NTRIPLog) << "NMEA GGA Message : " << gga;
  }
}
