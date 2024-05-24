/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick                      2.3
import QtQuick.Controls             1.2
import QtQuick.Dialogs              1.2
import QtQuick.Layouts              1.2

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controllers   1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0

import Custom.Constants 1.0




Item {
    id:         _root

    property Fact   _editorDialogFact: Fact { }
    property int _indexSelected: 0

    property int    _rowHeight:         ScreenTools.defaultFontPixelHeight * 2
    property int    _rowWidth:          10 // Dynamic adjusted at runtime
    property var    _searchResults      ///< List of parameter names from search results
    property var    _activeVehicle:     QGroundControl.multiVehicleManager.activeVehicle
    property bool   _showRCToParam:     _activeVehicle.px4Firmware
    property var    _appSettings:       QGroundControl.settingsManager.appSettings
    property var    _controller:        controller

    property bool   _searchFilter:      searchText.text.trim() != "" || controller.showModifiedOnly  ///< true: showing results of search
    property list<Fact> factList

    property var factNames: ["LOIT_SPEED", "WPNAV_SPEED", "FENCE_ALT_MAX"]
    property var factDescription: ["Maximum speed reached by drone in loiter mode. Warning: in altitude hold mode speed is not limited", "Maximum speed reached by drone during automatic mission", ""]
    property var factGoodNames: ["Maximum loiter speed", "Maximum auto speed", "Maximum altitude"]
    property var factMin: [200, 200, 10]
    property var factMax: [1500, 1500, 200]
    property var factEditable: [false, true, true]

    property bool developer: Constants.developer

    ParameterEditorController {
        id: controller
    }


    ExclusiveGroup { id: sectionGroup }


    //---------------------------------------------
    //-- Header
    Row {
        id:             header
        anchors.left:   parent.left
        anchors.right:  parent.right
        spacing:        ScreenTools.defaultFontPixelWidth
        visible: developer

        Timer {
            id:         clearTimer
            interval:   100;
            running:    false;
            repeat:     false
            onTriggered: {
                searchText.text = ""
                controller.searchText = ""
                console.log(developer)
            }
        }

        QGCLabel {
            anchors.verticalCenter: parent.verticalCenter
            text: qsTr("Search:")
        }

        QGCTextField {
            id:                 searchText
            text:               controller.searchText
            onDisplayTextChanged: controller.searchText = displayText
            anchors.verticalCenter: parent.verticalCenter
        }

        QGCButton {
            text: qsTr("Clear")
            onClicked: {
                if(ScreenTools.isMobile) {
                    Qt.inputMethod.hide();
                }
                clearTimer.start()
            }
            anchors.verticalCenter: parent.verticalCenter
        }

        QGCCheckBox {
            text:                   qsTr("Show modified only")
            anchors.verticalCenter: parent.verticalCenter
            checked:                controller.showModifiedOnly
            onClicked:              controller.showModifiedOnly = checked
            visible:                QGroundControl.multiVehicleManager.activeVehicle.px4Firmware
        }
    } // Row - Header




    QGCButton {
        id: tools
        // anchors.top:    header.top
        // anchors.bottom: header.bottom
        anchors.right:  parent.right
        anchors.rightMargin: ScreenTools.defaultFontPixelWidth
        text:           qsTr("Tools")
       // visible:        !_searchFilter
        onClicked:      toolsMenu.popup()
    }

    QGCMenu {
        id:                 toolsMenu
        QGCMenuItem {
            text:           qsTr("Refresh")
            onTriggered:	controller.refresh()
        }
        QGCMenuItem {
            text:           qsTr("Reset all to firmware's defaults")
            onTriggered:    mainWindow.showMessageDialog(qsTr("Reset All"),
                                                         qsTr("Select Reset to reset all parameters to their defaults.\n\nNote that this will also completely reset everything, including UAVCAN nodes, all vehicle settings, setup and calibrations."),
                                                         StandardButton.Cancel | StandardButton.Reset,
                                                         function() { controller.resetAllToDefaults() })
        }
        QGCMenuItem {
            text:           qsTr("Reset to vehicle's configuration defaults")
            visible:        !_activeVehicle.apmFirmware
            onTriggered:    mainWindow.showMessageDialog(qsTr("Reset All"),
                                                         qsTr("Select Reset to reset all parameters to the vehicle's configuration defaults."),
                                                         StandardButton.Cancel | StandardButton.Reset,
                                                         function() { controller.resetAllToVehicleConfiguration() })
        }
        QGCMenuSeparator { }
        QGCMenuItem {
            text:           qsTr("Load from file...")
            onTriggered: {
                fileDialog.title =          qsTr("Load Parameters")
                fileDialog.selectExisting = true
                fileDialog.openForLoad()
            }
        }
        QGCMenuItem {
            text:           qsTr("Save to file...")
            onTriggered: {
                fileDialog.title =          qsTr("Save Parameters")
                fileDialog.selectExisting = false
                fileDialog.openForSave()
            }
        }
        QGCMenuSeparator { visible: _showRCToParam }
        QGCMenuItem {
            text:           qsTr("Clear all RC to Param")
            onTriggered:	_activeVehicle.clearAllParamMapRC()
            visible:        _showRCToParam
        }
        QGCMenuSeparator { }
        QGCMenuItem {
            text:           qsTr("Reboot Vehicle")
            onTriggered:    mainWindow.showMessageDialog(qsTr("Reboot Vehicle"),
                                                         qsTr("Select Ok to reboot vehicle."),
                                                         StandardButton.Cancel | StandardButton.Ok,
                                                         function() { _activeVehicle.rebootVehicle() })
        }
    }



    /// Group buttons
    QGCFlickable {
        id :                groupScroll
        width:              ScreenTools.defaultFontPixelWidth * 25
        anchors.top:        header.bottom
        anchors.bottom:     parent.bottom
        clip:               true
        pixelAligned:       true
        contentHeight:      groupedViewCategoryColumn.height
        flickableDirection: Flickable.VerticalFlick
        visible:            (!_searchFilter) && (developer)

        ColumnLayout {
            id:             groupedViewCategoryColumn
            anchors.left:   parent.left
            anchors.right:  parent.right
            spacing:        Math.ceil(ScreenTools.defaultFontPixelHeight * 0.25)

            Repeater {
                model: controller.categories

                Column {
                    Layout.fillWidth:   true
                    spacing:            Math.ceil(ScreenTools.defaultFontPixelHeight * 0.25)


                    SectionHeader {
                        id:             categoryHeader
                        anchors.left:   parent.left
                        anchors.right:  parent.right
                        text:           object.name
                        checked:        object == controller.currentCategory
                        exclusiveGroup: sectionGroup

                        onCheckedChanged: {
                            if (checked) {
                                controller.currentCategory  = object
                            }
                        }
                    }

                    Repeater {
                        model: categoryHeader.checked ? object.groups : 0

                        QGCButton {
                            width:          ScreenTools.defaultFontPixelWidth * 25
                            text:           object.name
                            height:         _rowHeight
                            checked:        object == controller.currentGroup
                            autoExclusive:  true

                            onClicked: {
                                if (!checked) _rowWidth = 10
                                checked = true
                                controller.currentGroup = object
                            }
                        }
                    }
                }
            }
        }
    }










    // -----------------------------------------------------------------------------------------------------------------------------

    function getModel() {

        for (var i = 0; i < factNames.length; ++i) {

            var name = factNames[i]
            controller.searchText = name

            for( var j = 0; j < controller.parameters.rowCount(); j++ ) {

                if ( controller.parameters.get(j).name == name)  {
                    factList.push(controller.parameters.get(j))
                    //controller.parameters.get(j).shortDescription = "Maximum speed reached by drone in loiter mode. Warning: in altitude hold mode speed is not limited."

                    break
                }
            }

        }
    }



    Component.onCompleted: {
        if (!developer)
          getModel() // Trigger the initial population of factList
      }

    function getText(modelFact)   {
               if(modelFact.enumStrings.length === 0) {

                   if(developer)
                       return modelFact.valueString + " " + modelFact.units

                   if (modelFact.units == "cm/s")
                       return (modelFact.rawValue / 100).toFixed(2) + " " + "m/s"

                   return modelFact.valueString + " " + modelFact.units

               }

               if(modelFact.bitmaskStrings.length != 0) {
                   return modelFact.selectedBitmaskStrings.join(',')
               }

               return modelFact.enumStringValue
    }


    /// Parameter list
    QGCListView {
        id:                 editorListView
        anchors.leftMargin: ScreenTools.defaultFontPixelWidth
        anchors.left:       developer ? groupScroll.right  : parent.left
        anchors.right:      parent.right
        anchors.top:        tools.bottom
        anchors.bottom:     parent.bottom
        orientation:        ListView.Vertical
        model:              developer ? controller.parameters : factList
        cacheBuffer:        height > 0 ? height * 2 : 0
        clip:               true

        delegate: Rectangle {
            id: itemDelegate
            height: developer ? _rowHeight : descriptionId.height + ScreenTools.defaultFontPixelHeight //_rowHeight
            width:  _rowWidth
            color:  Qt.rgba(0,0,0,0)
            anchors.horizontalCenter: developer ? undefined : parent.horizontalCenter

            Row {
                id:     factRow
                spacing: Math.ceil(ScreenTools.defaultFontPixelWidth * 0.5)
                anchors.verticalCenter: parent.verticalCenter

                property Fact modelFact: developer ? object : factList[index]

                QGCLabel {
                    id:     nameLabel
                    width:  ScreenTools.defaultFontPixelWidth  * 20
                    text:   factGoodNames[index]//factRow.modelFact.name
                    clip:   true
                }

                QGCLabel {
                    id:     valueLabel
                    width:  ScreenTools.defaultFontPixelWidth  * 20
                    color:  getColor()
                    text:  getText(factRow.modelFact)
                    clip:   true

                    function getColor() {

                        if (factGoodNames[index] == "Maximum altitude" && parseFloat(factRow.modelFact.value) > 120)
                            return qgcPal.warningText
                        return qgcPal.text

                    }

                }

                QGCLabel {
                    id: descriptionId
                    text: getDesc()
                    width: ScreenTools.defaultFontPixelWidth  * 30
                    wrapMode: developer ? Text.NoWrap : Text.Wrap

                    function getDesc() {
                        if(developer || factDescription[index] == "")
                            return factRow.modelFact.shortDescription
                        return factDescription[index]
                    }
                }

                Component.onCompleted: {
                    if(_rowWidth < factRow.width + ScreenTools.defaultFontPixelWidth) {
                        _rowWidth = factRow.width + ScreenTools.defaultFontPixelWidth
                    }
                }
            }

            Rectangle {
                width:  _rowWidth
                height: 1
                color:  qgcPal.text
                opacity: 0.15
                anchors.bottom: parent.bottom
                anchors.left:   parent.left
            }

            MouseArea {
                anchors.fill:       parent
                acceptedButtons:    Qt.LeftButton
                enabled: factEditable[index]

                onClicked: {
                    _editorDialogFact = factRow.modelFact
                    _indexSelected = index
                    editorDialogComponent.createObject(mainWindow).open()
                }
            }
        }
    }

    QGCFileDialog {
        id:             fileDialog
        folder:         _appSettings.parameterSavePath
        nameFilters:    [ qsTr("Parameter Files (*.%1)").arg(_appSettings.parameterFileExtension) , qsTr("All Files (*)") ]

        onAcceptedForSave: {
            controller.saveToFile(file)
            close()
        }

        onAcceptedForLoad: {
            close()
            if (controller.buildDiffFromFile(file)) {
                parameterDiffDialog.createObject(mainWindow).open()
            }
        }
    }

    Component {
        id: editorDialogComponent


        ParameterEditorDialog {
            fact:           _editorDialogFact
            showRCToParam:  _showRCToParam
            developer: developer
            max: factMax[_indexSelected]
            min: factMin[_indexSelected]
        }
    }

    Component {
        id: parameterDiffDialog

        ParameterDiffDialog {
            paramController: _controller
        }
    }
}
