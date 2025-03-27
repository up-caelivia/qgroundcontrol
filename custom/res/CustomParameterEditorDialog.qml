/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.3
import QtQuick.Controls 2.15
import QtQuick.Layouts  1.15
import QtQuick.Dialogs  1.3

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controllers   1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.ScreenTools   1.0

import Constants 1.0


QGCPopupDialog {
    id:         root
    title:      qsTr("Parameter Editor")
    buttons:    StandardButton.Cancel | StandardButton.Save
   // width: _editFieldWidth * 2.5

    property Fact   fact
    property bool   showRCToParam:  false
    property bool   validate:       false
    property string validateValue
    property bool   setFocus:       true    ///< true: focus is set to text field on display, false: focus not set (works around strange virtual keyboard bug with FactValueSlider

    signal valueChanged

    property real   _editFieldWidth:            ScreenTools.defaultFontPixelWidth * 20
    property bool   _longDescriptionAvailable:  fact.longDescription != ""
    property bool   _editingParameter:          fact.componentId != 0
    property bool   _allowForceSave:            QGroundControl.corePlugin.showAdvancedUI || !_editingParameter
    property bool   _allowDefaultReset:         fact.defaultValueAvailable && (QGroundControl.corePlugin.showAdvancedUI || !_editingParameter)
    property bool   _showCombo:                 fact.enumStrings.length !== 0 && fact.bitmaskStrings.length === 0 && !validate

    property int selectedIndex: -1
    property double max: selectedIndex == -1 ? 0 : Constants.factMax[selectedIndex]
    property double min: selectedIndex == -1 ? 0 : Constants.factMin[selectedIndex]
    property bool developer: Constants.developer
    property bool factor: ( fact.units == "cm" || fact.units == "cm/s" ) ? true : false

    ParameterEditorController { id: controller; }

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    property var oldValue: fact.value

    onAccepted: {

        var errStringCustom = developer || selectedIndex == -1 ? "" : customControl()
        //console.log(selectedIndex)

        if (validationError.text.includes("press another time on the save button") && oldValue == valueField.text) {}   // ok save
        else {

            if(errStringCustom != "") {
                validationError.text = errStringCustom
                preventClose = true
                oldValue = valueField.text
                return
            }

        }

        if (bitmaskColumn.visible && !manualEntry.checked) {
            fact.value = bitmaskValue();
            fact.valueChanged(fact.value)
            valueChanged()
        } else if (factCombo.visible && !manualEntry.checked) {
            fact.enumIndex = factCombo.currentIndex
            valueChanged()
        } else {


            var stringConverted = factor ? (parseFloat(valueField.text)*100).toFixed(0) : valueField.text

            var errorString = fact.validate(stringConverted, forceSave.checked)
            if (errorString === "") {

                if (fact.name == "FENCE_ALT_MAX"){
                    Constants.lastMaxHeight = parseFloat(stringConverted)
                    fact.value = (parseFloat(stringConverted) * Constants.altitudeFactor).toFixed(4)
                }
                else
                    fact.value = stringConverted
                fact.valueChanged(fact.value)
                valueChanged()
            } else {
                validationError.text = errorString
                if (_allowForceSave) {
                    forceSave.visible = true
                }
                preventClose = true
            }
        }
    }


    function customControl() {

        var value = factor ? (parseFloat(valueField.text)*100).toFixed(2) : valueField.text

        if (value > max || value < min)
            return "Value must be between " + (factor ? min/100 : min).toFixed(2) + " and " + (factor ? max/100 : max).toFixed(2)

        if (fact.name == "FENCE_ALT_MAX" && value > Constants.maxAltitudeWarning)
            return "Attention: value above 120m. If you want to continue, press another time on the save button."

        return ""
    }

    function reject() {
        fact.valueChanged(fact.value)
        close()
    }

    function bitmaskValue() {
        var value = 0;
        for (var i = 0; i < fact.bitmaskValues.length; ++i) {
            var checkbox = bitmaskRepeater.itemAt(i)
            if (checkbox.checked) {
                value |= fact.bitmaskValues[i];
            }
        }
        return value
    }

    Component.onCompleted: {
        if (validate) {
            validationError.text = fact.validate(validateValue, false /* convertOnly */)
            if (_allowForceSave) {
                forceSave.visible = true
            }
        }
    }

    ColumnLayout {
        width:      _editFieldWidth * 2 //editRow.width
        spacing:    globals.defaultTextHeight

        QGCLabel {
            id:                 validationError
            Layout.fillWidth:   true
            wrapMode:           Text.WordWrap
            color:              qgcPal.warningText
            visible:            text !== ""
        }

        RowLayout {
            id:         editRow
            spacing:    ScreenTools.defaultFontPixelWidth

            QGCTextField {
                id:                 valueField
                width:              _editFieldWidth
                text:               getText()
                unitsLabel:         getUnits()
                showUnits:          fact.units != ""
                focus:              setFocus && visible
                inputMethodHints:   (fact.typeIsString || ScreenTools.isiOS) ? // iOS numeric keyboard has no done button, we can't use it
                                        Qt.ImhNone :
                                        Qt.ImhFormattedNumbersOnly  // Forces use of virtual numeric keyboard
                visible:            !_showCombo || validate || manualEntry.checked


                function getUnits() {
                    return fact.units.replace(/cm/g, "m")
                }

                function getText() {

                    if (validate)
                        return  validateValue

                    if (fact.name == "FENCE_ALT_MAX"){
                        return (Constants.lastMaxHeight).toFixed(0)
                    }

                    return factor ? (parseFloat(fact.valueString)/100).toFixed(2) : fact.valueString
                }

            }

            QGCComboBox {
                id:         factCombo
                width:      _editFieldWidth
                model:      fact.enumStrings
                visible:    _showCombo
                focus:      setFocus && visible

                Component.onCompleted: {
                    // We can't bind directly to fact.enumIndex since that would add an unknown value
                    // if there are no enum strings.
                    if (_showCombo) {
                        currentIndex = fact.enumIndex
                    }
                }

                onCurrentIndexChanged: {
                    if (currentIndex >=0 && currentIndex < model.length) {
                        valueField.text = fact.enumValues[currentIndex]
                    }
                }
            }

            QGCButton {
                visible:    _allowDefaultReset
                text:       qsTr("Reset To Default")

                onClicked: {
                    fact.value = fact.defaultValue
                    fact.valueChanged(fact.value)
                    close()
                }
            }
        }

        Column {
            id:         bitmaskColumn
            spacing:    ScreenTools.defaultFontPixelHeight / 2
            visible:    fact.bitmaskStrings.length > 0

            Repeater {
                id:     bitmaskRepeater
                model:  fact.bitmaskStrings

                delegate : QGCCheckBox {
                    text : modelData
                    checked : fact.value & fact.bitmaskValues[index]

                    onClicked: {
                        valueField.text = bitmaskValue()
                    }
                }
            }
        }

        QGCLabel {
            Layout.fillWidth:   true
            wrapMode:           Text.WordWrap
            visible:            !longDescriptionLabel.visible
            text:               fact.shortDescription
        }

        QGCLabel {
            id:                 longDescriptionLabel
            Layout.fillWidth:   true
            wrapMode:           Text.WordWrap
            visible:            fact.longDescription != ""
            text:               unit_transform()

            function unit_transform() {

                var desc = fact.longDescription
                return desc.replace(/cm/g, "m")

            }

        }

        Row {
            spacing: ScreenTools.defaultFontPixelWidth

            QGCLabel {
                id:         minValueDisplay
                text:       qsTr("Min: ") + (factor ? min/100 : min)
                visible:    !fact.minIsDefaultForType
            }

            QGCLabel {
                text:       qsTr("Max: ") + (factor ? max/100 : max)
                visible:    !fact.maxIsDefaultForType
            }

            QGCLabel {
                text:       qsTr("Default: ") + fact.defaultValueString
                visible:    _allowDefaultReset
            }
        }

        QGCLabel {
            text:       qsTr("Parameter name: ") + fact.name
            visible:    fact.componentId > 0 // > 0 means it's a parameter fact
        }

        QGCLabel {
            visible:    fact.vehicleRebootRequired
            text:       qsTr("Vehicle reboot required after change")
        }

        QGCLabel {
            visible:    fact.qgcRebootRequired
            text:       qsTr("Application restart required after change")
        }

        QGCLabel {
            Layout.fillWidth:   true
            wrapMode:   Text.WordWrap
            text:       qsTr("Warning: Modifying values while vehicle is in flight can lead to vehicle instability and possible vehicle loss. ") +
                        qsTr("Make sure you know what you are doing and double-check your values before Save!")
            visible:    fact.componentId != -1
        }

        QGCCheckBox {
            id:         forceSave
            visible:    false
            text:       qsTr("Force save (dangerous!)")
        }

        QGCCheckBox {
            id:         _advanced
            text:       qsTr("Advanced settings")
            visible:    showRCToParam || factCombo.visible || bitmaskColumn.visible
        }

        // Checkbox to allow manual entry of enumerated or bitmask parameters
        QGCCheckBox {
            id:         manualEntry
            visible:    _advanced.checked && (factCombo.visible || bitmaskColumn.visible)
            text:       qsTr("Manual Entry")

            onClicked: {
                valueField.text = fact.valueString
            }
        }

        QGCButton {
            text:       qsTr("Set RC to Param")
            visible:    _advanced.checked && !validate && showRCToParam
            onClicked:  rcToParamDialog.createObject(mainWindow).open()
        }
    } // Column

    Component {
        id: rcToParamDialog

        RCToParamDialog {
            tuningFact: fact
        }
    }
}
