/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.12
import QtQuick.Controls 2.4
import QtQuick.Layouts  1.11
import QtQuick.Dialogs  1.3

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.Palette               1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Controllers           1.0
import Custom.Widgets 1.0


Rectangle {
    id:     _root
    color:  qgcPal.toolbarBackground


    property int currentToolbar: flyViewToolbar

    readonly property int flyViewToolbar:   0
    readonly property int planViewToolbar:  1
    readonly property int simpleToolbar:    2

    property var    _activeVehicle:     QGroundControl.multiVehicleManager.activeVehicle
    property bool   _communicationLost: _activeVehicle ? _activeVehicle.vehicleLinkManager.communicationLost : false
    property color  _mainStatusBGColor: qgcPal.brandingPurple

    function dropMessageIndicatorTool() {
        if (currentToolbar === flyViewToolbar) {
            indicatorLoader.item.dropMessageIndicatorTool();
        }
    }

    QGCPalette { id: qgcPal }

    /// Bottom single pixel divider
    Rectangle {
        anchors.left:   parent.left
        anchors.right:  parent.right
        anchors.bottom: parent.bottom
        height:         1
        color:          "black"
        visible:        qgcPal.globalTheme === QGCPalette.Light
    }

    RowLayout {
        id:                     viewButtonRow
        anchors.bottomMargin:   1
        anchors.top:            parent.top
        anchors.bottom:         parent.bottom
        spacing:                ScreenTools.defaultFontPixelWidth / 2

        //left logo
        QGCToolBarButton {
            id:                     currentButton
            Layout.preferredHeight: viewButtonRow.height
            icon.source:            "/res/QGCLogoFull"
            logo:                   true
            onClicked:              mainWindow.showToolSelectDialog()
        }

        QGCButton {

            id:                 connectButton
            text:               qsTr("Connect")
            visible:            !_activeVehicle && 0
            enabled:            getEnable() //count > 1  && (obj == undefined || obj.link != null)// the first one is at 14550

            property var count: model.count
            property var model: QGroundControl.linkManager.linkConfigurations

            function getEnable() {

                for (var i = 0; i < count; i++)
                    if (model.get(i).localPort == 14551 && !model.get(i).link) {
                        return true
                    }

                return false
            }


            onClicked:
            {                
                for (var i = 0; i < count; i++)
                    if (model.get(i).localPort == 14551 && !model.get(i).link) {
                        QGroundControl.linkManager.createConnectedLink(model.get(i))
                        console.log("Try to connect")
                        break
                    }
            }
        }

        QGCButton {
            id:                 disconnectButton
            text:               qsTr("Disconnect")
            onClicked:          _activeVehicle.closeVehicle()
            visible:            _activeVehicle && _communicationLost && currentToolbar === flyViewToolbar
        }
    }



    // manual and the other on the right
    MainStatusIndicator {

        id: mainIndicator
        x: parent.width - width - ScreenTools.defaultFontPixelWidth * 2
        anchors.verticalCenter: parent.verticalCenter
        visible:                currentToolbar === flyViewToolbar
        //color: _mainStatusBGColor
        // function getX(){
        //     return brand_image.visible ? parent.width - width - brand_image.width - 40 : parent.width - width - 10
        // }
    }


    // buttons scrollable list
    QGCFlickable {
        id:                     toolsFlickable
        anchors.leftMargin:     ScreenTools.defaultFontPixelWidth * ScreenTools.largeFontPointRatio * 1.5
        anchors.left:           viewButtonRow.right
        anchors.bottomMargin:   1
        anchors.top:            parent.top
        anchors.bottom:         parent.bottom
        anchors.right:          mainIndicator.left
        contentWidth:           indicatorLoader.x + indicatorLoader.width
        flickableDirection:     Flickable.HorizontalFlick

        Loader {
            id:                 indicatorLoader
            anchors.left:       parent.left
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            source:             currentToolbar === flyViewToolbar ?
                                    "qrc:/toolbar/MainToolBarIndicators.qml" :
                                    (currentToolbar == planViewToolbar ? "qrc:/qml/PlanToolBarIndicators.qml" : "")
        }

        FlightTimeWidget {
            id: tf
            anchors.left:       indicatorLoader.right
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            // anchors.leftMargin: 10
            anchors.margins: ScreenTools.defaultFontPixelHeight * 0.66
            visible:            _activeVehicle && currentToolbar === flyViewToolbar
        }
    }

    // Small parameter download progress bar
    Rectangle {
        anchors.bottom: parent.bottom
        height:         _root.height * 0.05
        width:          _activeVehicle ? _activeVehicle.loadProgress * parent.width : 0
        color:          qgcPal.colorGreen
        visible:        !largeProgressBar.visible
    }

    // Large parameter download progress bar
    Rectangle {
        id:             largeProgressBar
        anchors.bottom: parent.bottom
        anchors.left:   parent.left
        anchors.right:  parent.right
        height:         parent.height
        color:          qgcPal.window
        visible:        _showLargeProgress

        property bool _initialDownloadComplete: _activeVehicle ? _activeVehicle.initialConnectComplete : true
        property bool _userHide:                false
        property bool _showLargeProgress:       !_initialDownloadComplete && !_userHide && qgcPal.globalTheme === QGCPalette.Light

        Connections {
            target:                 QGroundControl.multiVehicleManager
            function onActiveVehicleChanged(activeVehicle) { largeProgressBar._userHide = false }
        }

        Rectangle {
            anchors.top:    parent.top
            anchors.bottom: parent.bottom
            width:          _activeVehicle ? _activeVehicle.loadProgress * parent.width : 0
            color:          qgcPal.colorGreen
        }

        QGCLabel {
            anchors.centerIn:   parent
            text:               qsTr("Downloading")
            font.pointSize:     ScreenTools.largeFontPointSize
        }

        QGCLabel {
            anchors.margins:    _margin
            anchors.right:      parent.right
            anchors.bottom:     parent.bottom
            text:               qsTr("Click anywhere to hide")

            property real _margin: ScreenTools.defaultFontPixelWidth / 2
        }

        MouseArea {
            anchors.fill:   parent
            onClicked:      largeProgressBar._userHide = true
        }
    }
}
