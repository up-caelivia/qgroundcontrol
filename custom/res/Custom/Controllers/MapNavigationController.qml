import QtQuick 2.11
import QtPositioning 5.3

Item {
    id: navigationController

    property var mapControl: null
    property var vehicle: null
    property var toolInsets: null

    property bool trackingEnabled: false
    property bool keepMapCenteredOnVehicle: false
    property bool pipMode: false
    property bool _disableVehicleTracking: false

    readonly property bool canCenterOnVehicle: !!vehicle && !!mapControl && vehicle.coordinate.isValid
    readonly property var _vehicleCoordinate: vehicle ? vehicle.coordinate : QtPositioning.coordinate()
    readonly property bool recentering: animateLat.running || animateLong.running

    onTrackingEnabledChanged: {
        if (!trackingEnabled) {
            panRecenterTimer.stop()
            _disableVehicleTracking = false
        }
    }

    Timer {
        id:         panRecenterTimer
        interval:   10000
        running:    false
        onTriggered: {
            _disableVehicleTracking = false
            updateMapToVehiclePosition()
        }
    }

    Timer {
        interval:       500
        running:        navigationController.trackingEnabled
        repeat:         true
        onTriggered:    updateMapToVehiclePosition()
    }

    Connections {
        target: mapControl ? mapControl.gesture : null
        enabled: navigationController.trackingEnabled

        function onPanStarted() {       _disableVehicleTracking = true }
        function onFlickStarted() {     _disableVehicleTracking = true }
        function onPanFinished() {      panRecenterTimer.restart() }
        function onFlickFinished() {    panRecenterTimer.restart() }
    }

    property real _animatedLatitudeStart
    property real _animatedLatitudeStop
    property real _animatedLongitudeStart
    property real _animatedLongitudeStop
    property real animatedLatitude
    property real animatedLongitude

    onAnimatedLatitudeChanged: {
        if (mapControl) {
            mapControl.center = QtPositioning.coordinate(animatedLatitude, animatedLongitude)
        }
    }
    onAnimatedLongitudeChanged: {
        if (mapControl) {
            mapControl.center = QtPositioning.coordinate(animatedLatitude, animatedLongitude)
        }
    }

    NumberAnimation on animatedLatitude {
        id: animateLat
        from: _animatedLatitudeStart
        to: _animatedLatitudeStop
        duration: 1000
    }
    NumberAnimation on animatedLongitude {
        id: animateLong
        from: _animatedLongitudeStart
        to: _animatedLongitudeStop
        duration: 1000
    }

    function animatedMapRecenter(fromCoord, toCoord) {
        _animatedLatitudeStart = fromCoord.latitude
        _animatedLongitudeStart = fromCoord.longitude
        _animatedLatitudeStop = toCoord.latitude
        _animatedLongitudeStop = toCoord.longitude
        animateLat.start()
        animateLong.start()
    }

    function pointInRect(point, rect) {
        return point.x > rect.x &&
                point.x < rect.x + rect.width &&
                point.y > rect.y &&
                point.y < rect.y + rect.height;
    }

    function _insetCenterRect() {
        if (!toolInsets) {
            return Qt.rect(0, 0, mapControl.width, mapControl.height)
        }

        return Qt.rect(toolInsets.leftEdgeCenterInset,
                       toolInsets.topEdgeCenterInset,
                       mapControl.width - toolInsets.leftEdgeCenterInset - toolInsets.rightEdgeCenterInset,
                       mapControl.height - toolInsets.topEdgeCenterInset - toolInsets.bottomEdgeCenterInset)
    }

    function _insetCornerRects() {
        var rects = {
        "topleft":      Qt.rect(0,0,
                               toolInsets.leftEdgeTopInset,
                               toolInsets.topEdgeLeftInset),
        "topright":     Qt.rect(mapControl.width-toolInsets.rightEdgeTopInset,0,
                               toolInsets.rightEdgeTopInset,
                               toolInsets.topEdgeRightInset),
        "bottomleft":   Qt.rect(0,mapControl.height-toolInsets.bottomEdgeLeftInset,
                               toolInsets.leftEdgeBottomInset,
                               toolInsets.bottomEdgeLeftInset),
        "bottomright":  Qt.rect(mapControl.width-toolInsets.rightEdgeBottomInset,mapControl.height-toolInsets.bottomEdgeRightInset,
                               toolInsets.rightEdgeBottomInset,
                               toolInsets.bottomEdgeRightInset)}
        return rects
    }

    function recenterNeeded() {
        var vehiclePoint = mapControl.fromCoordinate(_vehicleCoordinate, false /* clipToViewport */)
        var centerRect = _insetCenterRect()
        if (!pointInRect(vehiclePoint, centerRect)) {
            return true
        }

        if (!toolInsets) {
            return false
        }

        var cornerRects = _insetCornerRects()
        if (pointInRect(vehiclePoint, cornerRects["topleft"])) {
            return true
        } else if (pointInRect(vehiclePoint, cornerRects["topright"])) {
            return true
        } else if (pointInRect(vehiclePoint, cornerRects["bottomleft"])) {
            return true
        } else if (pointInRect(vehiclePoint, cornerRects["bottomright"])) {
            return true
        }

        return false
    }

    function updateMapToVehiclePosition() {
        if (!trackingEnabled || !canCenterOnVehicle || recentering || _disableVehicleTracking ||
                keepMapCenteredOnVehicle || !mapControl.firstVehiclePositionReceived) {
            return
        }

        if (pipMode) {
            mapControl.center = _vehicleCoordinate
            return
        }

        if (!recenterNeeded()) {
            return
        }

        // Center the vehicle in the area left clear by the surrounding widgets.
        var vehiclePoint = mapControl.fromCoordinate(_vehicleCoordinate, false)
        var centerInsetRect = _insetCenterRect()
        var centerInsetPoint = Qt.point(centerInsetRect.x + centerInsetRect.width / 2,
                                        centerInsetRect.y + centerInsetRect.height / 2)
        var centerOffset = Qt.point(mapControl.width / 2 - centerInsetPoint.x,
                                   mapControl.height / 2 - centerInsetPoint.y)
        var vehicleOffsetPoint = Qt.point(vehiclePoint.x + centerOffset.x,
                                         vehiclePoint.y + centerOffset.y)
        var vehicleOffsetCoord = mapControl.toCoordinate(vehicleOffsetPoint, false)
        animatedMapRecenter(mapControl.center, vehicleOffsetCoord)
    }

    on_VehicleCoordinateChanged: {
        if (trackingEnabled && keepMapCenteredOnVehicle && canCenterOnVehicle &&
                !recentering && !_disableVehicleTracking) {
            mapControl.center = _vehicleCoordinate
        }
    }

    function centerOnVehicle() {
        if (canCenterOnVehicle) {
            animateLat.stop()
            animateLong.stop()
            panRecenterTimer.stop()

            _disableVehicleTracking = false
            animatedMapRecenter(mapControl.center, _vehicleCoordinate)
        }
    }
}
