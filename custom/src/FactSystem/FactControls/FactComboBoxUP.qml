import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.4

import QGroundControl.FactSystem 1.0
import QGroundControl.Palette 1.0
import QGroundControl.Controls 1.0

QGCComboBox {
    property Fact fact: Fact { }
    property bool indexModel: true  ///< true: model must be specified, selected index is fact value, false: use enum meta data

    // ✅ NEW: Optional filter for enumValues
    property var allowedValues: undefined

    // ✅ NEW: build filtered list if allowedValues is set
    model: fact && allowedValues ? filteredLabels() : fact ? fact.enumStrings : null

    currentIndex: {
        if (!fact) return 0
        if (indexModel) return fact.value
        if (allowedValues) return allowedValues.indexOf(fact.value)
        return fact.enumIndex
    }

    onModelChanged: {
        // When the model changes, the index gets reset to 0, so make sure to
        // restore it correctly.
        // Since enumIndex could trigger a model change, we use callLater() to
        // avoid an event binding loop (the 2. call will for certain not trigger
        // another model change)
        Qt.callLater(function() {
            currentIndex = fact ? (indexModel ? fact.value : fact.enumIndex) : 0
        })
    }

    onActivated: {
        if (!fact) return
        if (indexModel) {
            fact.value = index
        } else {
            // ✅ NEW: use allowedValues if defined
            fact.value = allowedValues ? allowedValues[index] : fact.enumValues[index]
        }
    }

    // ✅ NEW: helper function to filter labels
    function filteredLabels() {
        var result = []
        var labels = fact.enumStrings
        var values = fact.enumValues
        for (var i = 0; i < values.length; ++i) {
            if (allowedValues.indexOf(values[i]) !== -1) {
                result.push(labels[i])
            }
        }
        return result
    }
}
