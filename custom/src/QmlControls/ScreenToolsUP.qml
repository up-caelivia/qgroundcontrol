pragma Singleton

import QtQuick          2.12
import QtQuick.Window   2.2

import QGroundControl                       1.0
import QGroundControl.ScreenToolsController 1.0

Item {
    id: _screenTools

    //-- Sizing input and internal state 

    property var sizingController: null
    property bool _fontSizingInitialized: false
    readonly property bool usingSizingController: sizingController !== null && sizingController.valid

    property real _legacyPlatformFontPointSize: 10

    //-- Platform information
    
    property bool isAndroid:                        ScreenToolsController.isAndroid
    property bool isiOS:                            ScreenToolsController.isiOS
    property bool isMobile:                         ScreenToolsController.isMobile
    property bool isWindows:                        ScreenToolsController.isWindows
    property bool isDebug:                          ScreenToolsController.isDebug
    property bool isMac:                            ScreenToolsController.isMacOS
    property bool isLinux:                          ScreenToolsController.isLinux
    property bool isSerialAvailable:                ScreenToolsController.isSerialAvailable

    property bool isTinyScreen:                     (Screen.width / realPixelDensity) < 120
    property bool isShortScreen:                    ((Screen.height / realPixelDensity) < 120) || (ScreenToolsController.isMobile && ((Screen.height / Screen.width) < 0.6))
    property bool isHugeScreen:                     (Screen.width / realPixelDensity) >= (23.5 * 25.4)

    property real realPixelDensity: {
        var definedDensity = QGroundControl.corePlugin.options.devicePixelDensity 

        if(definedDensity != 0) {
            return definedDensity
        }

        if(isAndroid && (Screen.width / Screen.pixelDensity) > 300) {
            return Screen.pixelDensity * 2
        }

        return Screen.pixelDensity
    }

    //-- Font size information 

    readonly property string normalFontFamily:      ScreenToolsController.normalFontFamily
    readonly property string demiboldFontFamily:    ScreenToolsController.boldFontFamily
    readonly property string fixedFontFamily:       ScreenToolsController.fixedFontFamily

    readonly property real smallFontPointRatio:      0.75
    readonly property real mediumFontPointRatio:     1.25
    readonly property real largeFontPointRatio:      1.5

    property real smallFontPointSize:       10
    property real mediumFontPointSize:      10
    property real largeFontPointSize:       10

    property real defaultFontPointSize:     10
    property real platformFontPointSize:    10

    property real defaultFontPixelHeight:   10
    property real defaultFontPixelWidth:    10
    property real defaultFontDescent:       0

    //-- Derived dimensions

    readonly property real minTouchMillimeters:     10
    property real minTouchPixels:                   0

    property real defaultDialogControlSpacing: defaultFontPixelHeight / 2

    property real toolbarHeight:            0

    property real implicitButtonWidth:              Math.round(defaultFontPixelWidth *  (isMobile ? 7.0 : 5.0))
    property real implicitButtonHeight:             Math.round(defaultFontPixelHeight * (isMobile ? 2.0 : 1.6))
    property real implicitCheckBoxHeight:           Math.round(defaultFontPixelHeight * (isMobile ? 2.0 : 1.0))
    property real implicitRadioButtonHeight:        implicitCheckBoxHeight
    property real implicitTextFieldHeight:          implicitButtonHeight
    property real implicitComboBoxHeight:           implicitButtonHeight
    property real implicitComboBoxWidth:            implicitButtonWidth
    property real comboBoxPadding:                  defaultFontPixelWidth
    property real implicitSliderHeight:             isMobile ? Math.max(defaultFontPixelHeight, minTouchPixels) : defaultFontPixelHeight
    property real checkBoxIndicatorSize:            2 * Math.floor(defaultFontPixelHeight * (isMobile ? 1.5 : 1.0) / 2) + 1
    property real radioButtonIndicatorSize:         checkBoxIndicatorSize

    //-- Font measurement

    Text {
        id:     _defaultFont
        text:   "X"
    }

    Text {
        id:     _textMeasure

        text:   "X"
        font.family:    normalFontFamily

        Component.onCompleted: {
            _screenTools._initializeFontSizing()
        }
    }

    //-- Change notifications 

    Connections {
        target: QGroundControl.settingsManager.appSettings.appFontPointSize

        function onValueChanged() {
            _screenTools._updateFontSizing()
        }
    }

    Connections {
        target: _screenTools.sizingController

        function onSizingChanged() {
            _screenTools._updateFontSizing()
        }
    }

    // Reacts to the creation of the biding
    onSizingControllerChanged: _updateFontSizing()
    // Changes of physical density
    onRealPixelDensityChanged: _updateFontSizing()

    //-- Internal sizing functions 

    function _initializeFontSizing() {
        if (!_screenTools.isMobile) {
            platformFontPointSize = _defaultFont.font.pointSize
        } else if (_screenTools.isiOS) {
            platformFontPointSize = (Screen.width < 570) ? 12 : 14
        } else {
            platformFontPointSize = (Screen.width / realPixelDensity) < 120 ? 11 : 14
        }

        _legacyPlatformFontPointSize = platformFontPointSize

        var fontPointSizeFact = QGroundControl.settingsManager.appSettings.appFontPointSize
        var sizeSaved = fontPointSizeFact.value

        if(sizeSaved < fontPointSizeFact.min || sizeSaved > fontPointSizeFact.max) {
            fontPointSizeFact.value = platformFontPointSize 
        }

        _fontSizingInitialized = true
        _screenTools._updateFontSizing();
    }

    function _updateFontSizing() {
        if (!_fontSizingInitialized) {
            return
        }

        if (usingSizingController) {
            platformFontPointSize = sizingController.baselinePointSize
            _setBasePointSize(sizingController.effectivePointSize)
        } else {
            platformFontPointSize = _legacyPlatformFontPointSize
            _setBasePointSize(
                QGroundControl.settingsManager.appSettings.appFontPointSize.value)
        }
    }

    function _setBasePointSize(pointSize) {
        _textMeasure.font.pointSize = pointSize
        defaultFontPointSize    = pointSize
        defaultFontPixelHeight  = Math.round(_textMeasure.contentHeight / 2.0) * 2
        defaultFontPixelWidth   = Math.round(_textMeasure.contentWidth / 2.0) * 2
        defaultFontDescent      = ScreenToolsController.defaultFontDescent(defaultFontPointSize)
        smallFontPointSize      = defaultFontPointSize  * _screenTools.smallFontPointRatio
        mediumFontPointSize     = defaultFontPointSize  * _screenTools.mediumFontPointRatio
        largeFontPointSize      = defaultFontPointSize  * _screenTools.largeFontPointRatio
        var physicalTouchPixels = Math.round(minTouchMillimeters * realPixelDensity)
        minTouchPixels          = physicalTouchPixels / Screen.height > 0.15 
                                    ? defaultFontPixelHeight * 3
                                    : physicalTouchPixels
        var baseToolbarHeight   = isMobile ? minTouchPixels : defaultFontPixelHeight * 3
        toolbarHeight           = QGroundControl.corePlugin.options.toolbarHeightMultiplier * baseToolbarHeight
    }

    function printScreenStats() {
        console.log('ScreenTools: Screen.width: ' + Screen.width + ' Screen.height: ' + Screen.height + ' Screen.pixelDensity: ' + Screen.pixelDensity)
    }

    function mouseX() {
        return ScreenToolsController.mouseX()
    }

    function mouseY() {
        return ScreenToolsController.mouseY()
    }
}
