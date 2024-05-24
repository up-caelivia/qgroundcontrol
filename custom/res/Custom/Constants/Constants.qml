pragma Singleton
import QtQuick 2.6

QtObject {
    id: constants

    readonly property bool developer: false

    readonly property var factNames: ["LOIT_SPEED", "WPNAV_SPEED", "FENCE_ALT_MAX"]
    readonly property var factDescription: ["Maximum speed reached by drone in loiter mode. Warning: in altitude hold mode speed is not limited", "Maximum speed reached by drone during automatic mission", ""]
    readonly property var factGoodNames: ["Maximum loiter speed", "Maximum auto speed", "Maximum altitude"]
    readonly property var factMin: [200, 200, 10]
    readonly property var factMax: [1500, 1500, 200]
    readonly property var factEditable: [false, true, true]
    readonly property int maxAltitudeWarning: 120
    readonly property double altitudeFactor: 0.95
    property int lastMaxHeight

}
