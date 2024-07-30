/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "ntrip.h"
#include "QGCLoggingCategory.h"
#include "QGCToolbox.h"
#include "QGCApplication.h"
#include "SettingsManager.h"
#include "NTRIPSettings.h"
#include <QByteArray>


#define RTCM3_PREAMBLE 0xD3
#define MSG_TYPE_1006 1006
#define MSG_TYPE_1005 1005

static int parse_num_satellites(const QByteArray &message, int message_type);

NTRIP::NTRIP(QGCApplication* app, QGCToolbox* toolbox)
    : QGCTool(app, toolbox)
{
}

void NTRIP::setToolbox(QGCToolbox* toolbox)
{
    QGCTool::setToolbox(toolbox);

    NTRIPSettings* settings = qgcApp()->toolbox()->settingsManager()->ntripSettings();
    if (settings->ntripServerConnectEnabled()->rawValue().toBool()) {
        qCDebug(NTRIPLog) << settings->ntripEnableVRS()->rawValue().toBool();
        _rtcmMavlink = new RTCMMavlink(*toolbox);        

        _tcpLink = new NTRIPTCPLink(settings->ntripServerHostAddress()->rawValue().toString(),
                                    settings->ntripServerPort()->rawValue().toInt(),
                                    settings->ntripUsername()->rawValue().toString(),
                                    settings->ntripPassword()->rawValue().toString(),
                                    settings->ntripMountpoint()->rawValue().toString(),
                                    settings->ntripWhitelist()->rawValue().toString(),
                                    settings->ntripEnableVRS()->rawValue().toBool());
        connect(_tcpLink, &NTRIPTCPLink::error,              this, &NTRIP::_tcpError,           Qt::QueuedConnection);
        connect(_tcpLink, &NTRIPTCPLink::RTCMDataUpdate,   _rtcmMavlink, &RTCMMavlink::RTCMDataUpdate);
    }
}


void NTRIP::_tcpError(const QString errorMsg)
{
    qgcApp()->showAppMessage(tr("NTRIP Server Error: %1").arg(errorMsg));
}


