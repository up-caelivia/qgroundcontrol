#include "ParseNTRIP.h"
#include "QGCApplication.h"
#include "QGCToolbox.h"
#include "constants.h"

#define RTCM3_PREAMBLE 0xD3
#define MSG_TYPE_1005 1005
#define MSG_TYPE_1006 1006


ParseNTRIP::ParseNTRIP()
{}

void ParseNTRIP::setNtripReceiving(bool receiving)
{
    constants->setNtripReceiving(receiving);
}

void ParseNTRIP::setNtripEnabled(bool value)
{
    constants->setNtripEnabled(value);
}

void ParseNTRIP::setauthError(bool value)
{
    constants->setauthError(value);
}

void ParseNTRIP::setmountError(bool value)
{
    constants->setmountError(value);
}


void ParseNTRIP::handleRTCM(const QByteArray& message)
{
    if (message.isEmpty()) return;
    constants->setnumM(0);

    uint16_t id = ((uint8_t)message[3] << 4) | ((uint8_t)message[4] >> 4);

    constants->setNtripReceiving(true);

    if(id == 1005 || id == 1006)
        decode_type1005_1006(message);

    int sat = -1;
    if(id == 1004 || id == 1012 || id == 1094 || id == 1124)
        sat = parse_num_satellites(message, id);

    if(id == 1004 && sat != -1)
        constants->setnumGPS(sat);

    if(id == 1012 && sat != -1)
        constants->setnumGLO(sat);
}

// ---- Funzioni di parsing --------------------------------------------

uint32_t ParseNTRIP::getbitu(const QByteArray &buff, int pos, int len) {
    uint32_t bits = 0;
    for (int i = pos; i < pos + len; i++) {
        if (i / 8 >= buff.size()) return 0;
        bits = (bits << 1) | ((buff[i / 8] >> (7 - i % 8)) & 1u);
    }
    return bits;
}

int32_t ParseNTRIP::getbits(const QByteArray &buff, int pos, int len) {
    int32_t bits = getbitu(buff, pos, len);
    if (len <= 0 || len >= 32 || !(bits & (1u << (len - 1)))) return bits;
    return bits | (~0u << len);
}

int64_t ParseNTRIP::getbits_38(const QByteArray &buff, int pos) {
    return (int64_t)getbits(buff, pos, 32) * 64 + getbitu(buff, pos + 32, 6);
}

LatLongAlt ParseNTRIP::convertECEFToLatLonAlt(double X, double Y, double Z) {
    LatLongAlt result = {0, 0, 0, 0, 0, 0, true};
    const double a = 6378137.0;
    const double f = 1.0 / 298.257223563;
    const double b = a * (1 - f);
    const double e2 = f * (2 - f);
    const double ep2 = (a * a - b * b) / (b * b);

    double p = sqrt(X * X + Y * Y);
    double theta = atan2(Z * a, p * b);

    result.longitude = atan2(Y, X);
    result.latitude = atan2(Z + ep2 * b * pow(sin(theta), 3), p - e2 * a * pow(cos(theta), 3));

    double N = a / sqrt(1 - e2 * pow(sin(result.latitude), 2));
    result.altitude = p / cos(result.latitude) - N;

    result.latitude *= 180.0 / M_PI;
    result.longitude *= 180.0 / M_PI;

    return result;
}

LatLongAlt ParseNTRIP::decode_type1005_1006(const QByteArray &data) {
    LatLongAlt result = {0, 0, 0, 0, 0, 0, false};

    if (data.size() < 6 || static_cast<uint8_t>(data[0]) != RTCM3_PREAMBLE) return result;

    int i = 24 + 12;
    int staid;
    double rr[3], anth = 0;

    uint16_t length = ((static_cast<uint8_t>(data[1]) & 0x03) << 8) | static_cast<uint8_t>(data[2]);
    if (data.size() < length + 6) return result;

    result.messageType = getbitu(data, 24, 12);
    if (result.messageType != MSG_TYPE_1005 && result.messageType != MSG_TYPE_1006) return result;

    staid = getbitu(data, i, 12); i += 12;
    i += 6 + 4;
    rr[0] = getbits_38(data, i); i += 38 + 2;
    rr[1] = getbits_38(data, i); i += 38 + 2;
    rr[2] = getbits_38(data, i); i += 38;

    if (result.messageType == MSG_TYPE_1006)
        anth = getbitu(data, i, 16);

    result.referenceStationId = staid;

    LatLongAlt converted = convertECEFToLatLonAlt(rr[0] * 0.0001, rr[1] * 0.0001, rr[2] * 0.0001);
    result.latitude = converted.latitude;
    result.longitude = converted.longitude;
    result.altitude = converted.altitude;

    result.antennaHeight = (result.messageType == MSG_TYPE_1006) ? anth * 0.0001 : 0;
    result.isValid = true;

    constants->setNtripInfoLon(result.longitude);
    constants->setNtripInfoLat(result.latitude);
    constants->setNtripInfoAlt(result.altitude);

    return result;
}

int ParseNTRIP::parse_num_satellites(const QByteArray &message, int message_type)
{
    int i = 24 + 12;
    int nsat = -1;

    if (message_type == 1004 && (i + 52 <= message.size() * 8)) {
        i += 12 + 30 + 1;
        nsat = getbitu(message, i, 5);
    }

    if (message_type == 1012 && (i + 49 <= message.size() * 8)) {
        i += 12 + 27 + 1;
        nsat = getbitu(message, i, 5);
    }

    return nsat;
}
