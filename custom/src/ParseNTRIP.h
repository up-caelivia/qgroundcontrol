#pragma once

#include "NTRIP.h"
#include "QGCLoggingCategory.h"
#include "constants.h"

    
struct LatLongAlt {
    double latitude;
    double longitude;
    double altitude;
    double antennaHeight;
    uint16_t referenceStationId;
    uint16_t messageType;
    bool isValid;
};

class ParseNTRIP
{
public:
    ParseNTRIP();
    void handleRTCM(const QByteArray& message);
    void setNtripReceiving(bool receiving);
    void setNtripEnabled(bool value);
    void setauthError(bool value);
    void setmountError(bool value);



private slots:

private:
    LatLongAlt decode_type1005_1006(const QByteArray& data);
    int parse_num_satellites(const QByteArray &message, int message_type);

    // Funzioni di utilità per il parsing
    uint32_t getbitu(const QByteArray &buff, int pos, int len);
    int32_t getbits(const QByteArray &buff, int pos, int len);
    int64_t getbits_38(const QByteArray &buff, int pos);
    LatLongAlt convertECEFToLatLonAlt(double X, double Y, double Z);
    Constants* constants = Constants::getInstance();
};