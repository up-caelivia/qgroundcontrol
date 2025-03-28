/*
 * ParameterEditorDialog.qml
 * QGroundControl custom version: supports unit conversion and displays values in user-friendly units.
 */

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

QGCPopupDialog {
    id: root
    title: qsTr("Parameter Editor")
    buttons: StandardButton.Cancel | StandardButton.Save

    property Fact fact
    property bool showRCToParam: false
    property bool validate: false
    property string validateValue
    property bool setFocus: true

    signal valueChanged

    property real _editFieldWidth: ScreenTools.defaultFontPixelWidth * 20
    property bool _editingParameter: fact.componentId != 0
    property bool _allowForceSave: QGroundControl.corePlugin.showAdvancedUI || !_editingParameter
    property bool _allowDefaultReset: fact.defaultValueAvailable && (QGroundControl.corePlugin.showAdvancedUI || !_editingParameter)
    property bool _showCombo: fact.enumStrings.length !== 0 && fact.bitmaskStrings.length === 0 && !validate

    ParameterEditorController { id: controller }

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    function conversionFactor(unit) {
        switch (unit) {
            case "cm": return 100.0
            case "cm/s": return 100.0
            case "cm/s/s": return 100.0
            case "mm": return 1000.0
            case "mm/s": return 1000.0
            case "mm/s/s": return 1000.0
            default: return 1.0
        }
    }

    function needsConversion() {
        return conversionFactor(fact.units) !== 1.0
    }

    function convertedUnit() {
        if (fact.units.endsWith("/s/s")) {
            var base = fact.units.split("/")[0]
            return (base === "cm" || base === "mm") ? "m/s²" : fact.units
        } else if (fact.units.endsWith("/s")) {
            var base = fact.units.split("/")[0]
            return (base === "cm" || base === "mm") ? "m/s" : fact.units
        }
        switch (fact.units) {
            case "cm": return "m"
            case "mm": return "m"
            default: return fact.units
        }
    }

    function toDisplayValue(val) {
        var num = parseFloat(val)
        if (isNaN(num)) return ""
        return (num / conversionFactor(fact.units)).toFixed(2)
    }

    function toInternalValue(val) {
        return parseFloat(val) * conversionFactor(fact.units)
    }

    function inputForValidation() {
        return toInternalValue(valueField.text)
    }

    function convertErrorString(rawError) {
        if (!needsConversion() || rawError === "") {
            return rawError
        }
        var converted = rawError
            .replace(/\d+([.,]\d+)?/g, function(match) {
                var val = parseFloat(match)
                return isNaN(val) ? match : toDisplayValue(val)
            })
            .replace(/cm\/s\/s|mm\/s\/s/g, "m/s²")
            .replace(/cm\/s|mm\/s/g, "m/s")
            .replace(/cm|mm/g, "m")

        return converted + " " + convertedUnit()
    }

    onAccepted: {
        if (bitmaskColumn.visible && !manualEntry.checked) {
            fact.value = bitmaskValue()
            fact.valueChanged(fact.value)
            valueChanged()
        } else if (factCombo.visible && !manualEntry.checked) {
            fact.enumIndex = factCombo.currentIndex
            valueChanged()
        } else {
            var rawError = fact.validate(inputForValidation(), forceSave.checked)
            var errorString = convertErrorString(rawError)
            if (errorString === "") {
                fact.value = toInternalValue(parseFloat(valueField.text))
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

    function reject() {
        fact.valueChanged(fact.value)
        close()
    }

    function bitmaskValue() {
        var value = 0
        for (var i = 0; i < fact.bitmaskValues.length; ++i) {
            var checkbox = bitmaskRepeater.itemAt(i)
            if (checkbox.checked) {
                value |= fact.bitmaskValues[i]
            }
        }
        return value
    }

    Component.onCompleted: {
        if (validate) {
            var rawError = fact.validate(inputForValidation(), false)
            validationError.text = convertErrorString(rawError)
            if (_allowForceSave) {
                forceSave.visible = true
            }
        }
    }

    ColumnLayout {
        width:      editRow.width
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
                text:               validate ? validateValue : toDisplayValue(fact.rawValue)
                unitsLabel:         convertedUnit()
                showUnits:          fact.units !== ""
                focus:              setFocus && visible
                inputMethodHints:   (fact.typeIsString || ScreenTools.isiOS) ? Qt.ImhNone : Qt.ImhFormattedNumbersOnly
                visible:            !_showCombo || validate || manualEntry.checked
            }

            QGCComboBox {
                id:         factCombo
                width:      _editFieldWidth
                model:      fact.enumStrings
                visible:    _showCombo
                focus:      setFocus && visible

                Component.onCompleted: {
                    if (_showCombo) {
                        currentIndex = fact.enumIndex
                    }
                }

                onCurrentIndexChanged: {
                    if (currentIndex >= 0 && currentIndex < model.length) {
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

                delegate: QGCCheckBox {
                    text: modelData
                    checked: fact.value & fact.bitmaskValues[index]
                    onClicked: {
                        valueField.text = bitmaskValue()
                    }
                }
            }
        }

        QGCLabel {
            Layout.fillWidth:   true
            wrapMode:           Text.WordWrap
            visible:            fact.longDescription === ""
            text:               fact.shortDescription
        }

        QGCLabel {
            id:                 longDescriptionLabel
            Layout.fillWidth:   true
            wrapMode:           Text.WordWrap
            visible:            fact.longDescription !== ""
            text:               fact.longDescription
        }

        Row {
            spacing: ScreenTools.defaultFontPixelWidth

            QGCLabel {
                text: qsTr("Min: ") + (needsConversion() ? toDisplayValue(fact.minString) + " " + convertedUnit() : fact.minString)
                visible: !fact.minIsDefaultForType
            }

            QGCLabel {
                text: qsTr("Max: ") + (needsConversion() ? toDisplayValue(fact.maxString) + " " + convertedUnit() : fact.maxString)
                visible: !fact.maxIsDefaultForType
            }

            QGCLabel {
                text: qsTr("Default: ") + (needsConversion() ? toDisplayValue(fact.defaultValueString) + " " + convertedUnit() : fact.defaultValueString)
                visible: _allowDefaultReset
            }
        }

        QGCLabel {
            text:       qsTr("Parameter name: ") + fact.name
            visible:    fact.componentId > 0
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
            wrapMode:           Text.WordWrap
            visible:            fact.componentId != -1
            text:               qsTr("Warning: Modifying values while vehicle is in flight can lead to vehicle instability and possible vehicle loss. ") +
                                qsTr("Make sure you know what you are doing and double-check your values before Save!")
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

        QGCCheckBox {
            id:         manualEntry
            visible:    _advanced.checked && (factCombo.visible || bitmaskColumn.visible)
            text:       qsTr("Manual Entry")
            onClicked: {
                valueField.text = fact.rawValue
            }
        }

        QGCButton {
            text:       qsTr("Set RC to Param")
            visible:    _advanced.checked && !validate && showRCToParam
            onClicked:  rcToParamDialog.createObject(mainWindow).open()
        }
    }

    Component {
        id: rcToParamDialog
        RCToParamDialog { tuningFact: fact }
    }
}
