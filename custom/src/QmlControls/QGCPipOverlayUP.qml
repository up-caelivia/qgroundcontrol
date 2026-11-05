/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick                      2.12
import QtQuick.Window               2.12
import QtQml                        2.15
import QtGraphicalEffects           1.0

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0

Item {
    id:         _root
    width:      Math.max(_minimumPipWidth, Math.min(_pipSize, _maximumPipWidth))
    height:     width * (9/16)
    z:          pipZOrder + 1
    visible:    show && _pipOrWindowItem && _pipOrWindowItem.pipState.state !== _pipOrWindowItem.pipState.windowState

    property var    item1:                  null    // Required
    property var    item2:                  null    // Optional, may come and go
    property string item1IsFullSettingsKey          // Settings key to save whether item1 was saved in full mode
    property real   fullZOrder:             0       // zOrder for items in full mode
    property real   pipZOrder:              1       // zOrder for items in pip mode
    property bool   show:                   true
    property real   topInset:               0       // Reserved height in parent coordinates
    property real   cornerRadius:           ScreenTools.defaultFontPixelWidth * 1.5
    readonly property real occupiedWidth: visible ? (_isExpanded ? width : hidePIP.width) + ScreenTools.defaultFontPointSize : 0
    readonly property real occupiedHeight: visible ? (_isExpanded ? height : hidePIP.height) + ScreenTools.defaultFontPointSize : 0

    readonly property string _pipExpandedSettingsKey: "IsPIPVisible"

    property var    _fullItem
    property var    _pipOrWindowItem
    property alias  _windowContentItem: window.contentItem
    property bool   _isExpanded:        true
    property real   _pipSize:           parent.width * 0.3
    property real   _maxSize:           0.5
    property real   _minSize:           0.1
    readonly property real _maximumPipWidth: Math.max(0, Math.min(
        parent.width * _maxSize,
        parent.width - anchors.leftMargin - anchors.rightMargin,
        (parent.height - topInset - anchors.bottomMargin) * (16/9)
    ))
    readonly property real _minimumPipWidth: Math.min(
        parent.width * _minSize,
        _maximumPipWidth
    )
    property real maximumIconSize: ScreenTools.defaultFontPixelHeight * 2.5
    readonly property real preferredSize: Math.min(
        _root.height * 0.30,
        maximumIconSize
    )

    readonly property var _pipLayer: _pipOrWindowItem ? _pipOrWindowItem.layer : null
    readonly property bool _roundPipContent: show && _isExpanded && !!_pipOrWindowItem &&
                                            _pipOrWindowItem.pipState.state === _pipOrWindowItem.pipState.pipState

    Component {
        id: pipMaskEffect

        OpacityMask {
            id: pipOpacityMask

            maskSource: Rectangle {
                width: pipOpacityMask.width
                height: pipOpacityMask.height
                radius: _root.cornerRadius
                color: "white"
                antialiasing: true
                visible: false
            }
        }
    }

    Binding {
        target: _root._pipLayer
        property: "effect"
        value: pipMaskEffect
        when: _root._roundPipContent
        restoreMode: Binding.RestoreBindingOrValue
    }

    Binding {
        target: _root._pipLayer
        property: "smooth"
        value: true
        when: _root._roundPipContent
        restoreMode: Binding.RestoreBindingOrValue
    }

    Binding {
        target: _root._pipLayer
        property: "enabled"
        value: true
        when: _root._roundPipContent
        restoreMode: Binding.RestoreBindingOrValue
    }

    Component.onCompleted: {
        _initForItems()
    }

    onShowChanged: {
        if (_pipOrWindowItem && _pipOrWindowItem.pipState.state !== _pipOrWindowItem.pipState.windowState) {
            _pipOrWindowItem.visible = show
        }
    }

    onItem2Changed: _initForItems()

    function showWindow() {
        window.width = _root.width
        window.height = _root.height
        window.show()
    }

    function _initForItems() {
        var item1IsFull = QGroundControl.loadBoolGlobalSetting(item1IsFullSettingsKey, true)
        if (item1 && item2) {
            item1.pipState.state = item1IsFull ? item1.pipState.fullState : item1.pipState.pipState
            item2.pipState.state = item1IsFull ? item2.pipState.pipState : item2.pipState.fullState
            _fullItem = item1IsFull ? item1 : item2
            _pipOrWindowItem = item1IsFull ? item2 : item1
        } else {
            item1.pipState.state = item1.pipState.fullState
            _fullItem = item1
            _pipOrWindowItem = null
            item1.visible = true
        }
        _setPipIsExpanded(QGroundControl.loadBoolGlobalSetting(_pipExpandedSettingsKey, true))
    }

    function _swapPip() {
        var item1IsFull = false
        if (item1.pipState.state === item1.pipState.fullState) {
            item1.pipState.state = item1.pipState.pipState
            item2.pipState.state = item2.pipState.fullState
            _fullItem = item2
            _pipOrWindowItem = item1
            item1IsFull = false
        } else {
            item1.pipState.state = item1.pipState.fullState
            item2.pipState.state = item2.pipState.pipState
            _fullItem = item1
            _pipOrWindowItem = item2
            item1IsFull = true
        }
        QGroundControl.saveBoolGlobalSetting(item1IsFullSettingsKey, item1IsFull)
    }

    function _setPipIsExpanded(isExpanded) {
        QGroundControl.saveBoolGlobalSetting(_pipExpandedSettingsKey, isExpanded)
        _isExpanded = isExpanded
        if (_pipOrWindowItem) {
            _pipOrWindowItem.visible = isExpanded
        }
    }

    Window {
        id:         window
        visible:    false
        onClosing: {
            var item = contentItem.children[0]
            item.pipState.windowAboutToClose()
            item.pipState.state = item.pipState.windowClosingState
            item.pipState.state = item.pipState.pipState
            item.visible = _root.show
        }
    }

    MouseArea {
        id:             pipMouseArea
        anchors.fill:   parent
        enabled:        _isExpanded
        hoverEnabled:   true
        onClicked:      {
            _swapPip();
        }
    }

    // // MouseArea to drag in order to resize the PiP area
    MouseArea {
        id:             pipResize

        anchors.right:   parent.right
        anchors.top: parent.top

        height:         ScreenTools.minTouchPixels
        width:          ScreenTools.minTouchPixels

        property real initialX:     0
        property real initialWidth: 0
        property bool allowLess: false


        // When we push the mouse button down, we un-anchor the mouse area to prevent a resizing loop
        onPressed: {
            pipResize.anchors.top = undefined // Tleftop doesn't seem to 'detach'
            pipResize.anchors.right = undefined // This one works right, which is what we really need

            var positionInWindow = mapToItem(window.contentItem, mouse.x, mouse.y)

            pipResize.initialX = positionInWindow.x
            pipResize.initialWidth = _root.width

        }

        // When we let go of the mouse button, we re-anchor the mouse area in the correct position
        onReleased: {
            pipResize.anchors.top = _root.top
            pipResize.anchors.right = _root.right
        }

        // Drag
        onPositionChanged: {
            if (pipResize.pressed) {

                var positionInWindow = mapToItem(window.contentItem, mouse.x, mouse.y)
                var newWidth = pipResize.initialWidth + (positionInWindow.x - pipResize.initialX)

                _pipSize = Math.max(_minimumPipWidth, Math.min(newWidth, _maximumPipWidth))
            }
        }
    }

    // Resize icon
    Image {
        source: "/qmlimages/pipResize.svg"
        fillMode: Image.PreserveAspectFit
        mipmap: true

        height: preferredSize
        width: preferredSize
        sourceSize.height: height
        sourceSize.width: width

        anchors.right: parent.right
        anchors.top: parent.top

        visible: _isExpanded && (ScreenTools.isMobile || pipMouseArea.containsMouse)
    }

    // Pip to Window
    Image {
        source:         "/qmlimages/PiP.svg"
        mipmap:         true
        fillMode:       Image.PreserveAspectFit

        height: preferredSize
        width: preferredSize
        sourceSize.height: height
        sourceSize.width: width

        anchors.left:   parent.left
        anchors.top:    parent.top

        visible:        _isExpanded && !ScreenTools.isMobile && pipMouseArea.containsMouse

        MouseArea {
            anchors.fill:   parent
            onClicked:      _pipOrWindowItem.pipState.state = _pipOrWindowItem.pipState.windowState
        }
    }

    Rectangle {
        id: hidePIP

        anchors.left: parent.left
        anchors.bottom: parent.bottom

        height:                 ScreenTools.defaultFontPixelHeight * 2
        width:                  ScreenTools.defaultFontPixelHeight * 2
        radius:                 ScreenTools.defaultFontPixelHeight / 3
        visible: !_isExpanded
        color: _fullItem.pipState.isDark ? Qt.rgba(0,0,0,0.75) : Qt.rgba(0,0,0,0.5)

        Image {
            width: parent.width  * 0.75
            height: parent.height * 0.75
            sourceSize.height: height
            source: "/res/buttonRight.svg"
            mipmap: true
            fillMode: Image.PreserveAspectFit
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
        }

        MouseArea {
            anchors.fill:   parent
            onClicked:      _root._setPipIsExpanded(true)
        }
    }

    Image {
        source: "/qmlimages/pipHide.svg"
        mipmap: true
        fillMode: Image.PreserveAspectFit

        height: preferredSize
        width: preferredSize
        sourceSize.height: height
        sourceSize.width: width

        anchors.left: parent.left
        anchors.bottom: parent.bottom

        visible: _isExpanded && (ScreenTools.isMobile || pipMouseArea.containsMouse)

        MouseArea {
            anchors.fill: parent
            onClicked: _root._setPipIsExpanded(false)
        }
    }
}
