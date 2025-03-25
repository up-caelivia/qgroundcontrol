#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <QObject>
#include <QString>
#include <QQmlEngine>
#include <QJSEngine>
#include <QVector>
#include <QTimer>
#include <QDateTime>


class Constants : public QObject {

    Q_OBJECT

    Q_PROPERTY(bool developer READ developer CONSTANT)
    Q_PROPERTY(QVector<QString> factNames READ factNames CONSTANT)
    Q_PROPERTY(QVector<QString> factDescription READ factDescription CONSTANT)
    Q_PROPERTY(QVector<QString> factGoodNames READ factGoodNames CONSTANT)
    Q_PROPERTY(QVector<int> factMin READ factMin CONSTANT)
    Q_PROPERTY(QVector<int> factMax READ factMax CONSTANT)
    Q_PROPERTY(QVector<bool> factEditable READ factEditable CONSTANT)
    Q_PROPERTY(int maxAltitudeWarning READ maxAltitudeWarning CONSTANT)
    Q_PROPERTY(double altitudeFactor READ altitudeFactor CONSTANT)
    Q_PROPERTY(int lastMaxHeight READ lastMaxHeight WRITE setLastMaxHeight NOTIFY lastMaxHeightChanged)

    Q_PROPERTY(bool ntripEnabled READ ntripEnabled WRITE setNtripEnabled NOTIFY ntripEnabledChanged)
    Q_PROPERTY(bool ntripReceiving READ ntripReceiving WRITE setNtripReceiving NOTIFY ntripReceivingChanged)
    Q_PROPERTY(bool authError READ authError WRITE setauthError NOTIFY authErrorChanged)
    Q_PROPERTY(bool mountError READ mountError WRITE setmountError NOTIFY mountErrorChanged)

    Q_PROPERTY(double ntripInfoLat READ ntripInfoLat WRITE setNtripInfoLat NOTIFY ntripInfoLatChanged)
    Q_PROPERTY(double ntripInfoLon READ ntripInfoLon WRITE setNtripInfoLon NOTIFY ntripInfoLonChanged)
    Q_PROPERTY(double ntripInfoAlt READ ntripInfoAlt WRITE setNtripInfoAlt NOTIFY ntripInfoAltChanged)
    Q_PROPERTY(int numM READ numM WRITE setnumM NOTIFY numMChanged)
    Q_PROPERTY(int numGPS READ numGPS WRITE setnumGPS NOTIFY numGPSChanged)
    Q_PROPERTY(int numGLO READ numGLO WRITE setnumGLO NOTIFY numGLOChanged)


    Q_PROPERTY(QVector<QString> factSpeedNames READ factSpeedNames CONSTANT)
    Q_PROPERTY(QVector<int> lowSpeed READ lowSpeed CONSTANT)
    Q_PROPERTY(QVector<int> normalSpeed READ normalSpeed CONSTANT)
    Q_PROPERTY(QVector<int> highSpeed READ highSpeed CONSTANT)
    Q_PROPERTY(QVector<QString> settingToShow READ settingToShow CONSTANT)
    Q_PROPERTY(int compassNumber READ compassNumber CONSTANT)

public:
    explicit Constants(QObject* parent = nullptr) : QObject(parent) {

        m_timer = new QTimer(this);
        connect(m_timer, &QTimer::timeout, this, &Constants::checkNtripStatus);
        m_timer->start(2000); // Check every 2 seconds

    }

    // Destructor
    ~Constants() {
        if (m_timer) {
            m_timer->stop();
            delete m_timer;
            m_timer = nullptr;
        }
    }

    bool developer() const { return false; }
    QVector<QString> factNames() const { return {"LOIT_SPEED", "WPNAV_SPEED", "WPNAV_SPEED_DN", "WPNAV_SPEED_UP", "RTL_CLIMB_MIN", "WP_YAW_BEHAVIOR","FENCE_ALT_MAX"}; }
    QVector<QString> factDescription() const { return {"Maximum speed reached by drone in loiter mode. Warning: in altitude hold mode speed is not limited", "Maximum horizontal speed reached by drone during automatic mission", "Maximum descending speed reached by drone during automatic mission", "Maximum ascending speed reached by drone during automatic mission", "The altitude selected must be higher than all surrounding obstacles","",""}; }
    QVector<QString> factGoodNames() const { return {"Maximum loiter speed", "Maximum auto speed", "Automatic mode speed down", "Automatic mode speed up", "Altitude for Return To Launch mode", "Automatic mode yaw behaviour","Maximum altitude"}; }
    QVector<int> factMin() const { return {200, 200, 50, 50, 1000, 0, 10}; }
    QVector<int> factMax() const { return {1500, 1500, 150, 300, 6000, 0, 200}; }
    QVector<bool> factEditable() const { return {false, true, true, true, true, true, true}; }

