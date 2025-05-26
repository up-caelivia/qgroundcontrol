import QtQuick                  2.3
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.4
import QtQuick.Dialogs          1.2

import QGroundControl.FactSystem    1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0

QGCTextField {
    id: _textField

    // Display value converted to SI units (e.g., cm → m, cm/s → m/s)
    text: {
        if (!fact) return ""
        if (fact.typeIsString) return fact.valueString
        var siVal = toSI(fact.value, fact.units)
        return siVal.toFixed(totalDisplayDecimals(fact.units))
    }

    // Show SI unit label instead of internal units
    unitsLabel: {
        if (!fact) return ""
        if (fact.units === "cm") return "m"
        if (fact.units === "cm/s") return "m/s"
        return fact.units
    }

    showUnits:          true
    showHelp:           true
    numericValuesOnly:  fact && !fact.typeIsString

    signal updated()

    property Fact   fact: null
    property string _validateString

    function toSI(value, units) {
        if (units === "cm") return value / 100
        if (units === "cm/s") return value / 100
        return value
    }

    // Convert SI value back to internal units when setting the fact
    function fromSI(value, units) {
        if (units === "cm") return value * 100
        if (units === "cm/s") return value * 100
        return value
    }
    
    function unitScaleFactor(units) {
        if (units === "cm" || units === "cm/s") return 100
        return 1
    }

    function decimalShiftFromConversion(units) {
        var factor = unitScaleFactor(units)
        return Math.max(0, Math.round(Math.log10(factor)))
    }

    function totalDisplayDecimals(units) {
        return fact.decimalPlaces + decimalShiftFromConversion(units)
    }

    onEditingFinished: {
        if (!fact) return

        if (fact.typeIsString) {
            // Nessuna conversione per le stringhe
            var errorString = fact.validate(text, false /* convertOnly */)
            if (errorString === "") {
                fact.value = text
                _textField.updated()
            } else {
                _validateString = text
                validationErrorDialogComponent.createObject(mainWindow).open()
            }
        } else {
            // Numerico con conversione da SI
            var siVal = parseFloat(text)
            var internalVal = fromSI(siVal, fact.units)

            var errorString = fact.validate(internalVal.toString(), false /* convertOnly */)

            if (errorString === "") {
                fact.value = internalVal
                _textField.updated()
            } else {
                errorString = "CCC " + errorString
                _validateString = text
                var siMin = toSI(fact.rawMin, fact.units)
                var siMax = toSI(fact.rawMax, fact.units)
                var decimals = totalDisplayDecimals(fact.units)
                validationErrorDialogComponent.createObject(mainWindow, {
                    validate: true,
                    validateValue: _validateString,
                    fact: _textField.fact,
                    siMinString: siMin.toFixed(decimals),
                    siMaxString: siMax.toFixed(decimals),
                    siUnits: unitsLabel
                }).open()
            }
        }
    }

    onHelpClicked: helpDialogComponent.createObject(mainWindow).open()

    Component {
        id: validationErrorDialogComponent
        ParameterEditorDialog { }
    }

    Component {
        id: helpDialogComponent

        ParameterEditorDialog {
            title:          qsTr("Value Details")
            fact:           _textField.fact
        }
    }
}
