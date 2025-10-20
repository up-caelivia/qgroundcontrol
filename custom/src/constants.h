#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <QObject>
#include <QString>
#include <QQmlEngine>
#include <QJSEngine>
#include <QVector>
#include <QTimer>
#include <QDateTime>
#include <QSettings>         
#include <QGeoCoordinate>    
#include <limits>           

class Constants : public QObject {

    Q_OBJECT

    Q_PROPERTY(bool developer READ developer WRITE setDeveloper NOTIFY developerChanged)
    Q_PROPERTY(QVector<QString> factNames READ factNames CONSTANT)
    Q_PROPERTY(QVector<QString> factDescription READ factDescription CONSTANT)
    Q_PROPERTY(QVector<QString> factGoodNames READ factGoodNames CONSTANT)
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

    Q_PROPERTY(bool isABLUOApp READ isABLUOApp CONSTANT)

    Q_PROPERTY(bool showSavButtons READ showSavButtons WRITE setShowSavButtons NOTIFY showSavButtonsChanged)

    // ====== PERSISTENZA PIANO ABLUO ======
    Q_PROPERTY(double        savedPitch READ savedPitch WRITE setSavedPitch NOTIFY savedPitchChanged)
    Q_PROPERTY(QGeoCoordinate savedStart READ savedStart WRITE setSavedStart NOTIFY savedStartChanged)
    Q_PROPERTY(QGeoCoordinate savedStop  READ savedStop  WRITE setSavedStop  NOTIFY savedStopChanged)
    Q_PROPERTY(bool          savedSideLeft READ savedSideLeft WRITE setSavedSideLeft NOTIFY savedSideLeftChanged) // <<< AGGIUNTO

public:
    explicit Constants(QObject* parent = nullptr) : QObject(parent) {

        m_timer = new QTimer(this);
        connect(m_timer, &QTimer::timeout, this, &Constants::checkNtripStatus);
        m_timer->start(2000); // Check every 2 seconds

        // carica i valori persistenti
        loadAbluoPlan();
    }

    // Destructor
    ~Constants() {
        if (m_timer) {
            m_timer->stop();
            delete m_timer;
            m_timer = nullptr;
        }
    }

    bool developer() const { return m_developer; }
    QVector<QString> factNames() const { return {"LOIT_SPEED", 
                                                 "WPNAV_SPEED", 
                                                 "WPNAV_SPEED_DN", 
                                                 "WPNAV_SPEED_UP", 
                                                 "WP_YAW_BEHAVIOR",
                                                 "WPNAV_RADIUS"}; }
    QVector<QString> factDescription() const { return {"Maximum speed reached by drone in loiter mode. Warning: in altitude hold mode speed is not limited", 
                                                       "Maximum horizontal speed reached by drone during automatic mission", 
                                                       "Maximum descending speed reached by drone during automatic mission", 
                                                       "Maximum ascending speed reached by drone during automatic mission",
                                                       "",
                                                       "Distance from a waypoint, that when crossed indicates the waypoint has been hit"}; }
    QVector<QString> factGoodNames() const { return {"Maximum loiter speed", 
                                                     "Maximum horizontal auto speed", 
                                                     "Auto mode speed down", 
                                                     "Auto mode speed up", 
                                                     "Auto mode yaw behaviour",
                                                     "Waypoint Radius"}; }
    QVector<bool> factEditable() const { return {false, true, true, true, true, true}; }

    int maxAltitudeWarning() const { return 120; }
    double altitudeFactor() const { return 1.0; }

    int lastMaxHeight() const { return m_lastMaxHeight; }

    void setDeveloper(bool value){
        m_developer = value;
        emit developerChanged();
    }

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

    bool isABLUOApp() const { return _isABLUOApp; }
    bool showSavButtons() const { return _showSavButtons; }
    
    void setShowSavButtons(bool value){
        _showSavButtons = value;
        emit showSavButtonsChanged();
    }

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

    // ====== getter/setter persistenza ======
    double savedPitch() const { return m_savedPitch; }
    QGeoCoordinate savedStart() const { return m_savedStart; }
    QGeoCoordinate savedStop() const { return m_savedStop; }

    bool savedSideLeft() const { return m_savedSideLeft; }
    void setSavedSideLeft(bool v) {
        if (m_savedSideLeft != v) {
            m_savedSideLeft = v;
            saveAbluoPlan();
            emit savedSideLeftChanged();
        }
    }

