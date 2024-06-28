#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <QObject>
#include <QString>
#include <QQmlEngine>
#include <QJSEngine>
#include <QVector>

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
    Q_PROPERTY(QVector<QString> factSpeedNames READ factSpeedNames CONSTANT)
    Q_PROPERTY(QVector<int> lowSpeed READ lowSpeed CONSTANT)
    Q_PROPERTY(QVector<int> normalSpeed READ normalSpeed CONSTANT)
    Q_PROPERTY(QVector<int> highSpeed READ highSpeed CONSTANT)
    Q_PROPERTY(QVector<QString> settingToShow READ settingToShow CONSTANT)
    Q_PROPERTY(int compassNumber READ compassNumber CONSTANT)

public:
    explicit Constants(QObject* parent = nullptr) : QObject(parent) {}

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


    QVector<QString> settingToShow() const { return {"Motors", "Safety"}; }
    int compassNumber() const { return 1; }

    static QObject* constants_singleton_provider(QQmlEngine* engine, QJSEngine* scriptEngine);

signals:
    void lastMaxHeightChanged();

private:
    int m_lastMaxHeight;
};



#endif // CONSTANTS_H
