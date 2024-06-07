pragma Singleton
import QtQuick 2.6

QtObject {
    id: constants

    readonly property bool developer: true

    readonly property var factNames: ["LOIT_SPEED", "WPNAV_SPEED", "WPNAV_SPEED_DN", "WPNAV_SPEED_UP", "RTL_CLIMB_MIN", "WP_YAW_BEHAVIOR","FENCE_ALT_MAX"]
    readonly property var factDescription: ["Maximum speed reached by drone in loiter mode. Warning: in altitude hold mode speed is not limited", "Maximum horizontal speed reached by drone during automatic mission", "Maximum descending speed reached by drone during automatic mission", "Maximum ascending speed reached by drone during automatic mission", "The altitude selected must be higher than all surrounding obstacles","",""]
    readonly property var factGoodNames: ["Maximum loiter speed", "Maximum auto speed", "Automatic mode speed down", "Automatic mode speed up", "Altitude for Return To Launch mode", "Automatic mode yaw behaviour","Maximum altitude"]
    readonly property var factMin: [200, 200, 50, 50, 1000, 0,10]
    readonly property var factMax: [1500, 1500, 150, 300, 6000, 0,200]
    readonly property var factEditable: [false, true, true, true, true, true, true]

    readonly property int maxAltitudeWarning: 120
    readonly property double altitudeFactor: 0.95
    property int lastMaxHeight

    readonly property var factSpeedNames: ["LOIT_SPEED", "PILOT_SPEED_DN", "PILOT_SPEED_UP", "PILOT_Y_RATE"]
    readonly property var lowSpeed: [260, 50, 100, 30]
    readonly property var normalSpeed: [500, 100, 150, 45]
    readonly property var highSpeed: [1000, 150, 250, 60]

    readonly property var settingToShow: ["Motors", "Safety"]
    readonly property int compassNumber: 1

}