NTRIPTCPLink::NTRIPTCPLink(const QString& hostAddress,
                           int port,
                           const QString &username,
                           const QString &password,
                           const QString &mountpoint,
                           const QString &whitelist,
                           const bool    &enableVRS)
    : QThread       ()
    , _hostAddress  (hostAddress)
    , _port         (port)
    , _username     (username)
    , _password     (password)
    , _mountpoint   (mountpoint)
    , _isVRSEnable  (enableVRS)
    , _toolbox      (qgcApp()->toolbox())
{
    for(const auto& msg: whitelist.split(',')) {
        int msg_int = msg.toInt();
        if(msg_int) {
            _whitelist.insert(msg_int);
        }
    }

    constants->setNtripEnabled(true);


   // qDebug() << "AAAAAAA: " << _whitelist;


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

NTRIPTCPLink::~NTRIPTCPLink(void)
{
    if (_socket) {
        if(_isVRSEnable && _vrsSendTimer) {
            _vrsSendTimer->stop();
            QObject::disconnect(_vrsSendTimer, &QTimer::timeout, this, &NTRIPTCPLink::_sendNMEA);
            delete _vrsSendTimer;
            _vrsSendTimer = nullptr;
        }


        _requestTimer->stop();
        QObject::disconnect(_requestTimer, &QTimer::timeout, this, &NTRIPTCPLink::_hardwareConnect);
        delete _requestTimer;
        _requestTimer = nullptr;

        QObject::disconnect(_socket, &QTcpSocket::readyRead, this, &NTRIPTCPLink::_readBytes);
        _socket->disconnectFromHost();
        _socket->deleteLater();
        _socket = nullptr;

        // Delete Rtcm Parsing instance
        delete(_rtcm_parsing);
        _rtcm_parsing = nullptr;
    }

    quit();
    wait();
}

void NTRIPTCPLink::run(void)
{

    _requestTimer = new QTimer();
    _requestTimer->setInterval(_reqSendRateMSecs);
    _requestTimer->setSingleShot(false);
    QObject::connect(_requestTimer, &QTimer::timeout, this, &NTRIPTCPLink::_hardwareConnect);
    _requestTimer->start();

    //_hardwareConnect();
    exec();
}


void NTRIPTCPLink::startTimer(void) {

    // Init VRS Timer
    if(_isVRSEnable) {
        _vrsSendTimer = new QTimer();
        _vrsSendTimer->setInterval(_vrsSendRateMSecs);
        _vrsSendTimer->setSingleShot(false);
        QObject::connect(_vrsSendTimer, &QTimer::timeout, this, &NTRIPTCPLink::_sendNMEA);
        _vrsSendTimer->start();
    }
}



void NTRIPTCPLink::_hardwareConnect()
{

   // qDebug() << "start timer";

    _socket = new QTcpSocket();
    QObject::connect(_socket, &QTcpSocket::readyRead, this, &NTRIPTCPLink::_readBytes);
    _socket->connectToHost(_hostAddress, static_cast<quint16>(_port));

    // Give the socket a second to connect to the other side otherwise error out
    if (!_socket->waitForConnected(2000)) {
        qCDebug(NTRIPLog) << "NTRIP Socket failed to connect";

        constants->setNtripReceiving(false);
        qDebug() << "Socket failed to connect";

        //emit error(_socket->errorString());
        QObject::disconnect(_socket, &QTcpSocket::readyRead, this, &NTRIPTCPLink::_readBytes);
        delete _socket;
        _socket = nullptr;

        return;
    }

    // If mountpoint is specified, send an http get request for data
    if ( !_mountpoint.isEmpty()) {
        qCDebug(NTRIPLog) << "Sending HTTP request";
        QString auth = QString(_username + ":"  + _password).toUtf8().toBase64();
        QString query = "GET /%1 HTTP/1.0\r\nUser-Agent: NTRIP\r\nAuthorization: Basic %2\r\n\r\n";
        _socket->write(query.arg(_mountpoint).arg(auth).toUtf8());
        _state = NTRIPState::waiting_for_http_response;
    } else {
        // If no mountpoint is set, assume we will just get data from the tcp stream
        _state = NTRIPState::waiting_for_rtcm_header;
    }

    qCDebug(NTRIPLog) << "NTRIP Socket connected";
}

void NTRIPTCPLink::_parse(const QByteArray &buffer)
{

    // qDebug() << "AAAAAA: " << buffer;
    for(const uint8_t byte : buffer) {
        if(_state == NTRIPState::waiting_for_rtcm_header) {
            if(byte != RTCM3_PREAMBLE)
                continue;
            _state = NTRIPState::accumulating_rtcm_packet;
        }
        if(_rtcm_parsing->addByte(byte)) {

            constants->setNtripReceiving(true);

            _state = NTRIPState::waiting_for_rtcm_header;
            QByteArray message((char*)_rtcm_parsing->message(), static_cast<int>(_rtcm_parsing->messageLength()));
            uint16_t id = ((uint8_t)message[3] << 4) | ((uint8_t)message[4] >> 4);
            // qDebug() << "AAAA:" << id;

            if(_whitelist.empty() || _whitelist.contains(id)) {
                emit RTCMDataUpdate(message);
                qCDebug(NTRIPLog) << "Sending " << id << "of size " << message.length();
                constants->setnumM(0);

                if(id == 1005 || id == 1006)
                    decode_type1005_1006(message);

                int sat = -1;
                if(id == 1004 || id == 1012 || id == 1094 || id == 1124)
                    sat = parse_num_satellites(message, id);

                if(id == 1004 && sat != -1)
                    constants->setnumGPS(sat);

                if(id == 1012 && sat != -1)
                    constants->setnumGLO(sat);

            } else {
                qCDebug(NTRIPLog) << "Ignoring " << id;
            }
            _rtcm_parsing->reset();
        }
    }
}

void NTRIPTCPLink::_readBytes(void)
{
    if (!_socket) {
        return;
    }

    //reset timer
    _requestTimer->stop();
    _requestTimer->start(_reqSendRateMSecs);


    if(_state == NTRIPState::waiting_for_http_response) {
        QString line = _socket->readLine();

        if (line.contains("ICY 200")){
            _state = NTRIPState::waiting_for_rtcm_header;

            constants->setauthError(false);
            constants->setmountError(false);
            startTimer();

        } else {
            qCWarning(NTRIPLog) << "Server responded with " << line;
            // qgcApp()->showAppMessage("Unable to start NTRIP");

            if(line.contains("401"))
                constants->setauthError(true);

            if(line.contains("SOURCETABLE 200") || line.contains("Server: "))
                constants->setmountError(true);

            return;

            // TODO: Handle failure. Reconnect?
            // Just move into parsing mode and hope for now.
            //_state = NTRIPState::waiting_for_rtcm_header;
        }
    }
    QByteArray bytes = _socket->readAll();
    _parse(bytes);
}

void NTRIPTCPLink::_sendNMEA() {

    Vehicle* _activeVehicle = _toolbox->multiVehicleManager()->activeVehicle();

    if(_activeVehicle == nullptr)
        return;

    #ifdef QT_DEBUG  // coordinate of ISMEC by google maps
        double lat = 45.277432; //gcsPosition.latitude();
        double lng = 11.679657; //gcsPosition.longitude();
        double alt = 20; //gcsPosition.altitude();
    #else

        QGeoCoordinate gcsPosition =_activeVehicle->coordinate();            //qgcPositionManager()->gcsPosition();

        if(!gcsPosition.isValid() || _activeVehicle->gpsFactGroup()->getFact("count")->rawValue().value<int>() < 6
            || _activeVehicle->gpsFactGroup()->getFact("lock")->rawValue().value<int>() < 3 ) {
            return;
        }
        double lat = gcsPosition.latitude();
        double lng = gcsPosition.longitude();
        double alt = gcsPosition.altitude();

    #endif


    qCDebug(NTRIPLog) << "lat : " << lat << " lon : " << lng << " alt : " << alt;
    QString time = QDateTime::currentDateTimeUtc().toString("hhmmss.zzz");

    if(lat != 0 || lng != 0) {
        double latdms = (int) lat + (lat - (int) lat) * .6f;
        double lngdms = (int) lng + (lng - (int) lng) * .6f;
        if(isnan(alt)) alt = 0.0;

        QString line = QString("$GP%1,%2,%3,%4,%5,%6,%7,%8,%9,%10,%11,%12,%13,%14,%15")
                           .arg("GGA", time,
                                QString::number(qFabs(latdms * 100), 'f', 2), lat < 0 ? "S" : "N",
                                QString::number(qFabs(lngdms * 100), 'f', 2), lng < 0 ? "W" : "E",
                                "1", "10", "1",
                                QString::number(alt, 'f', 2),
                                "M", "0", "M", "0.0", "0");

        // Calculrate checksum and send message
        QString checkSum = _getCheckSum(line);
        QString* nmeaMessage = new QString(line + "*" + checkSum + "\r\n");

        // Write nmea message
        if(_socket) {
            _socket->write(nmeaMessage->toUtf8());

            //qDebug() << "AAAAAA:" << nmeaMessage->toUtf8();
        }

        qCDebug(NTRIPLog) << "NMEA Message : " << nmeaMessage->toUtf8();

        delete nmeaMessage;
    }
}

QString NTRIPTCPLink::_getCheckSum(QString line) {
    QByteArray temp_Byte = line.toUtf8();
    const char* buf = temp_Byte.constData();

    char character;
    int checksum = 0;

    for(int i = 0; i < line.length(); i++) {
        character = buf[i];
        switch(character) {
        case '$':
            // Ignore the dollar sign
            break;
        case '*':
            // Stop processing before the asterisk
            i = line.length();
            continue;
        default:
            // First value for the checksum
            if(checksum == 0) {
                // Set the checksum to the value
                checksum = character;
            }
            else {
                // XOR the checksum with this character's value
                checksum = checksum ^ character;
            }
        }
    }

    return QString("%1").arg(checksum, 0, 16);
}



// --------------------------------------------------------------


static uint32_t getbitu(const QByteArray &buff, int pos, int len) {
    uint32_t bits = 0;
    for (int i = pos; i < pos + len; i++) {
        if (i / 8 >= buff.size()) {
            qWarning() << "Buffer overrun in getbitu";
            return 0;
        }
        bits = (bits << 1) | ((buff[i / 8] >> (7 - i % 8)) & 1u);
    }
    return bits;
}

static int32_t getbits(const QByteArray &buff, int pos, int len) {
    int32_t bits = getbitu(buff, pos, len);
    if (len <= 0 || len >= 32 || !(bits & (1u << (len - 1)))) {
        return bits;
    }
    return bits | (~0u << len); // Extend sign
}

static int64_t getbits_38(const QByteArray &buff, int pos) {
    return (int64_t)getbits(buff, pos, 32) * 64 + getbitu(buff, pos + 32, 6);
}

static LatLongAlt convertECEFToLatLonAlt(double X, double Y, double Z) {
    LatLongAlt result = {0, 0, 0, 0, 0, 0, true};
    const double a = 6378137.0; // WGS-84 semi-major axis
    const double f = 1.0 / 298.257223563; // WGS-84 flattening
    const double b = a * (1 - f); // semi-minor axis
    const double e2 = f * (2 - f); // first eccentricity squared
    const double ep2 = (a * a - b * b) / (b * b); // second eccentricity squared

    double p = sqrt(X * X + Y * Y);
    double theta = atan2(Z * a, p * b);

    result.longitude = atan2(Y, X);
    result.latitude = atan2(Z + ep2 * b * pow(sin(theta), 3), p - e2 * a * pow(cos(theta), 3));

    double N = a / sqrt(1 - e2 * pow(sin(result.latitude), 2));
    result.altitude = p / cos(result.latitude) - N;

    // Convert to degrees
    result.latitude *= 180.0 / M_PI;
    result.longitude *= 180.0 / M_PI;

    return result;
}

LatLongAlt NTRIPTCPLink::decode_type1005_1006(const QByteArray &data) {
    LatLongAlt result = {0, 0, 0, 0, 0, 0, false};
    if (data.size() < 6 || static_cast<uint8_t>(data[0]) != RTCM3_PREAMBLE) {
        qWarning() << "Invalid RTCM3 message";
        return result;
    }

    int i = 24 + 12;
    int staid;
    double rr[3], anth = 0;

    uint16_t length = ((static_cast<uint8_t>(data[1]) & 0x03) << 8) | static_cast<uint8_t>(data[2]);
    if (data.size() < length + 6) {
        qWarning() << "Message too short";
        return result;
    }

    result.messageType = getbitu(data, 24, 12);
    if (result.messageType != MSG_TYPE_1005 && result.messageType != MSG_TYPE_1006) {
        qWarning() << "Not a Type 1005 or 1006 message";
        return result;
    }

    staid = getbitu(data, i, 12);
    i += 12;
    i += 6 + 4;
    rr[0] = getbits_38(data, i);
    i += 38 + 2;
    rr[1] = getbits_38(data, i);
    i += 38 + 2;
    rr[2] = getbits_38(data, i);
    i += 38;

    if (result.messageType == MSG_TYPE_1006) {
        anth = getbitu(data, i, 16);
    }

    result.referenceStationId = staid;

    // Convert ECEF to Lat/Lon/Alt
    LatLongAlt converted = convertECEFToLatLonAlt(rr[0] * 0.0001, rr[1] * 0.0001, rr[2] * 0.0001);
    result.latitude = converted.latitude;
    result.longitude = converted.longitude;
    result.altitude = converted.altitude;

    // Antenna height (only for message type 1006)
    result.antennaHeight = (result.messageType == MSG_TYPE_1006) ? anth * 0.0001 : 0;

    // qDebug() << "Message Type:" << result.messageType;
    // qDebug() << "Reference Station ID:" << result.referenceStationId;
    // qDebug() << "Latitude:" << result.latitude;
    // qDebug() << "Longitude:" << result.longitude;
    // qDebug() << "Altitude:" << result.altitude;
    // if (result.messageType == MSG_TYPE_1006) {
    //     qDebug() << "Antenna Height:" << result.antennaHeight;
    // }

    result.isValid = true;

    constants->setNtripInfoLon(result.longitude);
    constants->setNtripInfoLat(result.latitude);
    constants->setNtripInfoAlt(result.altitude);


    return result;
}

// ---------------------------------------------------


static int decode_head1001(QByteArray rtcm)
{

    int i=24;
    int nsat = 0;
     i+=12;

    if (i+52<=rtcm.size()*8) {
        i+=12;
        i+=30;
        i+= 1;
        nsat =getbitu(rtcm,i, 5);
    }
    else {
        return -1;
    }

    return nsat;
}


static int decode_head1009(QByteArray rtcm)
{
    int i=24;
    int nsat = 0;
    i+=12;

    if (i+49<=rtcm.size()*8) {
        i+=12;
        i+=27; /* sec in a day */
        i+= 1;
        nsat =getbitu(rtcm,i, 5);
    }
    else {
        return -1;
    }

    return nsat;
}


static int parse_num_satellites(const QByteArray &message, int message_type)
{
    int nsat = -1;

    if (message_type == 1004 && (nsat=decode_head1001(message))<0) return -1;
    if (message_type == 1012 && (nsat=decode_head1009(message))<0) return -1;

    // qDebug() << nsat << " " << message_type;
    return nsat;
}
