pragma Singleton
import QtQuick 2.6

QtObject {
    id: constants

    readonly property bool developer: false

    property var factNames: ["LOIT_SPEED", "WPNAV_SPEED", "FENCE_ALT_MAX"]
    property var factDescription: ["Maximum speed reached by drone in loiter mode. Warning: in altitude hold mode speed is not limited", "Maximum speed reached by drone during automatic mission", ""]
    property var factGoodNames: ["Maximum loiter speed", "Maximum auto speed", "Maximum altitude"]
    property var factMin: [200, 200, 10]
    property var factMax: [1500, 1500, 200]
    property var factEditable: [false, true, true]
    property int maxAltitudeWarning: 120

}