    void setSavedPitch(double v) {
        if (!qFuzzyCompare(1+v, 1+m_savedPitch)) {
            m_savedPitch = v;
            saveAbluoPlan();
            emit savedPitchChanged();
        }
    }
    void setSavedStart(const QGeoCoordinate& c) {
        if (c != m_savedStart) {
            m_savedStart = c;
            saveAbluoPlan();
            emit savedStartChanged();
        }
    }
    void setSavedStop(const QGeoCoordinate& c) {
        if (c != m_savedStop) {
            m_savedStop = c;
            saveAbluoPlan();
            emit savedStopChanged();
        }
    }

    Q_INVOKABLE void clearAbluoPlan() {
        m_savedPitch = 0.0;
        m_savedStart = QGeoCoordinate();
        m_savedStop  = QGeoCoordinate();
        saveAbluoPlan();
        emit savedPitchChanged();
        emit savedStartChanged();
        emit savedStopChanged();
    }

    Q_INVOKABLE bool hasSavedStart() const { return m_savedStart.isValid(); }
    Q_INVOKABLE bool hasSavedStop() const  { return m_savedStop.isValid();  }
    

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
    void developerChanged();
    void showSavButtonsChanged();

    // persistenza
    void savedPitchChanged();
    void savedStartChanged();
    void savedStopChanged();
    void savedSideLeftChanged();

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
    bool m_developer = false;
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
#ifdef ABLUO_APP
    bool _isABLUOApp = true;
#else
    bool _isABLUOApp = false;
#endif
    bool _showSavButtons = false;

    // ====== storage plan ======
    double        m_savedPitch = 0.0;
    QGeoCoordinate m_savedStart;
    QGeoCoordinate m_savedStop;
    bool          m_savedSideLeft = true;

    inline QString _key(const char* name) const {
        return QStringLiteral("abl/planning/%1").arg(QString::fromUtf8(name));
    }

    void loadAbluoPlan() {
        QSettings s;
        m_savedPitch = s.value(_key("pitch"), 0.0).toDouble();

        // Start
        const double sLat = s.value(_key("startLat"), std::numeric_limits<double>::quiet_NaN()).toDouble();
        const double sLon = s.value(_key("startLon"), std::numeric_limits<double>::quiet_NaN()).toDouble();
        const double sAlt = s.value(_key("startAlt"), std::numeric_limits<double>::quiet_NaN()).toDouble();
        if (qIsFinite(sLat) && qIsFinite(sLon)) {
            m_savedStart = QGeoCoordinate(sLat, sLon, qIsFinite(sAlt) ? sAlt : 0.0);
        } else {
            m_savedStart = QGeoCoordinate();
        }

        // Stop
        const double tLat = s.value(_key("stopLat"), std::numeric_limits<double>::quiet_NaN()).toDouble();
        const double tLon = s.value(_key("stopLon"), std::numeric_limits<double>::quiet_NaN()).toDouble();
        const double tAlt = s.value(_key("stopAlt"), std::numeric_limits<double>::quiet_NaN()).toDouble();
        if (qIsFinite(tLat) && qIsFinite(tLon)) {
            m_savedStop = QGeoCoordinate(tLat, tLon, qIsFinite(tAlt) ? tAlt : 0.0);
        } else {
            m_savedStop = QGeoCoordinate();
        }
        
        m_savedSideLeft = s.value(_key("sideLeft"), true).toBool();
    }

    void saveAbluoPlan() const {
        QSettings s;
        s.setValue(_key("pitch"), m_savedPitch);

        if (m_savedStart.isValid()) {
            s.setValue(_key("startLat"), m_savedStart.latitude());
            s.setValue(_key("startLon"), m_savedStart.longitude());
            s.setValue(_key("startAlt"), m_savedStart.altitude());
        } else {
            s.remove(_key("startLat")); s.remove(_key("startLon")); s.remove(_key("startAlt"));
        }

        if (m_savedStop.isValid()) {
            s.setValue(_key("stopLat"), m_savedStop.latitude());
            s.setValue(_key("stopLon"), m_savedStop.longitude());
            s.setValue(_key("stopAlt"), m_savedStop.altitude());
        } else {
            s.remove(_key("stopLat")); s.remove(_key("stopLon")); s.remove(_key("stopAlt"));
        }

         s.setValue(_key("sideLeft"), m_savedSideLeft);
    }
};

#endif // CONSTANTS_H
