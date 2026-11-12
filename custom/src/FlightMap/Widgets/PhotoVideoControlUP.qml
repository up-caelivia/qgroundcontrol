/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick                  2.12
import QtQuick.Layouts          1.2
import QtQuick.Dialogs          1.2
import QtGraphicalEffects       1.0

import QGroundControl                   1.0
import QGroundControl.ScreenTools       1.0
import QGroundControl.Controls          1.0
import QGroundControl.Palette           1.0
import QGroundControl.Vehicle           1.0
import QGroundControl.Controllers       1.0
import QGroundControl.FactSystem        1.0
import QGroundControl.FactControls      1.0

Item {
    id: root

    property real buttonSize

    property bool collapsed: false
    property bool _grabVideoFrame: false

    readonly property bool recordingActive: _mavlinkCameraVideoIsRecording || _videoStreamRecording

    readonly property real _collapseButtonSize: Math.max(
        ScreenTools.minTouchPixels,
        ScreenTools.defaultFontPixelHeight * 2
    )

    // Slide the handle and camera buttons together, leaving only the handle at the edge.
    property real _revealProgress: collapsed ? 0 : 1
    readonly property real _collapseOffset: (1 - _revealProgress) * (buttonSize + _margins)
    readonly property real occupiedWidth: Math.max(
        _collapseButtonSize, width - _collapseOffset
    )

    Behavior on _revealProgress {
        NumberAnimation {
            duration: 150
            easing.type: Easing.InOutQuad
        }
    }

    implicitWidth: _collapseButtonSize + _margins + buttonSize
    implicitHeight: Math.max(_collapseButtonSize, cameraButtons.implicitHeight)

    height: implicitHeight
    width: implicitWidth
    clip: true

    visible:    _activeVehicle &&
                !_activeVehicle.vehicleLinkManager.communicationLost &&
                (!!_mavlinkCamera || _videoStreamManager.streaming) &&
                multiVehiclePanelSelector.showSingleVehiclePanel

    property real   _margins:                                   ScreenTools.defaultFontPixelHeight / 2
    property var    _activeVehicle:                             QGroundControl.multiVehicleManager.activeVehicle

    // The following properties relate to a simple camera
    property var    _flyViewSettings:                           QGroundControl.settingsManager.flyViewSettings
    property bool   _simpleCameraAvailable:                     !_mavlinkCamera && _activeVehicle && _flyViewSettings.showSimpleCameraControl.rawValue
    property bool   _onlySimpleCameraAvailable:                 !_anyVideoStreamAvailable && _simpleCameraAvailable
    property bool   _simpleCameraIsShootingInCurrentMode:       _onlySimpleCameraAvailable && !_simplePhotoCaptureIsIdle

    // The following properties relate to a simple video stream
    property var    _videoStreamSettings:                       QGroundControl.settingsManager.videoSettings
    property var    _videoStreamManager:                        QGroundControl.videoManager
    property bool   _simplePhotoCaptureIsIdle:             true
    property bool   _videoStreamRecording:                      _videoStreamManager.recording
    property bool   _videoStreamCanShoot:                       _videoStreamManager.streaming
    property bool   _videoStreamIsShootingInCurrentMode:        _videoStreamInPhotoMode ? !_simplePhotoCaptureIsIdle : _videoStreamRecording
    property bool   _videoStreamInPhotoMode:                    false

    // The following properties relate to a mavlink protocol camera
    property var    _mavlinkCameraManager:                      _activeVehicle ? _activeVehicle.cameraManager : null
    property int    _mavlinkCameraManagerCurCameraIndex:        _mavlinkCameraManager ? _mavlinkCameraManager.currentCamera : -1
    property bool   _noMavlinkCameras:                          _mavlinkCameraManager ? _mavlinkCameraManager.cameras.count === 0 : true
    property var    _mavlinkCamera: {
        if (!_noMavlinkCameras) {
            var camera = _mavlinkCameraManager.cameras.get(_mavlinkCameraManagerCurCameraIndex)
            return camera && camera.paramComplete ? camera : null
        }
        return null
    }
    property bool   _multipleMavlinkCameras:                    _mavlinkCameraManager ? _mavlinkCameraManager.cameras.count > 1 : false
    property bool   _multipleMavlinkCameraStreams:              _mavlinkCamera ? _mavlinkCamera.streamLabels.length > 1 : false
    property int    _mavlinCameraCurStreamIndex:                _mavlinkCamera ? _mavlinkCamera.currentStream : -1
    property bool   _mavlinkCameraHasThermalVideoStream:        _mavlinkCamera ? _mavlinkCamera.thermalStreamInstance : false
    property bool   _mavlinkCameraModeUndefined:                _mavlinkCamera ? _mavlinkCamera.cameraMode === QGCCameraControl.CAM_MODE_UNDEFINED : true
    property bool   _mavlinkCameraInVideoMode:                  _mavlinkCamera ? _mavlinkCamera.cameraMode === QGCCameraControl.CAM_MODE_VIDEO : false
    property bool   _mavlinkCameraInPhotoMode:                  _mavlinkCamera ? _mavlinkCamera.cameraMode === QGCCameraControl.CAM_MODE_PHOTO : false
    property bool   _mavlinkCameraElapsedMode:                  _mavlinkCamera && _mavlinkCamera.cameraMode === QGCCameraControl.CAM_MODE_PHOTO && _mavlinkCamera.photoMode === QGCCameraControl.PHOTO_CAPTURE_TIMELAPSE
    property bool   _mavlinkCameraHasModes:                     _mavlinkCamera && _mavlinkCamera.hasModes
    property bool   _mavlinkCameraVideoIsRecording:             _mavlinkCamera && _mavlinkCamera.videoStatus === QGCCameraControl.VIDEO_CAPTURE_STATUS_RUNNING
    property bool   _mavlinkCameraPhotoCaptureIsIdle:           _mavlinkCamera && (_mavlinkCamera.photoStatus === QGCCameraControl.PHOTO_CAPTURE_IDLE || _mavlinkCamera.photoStatus >= QGCCameraControl.PHOTO_CAPTURE_LAST)
    property bool   _mavlinkCameraStorageReady:                 _mavlinkCamera && _mavlinkCamera.storageStatus === QGCCameraControl.STORAGE_READY
    property bool   _mavlinkCameraBatteryReady:                 _mavlinkCamera && _mavlinkCamera.batteryRemaining >= 0
    property bool   _mavlinkCameraStorageSupported:             _mavlinkCamera && _mavlinkCamera.storageStatus !== QGCCameraControl.STORAGE_NOT_SUPPORTED
    property bool   _mavlinkCameraCanShoot:                     (!_mavlinkCameraModeUndefined && ((_mavlinkCameraStorageReady && _mavlinkCamera.storageFree > 0) || !_mavlinkCameraStorageSupported)) || _videoStreamManager.streaming
    property bool   _mavlinkCameraIsShooting:                   ((_mavlinkCameraInVideoMode && _mavlinkCameraVideoIsRecording) || (_mavlinkCameraInPhotoMode && !_mavlinkCameraPhotoCaptureIsIdle)) || _videoStreamManager.recording

    // The following settings and functions unify between a mavlink camera and a simple video stream for simple access

    property bool   _anyVideoStreamAvailable:                   _videoStreamManager.hasVideo
    property string _cameraName:                                _mavlinkCamera && _multipleMavlinkCameras ? _mavlinkCamera.modelName : ""
    property bool   _showModeIndicator:                         _mavlinkCamera ? _mavlinkCameraHasModes : _videoStreamManager.hasVideo
    property bool   _modeIndicatorPhotoMode:                    _mavlinkCamera ? _mavlinkCameraInPhotoMode : _videoStreamInPhotoMode || _onlySimpleCameraAvailable
    property bool   _canShootInCurrentMode:                     _mavlinkCamera ? _mavlinkCameraCanShoot : _videoStreamCanShoot || _simpleCameraAvailable
    property bool   _isShootingInCurrentMode:                   _mavlinkCamera ? _mavlinkCameraIsShooting : _videoStreamIsShootingInCurrentMode || _simpleCameraIsShootingInCurrentMode

    property var _settingsDialog: null

    Component.onDestruction: {
        if (_settingsDialog) {
            _settingsDialog.destroy()
        }
    }

    function setCameraMode(photoMode) {
        _videoStreamInPhotoMode = photoMode
        if (_mavlinkCamera) {
            if (photoMode) {
                _mavlinkCamera.setPhotoMode()
            } else {
                _mavlinkCamera.setVideoMode()
            }
        }
    }

    function toggleShooting() {
        console.log("toggleShooting", _anyVideoStreamAvailable)

        // This whole mavlinkCameraCaptureVideoOrPhotos stuff is to work around some strange qml boolean testing
        // behavior which wasn't working correctly. This should work:
        //    if (_mavlinkCamera && (_mavlinkCamera.capturesVideo || _mavlinkCamera.capturesPhotos) ) {
        // but it doesn't for some strange reason. Hence all the stuff below...
        var mavlinkCameraCaptureVideoOrPhotos = false
        if (_mavlinkCamera) {
            if (_mavlinkCamera.capturesVideo || _mavlinkCamera.capturesPhotos) {
                mavlinkCameraCaptureVideoOrPhotos = true
            }
        }

        if (mavlinkCameraCaptureVideoOrPhotos) {
            if(_mavlinkCameraInVideoMode) {
                _mavlinkCamera.toggleVideo()
            } else {
                if(_mavlinkCameraInPhotoMode && !_mavlinkCameraPhotoCaptureIsIdle && _mavlinkCameraElapsedMode) {
                    _mavlinkCamera.stopTakePhoto()
                } else {
                    _mavlinkCamera.takePhoto()
                }
            }
        } else if (_onlySimpleCameraAvailable || (_simpleCameraAvailable && _anyVideoStreamAvailable && _videoStreamInPhotoMode && !_grabVideoFrame)) {
            _simplePhotoCaptureIsIdle = false
            _activeVehicle.triggerSimpleCamera()
            simplePhotoCaptureTimer.start()
        } else if (_anyVideoStreamAvailable) {
            if (_videoStreamInPhotoMode) {
                _simplePhotoCaptureIsIdle = false
                _videoStreamManager.grabImage()
                simplePhotoCaptureTimer.start()
            } else {
                if (_videoStreamManager.recording) {
                    _videoStreamManager.stopRecording()
                } else {
                    _videoStreamManager.startRecording()
                }
            }
        }
    }

    Timer {
        id: simplePhotoCaptureTimer
        interval: 500
        onTriggered: _simplePhotoCaptureIsIdle = true
    }

    Timer {
        id: photoFeedback
        interval: 180
    }

    QGCPalette {
        id: qgcPal
        colorGroupEnabled: root.enabled
    }

    QGCMapPalette {
        id: overlayPal
        lightColors: true
    }

    Item {
        id: collapseHandle

        x: root._collapseOffset
        anchors.verticalCenter: parent.verticalCenter
        width: root._collapseButtonSize
        height: Math.min(root._collapseButtonSize, root.height)

        QGCColoredImage {
            anchors.centerIn: parent
            width: Math.min(collapseHandle.width, collapseHandle.height)
                   * (root._revealProgress === 0 ? 0.78 : 0.58)
            height: width
            source: root._revealProgress === 0 ? "/custom/img/buttonCamera.svg" : "/res/buttonRight.svg"
            color: overlayPal.text
            rotation: root._revealProgress === 0 ? 0 : 180 * (1 - root._revealProgress)
        }

        QGCLabel {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.bottom
            anchors.topMargin: ScreenTools.defaultFontPixelHeight * 0.15
            text: qsTr("REC")
            visible: root.recordingActive
            color: qgcPal.colorRed
            font.pointSize: ScreenTools.smallFontPointSize
            style: Text.Outline
            styleColor: overlayPal.textOutline
        }

        MouseArea {
            anchors.fill: parent
            onClicked: root.collapsed = !root.collapsed
        }
    }

    Flickable {
        id: cameraViewport

        x: collapseHandle.x + collapseHandle.width + root._margins
        y: 0
        width: root.buttonSize
        height: root.height

        contentWidth: width
        contentHeight: cameraButtons.implicitHeight
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        interactive: contentHeight > height

        opacity: root._revealProgress
        enabled: !root.collapsed

        Column {
            id: cameraButtons

            width: cameraViewport.width
            spacing: root._margins * 0.5

            Item {
                width: cameraViewport.width
                height: root.buttonSize
                visible: !root._onlySimpleCameraAvailable

                QGCColoredImage {
                    anchors.centerIn: parent
                    width: root.buttonSize * 0.46
                    height: width
                    source: "/res/gear-black.svg"
                    color: overlayPal.text

                    layer.enabled: true
                    layer.effect: DropShadow {
                        horizontalOffset: 0
                        verticalOffset: 1
                        radius: 4
                        samples: 9
                        color: overlayPal.textOutline
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        if (!_settingsDialog) {
                            _settingsDialog = settingsDialogComponent.createObject(mainWindow)
                        }
                        _settingsDialog.open()
                    }
                }
            }

            Item {
                id: captureControl

                width: cameraViewport.width
                height: root.buttonSize
                    + (recordingTime.visible ? recordingTime.implicitHeight : 0)
                enabled: root._canShootInCurrentMode || root._isShootingInCurrentMode
                opacity: enabled ? 1 : 0.35

                Rectangle {
                    anchors.horizontalCenter: parent.horizontalCenter
                    y: root.buttonSize * 0.08
                    width: root.buttonSize * 0.84
                    height: width
                    radius: width / 2
                    color: "transparent"
                    border.width: Math.max(2, root.buttonSize * 0.04)
                    border.color: overlayPal.text

                    Rectangle {
                        anchors.centerIn: parent
                        width: parent.width * (root._isShootingInCurrentMode ? 0.45 : 0.72)
                        height: width
                        radius: root._isShootingInCurrentMode ? width * 0.1 : width / 2
                        color: root._modeIndicatorPhotoMode
                            ? overlayPal.text : qgcPal.colorRed
                        scale: root._modeIndicatorPhotoMode && photoFeedback.running
                            ? 0.85 : 1

                        Behavior on scale {
                            NumberAnimation { duration: 90 }
                        }
                    }
                }

                QGCLabel {
                    id: recordingTime

                    anchors.horizontalCenter: parent.horizontalCenter
                    y: root.buttonSize
                    visible: root.recordingActive
                    text: root._mavlinkCameraVideoIsRecording
                        ? root._mavlinkCamera.recordTimeStr : qsTr("REC")
                    font.pointSize: ScreenTools.smallFontPointSize
                    color: overlayPal.text
                    style: Text.Outline
                    styleColor: overlayPal.textOutline
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        root.toggleShooting()
                        if (root._modeIndicatorPhotoMode) {
                            photoFeedback.restart()
                        }
                    }
                }
            }

            Item {
                width: cameraViewport.width
                height: root.buttonSize
                visible: root._showModeIndicator
                enabled: !root._mavlinkCamera || !root._mavlinkCameraIsShooting
                opacity: enabled ? 1 : 0.35

                QGCColoredImage {
                    anchors.centerIn: parent
                    width: root.buttonSize * 0.46
                    height: width
                    source: root._modeIndicatorPhotoMode
                        ? "/qmlimages/camera_video.svg"
                        : "/qmlimages/camera_photo.svg"
                    color: overlayPal.text

                    layer.enabled: true
                    layer.effect: DropShadow {
                        horizontalOffset: 0
                        verticalOffset: 1
                        radius: 4
                        samples: 9
                        color: overlayPal.textOutline
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: root.setCameraMode(!root._modeIndicatorPhotoMode)
                }
            }
        }
    }

    Component {
        id: settingsDialogComponent

        QGCSidebarDialog {
            title:      qsTr("Settings")

            contentItem: Component {
            ColumnLayout {
                spacing: _margins

                QGCLabel {
                    text: root._cameraName
                    visible: text !== ""
                }

                QGCLabel {
                    text: root._mavlinkCamera
                        ? qsTr("Free Space: ") + root._mavlinkCamera.storageFreeStr
                        : ""
                    visible: root._mavlinkCameraStorageReady
                }

                QGCLabel {
                    text: root._mavlinkCamera
                        ? qsTr("Battery: ") + root._mavlinkCamera.batteryRemainingStr
                        : ""
                    visible: root._mavlinkCameraBatteryReady
                }

                QGCLabel {
                    text: qsTr("Photos: %1").arg(
                        root._activeVehicle
                            ? root._activeVehicle.cameraTriggerPoints.count : 0)
                    visible: root._modeIndicatorPhotoMode
                }

                QGCLabel {
                    text: qsTr("Photo capture source")
                    visible: root._simpleCameraAvailable && root._anyVideoStreamAvailable
                }

                QGCComboBox {
                    visible: root._simpleCameraAvailable && root._anyVideoStreamAvailable
                    model: [qsTr("Camera Trigger"), qsTr("Video Grab")]
                    currentIndex: root._grabVideoFrame ? 1 : 0
                    onActivated: root._grabVideoFrame = index === 1
                }

                GridLayout {
                    id:     gridLayout
                    flow:   GridLayout.TopToBottom
                    rows:   dynamicRows + (_mavlinkCamera ? _mavlinkCamera.activeSettings.length : 0)
                    height: parent.height

                    // Layout.margins: _margins
                    property int dynamicRows: 10

                    // First column
                    QGCLabel {
                        text:               qsTr("Camera")
                        visible:            _multipleMavlinkCameras
                        onVisibleChanged:   gridLayout.dynamicRows += visible ? 1 : -1
                    }

                    QGCLabel {
                        text:               qsTr("Video Stream")
                        visible:            _multipleMavlinkCameraStreams
                        onVisibleChanged:   gridLayout.dynamicRows += visible ? 1 : -1
                    }

                    QGCLabel {
                        id: size_reference
                        text:               qsTr("Thermal View Mode")
                        visible:            _mavlinkCameraHasThermalVideoStream
                        onVisibleChanged:   gridLayout.dynamicRows += visible ? 1 : -1
                    }

                    QGCLabel {
                        text:               qsTr("Blend Opacity")
                        visible:            _mavlinkCameraHasThermalVideoStream && _mavlinkCamera.thermalMode === QGCCameraControl.THERMAL_BLEND
                        onVisibleChanged:   gridLayout.dynamicRows += visible ? 1 : -1
                    }



                    // Mavlink Camera Protocol active settings
                    Repeater {
                        model: _mavlinkCamera ? _mavlinkCamera.activeSettings : []

                        QGCLabel {
                            text: get_text(_mavlinkCamera.getFact(modelData).shortDescription)
                            width: ScreenTools.defaultFontPixelHeight * 10 // Aumentato per il test
                            wrapMode: Text.WordWrap
                            clip: false // Rimosso clipping

                            function get_text(text) {
                                if (text.indexOf("NUC") !== -1)
                                    return "NUC Correction";

                                if (text.indexOf("camera simultaneously") !== -1)
                                    return "Zoom EO/IR together";

                                if (text.indexOf("Upper limit") !== -1)
                                    return "Upper limit temp. alarm";

                                if (text.indexOf("Lower limit") !== -1)
                                    return "Lower limit temp. alarm";

                                return text;
                            }
                        }
                    }

                    QGCLabel {
                        text:               qsTr("Photo Mode")
                        visible:            _mavlinkCameraHasModes
                        onVisibleChanged:   gridLayout.dynamicRows += visible ? 1 : -1
                    }

                    QGCLabel {
                        text:               qsTr("Photo Interval (seconds)")
                        visible:            _mavlinkCameraInPhotoMode && _mavlinkCamera.photoMode === QGCCameraControl.PHOTO_CAPTURE_TIMELAPSE
                        onVisibleChanged:   gridLayout.dynamicRows += visible ? 1 : -1
                    }

                    QGCLabel {
                        text:               qsTr("Video Grid Lines")
                        visible:            _anyVideoStreamAvailable
                        onVisibleChanged:   gridLayout.dynamicRows += visible ? 1 : -1
                    }

                    QGCLabel {
                        text:               qsTr("Video Screen Fit")
                        visible:            _anyVideoStreamAvailable
                        onVisibleChanged:   gridLayout.dynamicRows += visible ? 1 : -1
                    }

                    QGCLabel {
                        text:               qsTr("Reset Camera Defaults")
                        visible:            _mavlinkCamera
                        onVisibleChanged:   gridLayout.dynamicRows += visible ? 1 : -1
                    }

                    QGCLabel {
                        text:               qsTr("Storage")
                        visible:            _mavlinkCameraStorageSupported
                        onVisibleChanged:   gridLayout.dynamicRows += visible ? 1 : -1
                    }

                    // Second column
                    QGCComboBox {
                        // Layout.fillWidth:   true
                        sizeToContents:     true
                        model:              _mavlinkCameraManager ? _mavlinkCameraManager.cameraLabels : []
                        currentIndex:       _mavlinkCameraManagerCurCameraIndex
                        visible:            _multipleMavlinkCameras
                        onActivated:        _mavlinkCameraManager.currentCamera = index
                    }

                    QGCComboBox {
                        // Layout.fillWidth:   true
                        sizeToContents:     true
                        model:              _mavlinkCamera ? _mavlinkCamera.streamLabels : []
                        currentIndex:       _mavlinCameraCurStreamIndex
                        visible:            _multipleMavlinkCameraStreams
                        onActivated:        _mavlinkCamera.currentStream = index
                    }

                    QGCComboBox {
                        // Layout.fillWidth:   true
                        sizeToContents:     true
                        model:              [ qsTr("Off"), qsTr("Blend"), qsTr("Full"), qsTr("Picture In Picture") ]
                        currentIndex:       _mavlinkCamera ? _mavlinkCamera.thermalMode : -1
                        visible:            _mavlinkCameraHasThermalVideoStream
                        onActivated:        _mavlinkCamera.thermalMode = index
                    }

                    QGCSlider {
                        // Layout.fillWidth:           true
                        maximumValue:               100
                        minimumValue:               0
                        value:                      _mavlinkCamera ? _mavlinkCamera.thermalOpacity : 0
                        updateValueWhileDragging:   true
                        visible:                    _mavlinkCameraHasThermalVideoStream && _mavlinkCamera.thermalMode === QGCCameraControl.THERMAL_BLEND
                        onValueChanged:             _mavlinkCamera.thermalOpacity = value
                    }

                    // Mavlink Camera Protocol active settings
                    Repeater {
                        model: _mavlinkCamera ? _mavlinkCamera.activeSettings : []

                        RowLayout {
                            Layout.fillWidth:   true
                            spacing:            ScreenTools.defaultFontPixelWidth

                            property var    _fact:      _mavlinkCamera.getFact(modelData)
                            property bool   _isBool:    _fact.typeIsBool
                            property bool   _isCombo:   !_isBool && _fact.enumStrings.length > 0
                            property bool   _isSlider:  _fact && !isNaN(_fact.increment)
                            property bool   _isEdit:    !_isBool && !_isSlider && _fact.enumStrings.length < 1

                            FactComboBox {
                                // Layout.fillWidth:   true
                                sizeToContents:     true
                                fact:               parent._fact
                                indexModel:         false
                                visible:            parent._isCombo
                            }
                            FactTextField {
                                // Layout.fillWidth:   true
                                fact:               parent._fact
                                visible:            parent._isEdit
                            }
                            QGCSlider {
                                // Layout.fillWidth:           true
                                maximumValue:               parent._fact.max
                                minimumValue:               parent._fact.min
                                stepSize:                   parent._fact.increment
                                visible:                    parent._isSlider
                                updateValueWhileDragging:   false
                                property bool initialized:  false

                                onValueChanged: {
                                    if (!initialized) {
                                        return
                                    }
                                    parent._fact.value = value
                                }

                                Component.onCompleted: {
                                    value = parent._fact.value
                                    initialized = true
                                }
                            }
                            QGCSwitch {
                                checked:        parent._fact ? parent._fact.value : false
                                visible:        parent._isBool
                                onClicked:      parent._fact.value = checked ? 1 : 0
                            }
                        }
                    }

                    QGCComboBox {
                        // Layout.fillWidth:   true
                        sizeToContents:     true
                        model:              [ qsTr("Single"), qsTr("Time Lapse") ]
                        currentIndex:       _mavlinkCamera ? _mavlinkCamera.photoMode : 0
                        visible:            _mavlinkCameraHasModes
                        onActivated:        _mavlinkCamera.photoMode = index
                    }

                    QGCSlider {
                        // Layout.fillWidth:           true
                        maximumValue:               60
                        minimumValue:               1
                        stepSize:                   1
                        value:                      _mavlinkCamera ? _mavlinkCamera.photoLapse : 5
                        displayValue:               true
                        updateValueWhileDragging:   true
                        visible:                    _mavlinkCameraInPhotoMode && _mavlinkCamera.photoMode === QGCCameraControl.PHOTO_CAPTURE_TIMELAPSE
                        onValueChanged: {
                            if (_mavlinkCamera) {
                                _mavlinkCamera.photoLapse = value
                            }
                        }
                    }

                    QGCSwitch {
                        checked:            _videoStreamSettings.gridLines.rawValue
                        visible:            _anyVideoStreamAvailable
                        onClicked:          _videoStreamSettings.gridLines.rawValue = checked ? 1 : 0
                    }

                    FactComboBox {
                        // Layout.fillWidth:   true
                        sizeToContents:     true
                        fact:               _videoStreamSettings.videoFit
                        indexModel:         false
                        visible:            _anyVideoStreamAvailable
                    }

                    QGCButton {
                        // Layout.fillWidth:   true
                        text:               qsTr("Reset")
                        visible:            _mavlinkCamera
                        onClicked:          resetPrompt.open()
                        MessageDialog {
                            id:                 resetPrompt
                            title:              qsTr("Reset Camera to Factory Settings")
                            text:               qsTr("Confirm resetting all settings?")
                            standardButtons:    StandardButton.Yes | StandardButton.No
                            onNo: resetPrompt.close()
                            onYes: {
                                _mavlinkCamera.resetSettings()
                                resetPrompt.close()
                            }
                        }
                    }

                    QGCButton {
                        // Layout.fillWidth:   true
                        text:               qsTr("Format")
                        visible:            _mavlinkCameraStorageSupported
                        onClicked:          formatPrompt.open()
                        MessageDialog {
                            id:                 formatPrompt
                            title:              qsTr("Format Camera Storage")
                            text:               qsTr("Confirm erasing all files?")
                            standardButtons:    StandardButton.Yes | StandardButton.No
                            onNo: formatPrompt.close()
                            onYes: {
                                _mavlinkCamera.formatCard()
                                formatPrompt.close()
                            }
                        }
                    }
                }
                }
            }
        }
    }
}