    int maxAltitudeWarning() const { return 120; }
    double altitudeFactor() const { return 1.0; }

    int lastMaxHeight() const { return m_lastMaxHeight; }

    void setLastMaxHeight(int height) {
        if (m_lastMaxHeight != height) {
            m_lastMaxHeight = height;
            emit lastMaxHeightChanged();
        }
    }

    QVector<QString> factSpeedNames() const { return {"LOIT_SPEED", "PILOT_SPEED_DN", "PILOT_SPEED_UP", "PILOT_Y_RATE"}; }
    QVector<int> lowSpeed() const { return {260, 50, 100, 30}; }
    QVector<int> normalSpeed() const { return {500, 100, 150, 45}; }
    //QVector<int> highSpeed() const { return {1000, 250, 500, 60}; }   // CNES
    QVector<int> highSpeed() const { return {1000, 150, 250, 60}; }

    //Non superare i 19 m/s per certificazione

    bool ntripEnabled() const { return ntripEnableV; }
    bool authError() const { return authErrV; }
    bool ntripReceiving() const { return ntripReceivedV; }
    bool mountError() const { return mountErrorV; }

    double ntripInfoLat() const {return latitude;}
    double ntripInfoLon() const {return longitude;}
    double ntripInfoAlt() const {return altitude;}
    int numM() const {return numMV;}
    int numGPS() const {return numGPSV;}
    int numGLO() const {return numGLOV;}


    void setNtripEnabled(bool enable) {
        if (ntripEnableV != enable) {
            ntripEnableV = enable;
            emit ntripEnabledChanged();
        }
    }

    void setNtripReceiving(bool received) {

        if (received == true) {
            m_messageCount++;
            m_lastMessageTime = QDateTime::currentMSecsSinceEpoch();
        }

        // if(received == true && m_messageCount < 3) return;

        if (ntripReceivedV != received) {
            ntripReceivedV = received;
            emit ntripReceivingChanged();
        }
    }

    void setauthError(bool received) {
        if (authErrV != received) {
            authErrV = received;
            emit authErrorChanged();
        }
    }

    void setmountError(bool received) {
        if (mountErrorV != received) {
            mountErrorV = received;
            emit mountErrorChanged();
        }
    }

    void setNtripInfoLat(double lat) {
        if (lat != latitude) {
            latitude = lat;
            emit ntripInfoLatChanged();
        }
    }

    void setNtripInfoLon(double lon) {
        if (lon != longitude) {
            longitude = lon;
            emit ntripInfoLonChanged();
        }
    }

    void setNtripInfoAlt(double alt) {
        if (alt != altitude) {
            altitude = alt;
            emit ntripInfoAltChanged();
        }
    }

    void setnumM(int placeholder) {
        numMV++;
        emit numMChanged();

    }

    void setnumGPS(int num) {
        if (num != numGPSV) {
            numGPSV = num;
            emit numGPSChanged();
        }
    }

    void setnumGLO(int num) {
        if (num != numGLOV) {
            numGLOV = num;
            emit numGLOChanged();
        }
    }



    QVector<QString> settingToShow() const { return {"Motors", "Safety"}; }
    int compassNumber() const { return 3; }

    static QObject* constants_singleton_provider(QQmlEngine* engine, QJSEngine* scriptEngine);
    static Constants* getInstance();


signals:
    void lastMaxHeightChanged();
    void ntripEnabledChanged();
    void ntripReceivingChanged();
    void authErrorChanged();
    void mountErrorChanged();
    void ntripInfoLatChanged();
    void ntripInfoLonChanged();
    void ntripInfoAltChanged();
    void numMChanged();
    void numGPSChanged();
    void numGLOChanged();

private slots:

    void checkNtripStatus() {
        qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
        qint64 timeSinceLastMessage = currentTime - m_lastMessageTime;

        if (timeSinceLastMessage >= 2000 && m_messageCount < 3) {
            ntripReceivedV = false;
            emit ntripReceivingChanged();
        }

        // Reset the message count for the next 2-second interval
        m_messageCount = 0;
    }


private:
    int m_lastMaxHeight;
    bool ntripEnableV = false;
    bool authErrV = false;
    bool ntripReceivedV = false;
    bool mountErrorV = false;

    double latitude = 0;
    double longitude = 0;
    double altitude = 0;
    int numMV = 0;
    int numGPSV = 0;
    int numGLOV = 0;

    QTimer *m_timer;
    qint64 m_lastMessageTime = 0;
    int m_messageCount = 0;


};



#endif // CONSTANTS_H
