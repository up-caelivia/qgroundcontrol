// Variables with possible distances
var _scaleLengthsMeters = [5, 10, 25, 50, 100, 150, 250, 500, 1000, 2000, 5000, 10000, 20000, 50000, 100000, 200000, 500000, 1000000, 2000000]
var _scaleLengthsFeet = [10, 25, 50, 100, 250, 500, 1000, 2000, 3000, 4000, 5280, 5280*2, 5280*5, 5280*10, 5280*25, 5280*50, 5280*100, 5280*250, 5280*500, 5280*1000]

// Format methods
function formatDistanceMeters(meters) {
    var dist = Math.round(meters)
    if (dist > 1000 ){
        if (dist > 100000){
            dist = Math.round(dist / 1000)
        } else {
            dist = Math.round(dist / 100)
            dist /= 10
        }
        dist += qsTr(" km")
    } else {
        dist += qsTr(" m")
    }
    return dist
}

function formatDistanceFeet(feet) {
    var dist = Math.round(feet)
    if (dist >= 5280) {
        dist = Math.round(dist / 5280)
        if (dist == 1) {
            dist += qsTr(" mile")
        } else {
            dist += qsTr(" miles")
        }
    } else {
        dist += qsTr(" ft")
    }
    return dist
}

// Mathematical Methods
function calculateScaleValues(measuredMeters, measuredPixels, useFeet) {
    if (!isFinite(measuredMeters) || measuredMeters <= 0.0 ||
            !isFinite(measuredPixels) || measuredPixels <= 0.0) {
        return {
            valid: false,
            text: "",
            pixelWidth: 0
        }
    }

    var measuredDistance = useFeet ? measuredMeters * 3.2808399 : measuredMeters
    var distances = useFeet ? _scaleLengthsFeet : _scaleLengthsMeters
    var selectedDistance = distances[distances.length - 1]

    for (var i = 0; i < distances.length - 1; i++) {
        var midpoint = (distances[i] + distances[i + 1]) / 2
        if (measuredDistance < midpoint) {
            selectedDistance = distances[i]
            break
        }
    }

    return {
        valid: true,
        text: useFeet
                ? formatDistanceFeet(selectedDistance)
                : formatDistanceMeters(selectedDistance),
        pixelWidth: measuredPixels * selectedDistance / measuredDistance
    }
}

function applyScale(measuredMeters, measuredPixels, useFeet) {
    var results = calculateScaleValues(measuredMeters, measuredPixels, useFeet)
    leftEnd.visible = results.valid 
    rightEnd.visible = results.valid 
    centerLine.width = results.valid
                        ? results.pixelWidth - (2 * leftEnd.width)
                        : 0
    scaleText.text = results.text
}

function calculateScale() {
    if(mapControl) {
        var sampleWidth = 100
        var sampleY = mapControl.height / 2
        var sampleX = (mapControl.width - sampleWidth) / 2
        var leftCoord = mapControl.toCoordinate(Qt.point(sampleX, sampleY), false)
        var rightCoord = mapControl.toCoordinate(Qt.point(sampleX + sampleWidth, sampleY), false)

        var measuredMeters = leftCoord.distanceTo(rightCoord)
        var useFeet = QGroundControl.settingsManager.unitsSettings.horizontalDistanceUnits.value === UnitsSettings.HorizontalDistanceUnitsFeet

        applyScale(measuredMeters, sampleWidth, useFeet)
    }
}