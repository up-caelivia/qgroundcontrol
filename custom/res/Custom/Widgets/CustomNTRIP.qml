/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.12
import QtQuick.Dialogs  1.3
import QtQuick.Layouts  1.12

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Palette       1.0

import Constants 1.0

Rectangle {
    id: _root
    color: qgcPal.window
    anchors.fill: parent
    anchors.margins: ScreenTools.defaultFontPixelWidth

    readonly property var  _manager: QGroundControl.ntrip
    readonly property var  _profiles: _manager.configurations
    readonly property real _spacing: ScreenTools.defaultFontPixelWidth
    readonly property real _panelWidth: Math.min(width, ScreenTools.defaultFontPixelWidth * 70)
    readonly property bool _editing: settingsLoader.sourceComponent !== null
    readonly property bool _canMutate: _manager.storageReady && !_manager.storageBusy && !_manager.shutdownRequested
    readonly property color _connectionColor: _manager.receivingData ? qgcPal.colorGreen
                                               : _manager.connectionState === NTRIPConnection.Disconnected
                                                 ? qgcPal.colorGrey : qgcPal.colorOrange

    property string _selectedId: ""
    property string _errorText: ""
    property string _pendingOperation: ""
    property string _pendingId: ""

    QGCPalette {
        id: qgcPal
        colorGroupEnabled: _root.enabled
    }

    function selectedProfile() {
        for (var i = 0; i < _profiles.length; i++) {
            if (_profiles[i].id === _selectedId) {
                return _profiles[i]
            }
        }
        return null
    }

    function updateSelection() {
        if (selectedProfile()) {
            return
        }
        _selectedId = ""
        for (var i = 0; i < _profiles.length; i++) {
            if (_profiles[i].id === _manager.connectionId) {
                _selectedId = _profiles[i].id
                return
            }
        }
        if (_profiles.length) {
            _selectedId = _profiles[0].id
        }
    }

    function connectionStatus() {
        switch (_manager.connectionState) {
        case NTRIPConnection.Connecting:
            return qsTr("Connecting")
        case NTRIPConnection.RequestStream:
            return qsTr("Requesting corrections")
        case NTRIPConnection.Connected:
            return _manager.receivingData ? qsTr("RTCM received") : qsTr("Waiting for RTCM")
        default:
            return qsTr("Disconnected")
        }
    }

    function openEditor(id) {
        var values = id ? _manager.configuration(id) : {
            name: "", hostAddress: "", port: "", username: "", mountpoint: "",
            whitelist: "", enableVRS: true, forceNtripV1: false, hasPassword: false
        }
        if (id && !values.id) {
            _errorText = qsTr("The selected profile no longer exists.")
            return
        }
        _errorText = ""
        settingsLoader.configurationId = id
        settingsLoader.editingConfiguration = values
        settingsLoader.sourceComponent = configurationEditor
    }

    function closeEditor() {
        settingsLoader.sourceComponent = null
        settingsLoader.editingConfiguration = ({})
        settingsLoader.configurationId = ""
    }

    function saveConfiguration(values) {
        _errorText = ""
        _pendingId = settingsLoader.configurationId
        _pendingOperation = _pendingId ? "update" : "add"
        var result = _pendingId ? _manager.updateConfiguration(_pendingId, values) : _manager.addConfiguration(values)
        if (result !== NTRIPManager.Accepted) {
            _pendingOperation = ""
            _pendingId = ""
        }
    }

    function removeConfiguration(id) {
        _errorText = ""
        _pendingId = id
        _pendingOperation = "remove"
        if (_manager.removeConfiguration(id) !== NTRIPManager.Accepted) {
            _pendingOperation = ""
            _pendingId = ""
        }
    }

    Component.onCompleted: updateSelection()

    Connections {
        target: _root._manager

        function onConfigurationsChanged() {
            _root.updateSelection()
        }

        function onOperationRejected(result, message) {
            _root._errorText = message
        }

        function onErrorOccurred(message) {
            _root._errorText = message
        }

        function onConfigurationOperationFinished(operation, id, success, message) {
            if (operation !== _root._pendingOperation || (_root._pendingId && id !== _root._pendingId)) {
                return
            }
            _root._pendingOperation = ""
            _root._pendingId = ""
            if (success) {
                _root._errorText = ""
                if (operation !== "remove") {
                    _root._selectedId = id
                    _root.closeEditor()
                }
            } else {
                _root._errorText = message || qsTr("The profile could not be saved.")
            }
        }
    }

    ColumnLayout {
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        width: _root._panelWidth
        spacing: _root._spacing

        QGCLabel {
            Layout.fillWidth: true
            visible: _root._editing && _root._manager.connectionId.length > 0
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            text: qsTr("%1 (%2)").arg(_root._manager.connectionName).arg(_root.connectionStatus())
        }

        QGCLabel {
            Layout.fillWidth: true
            visible: _root._errorText.length > 0
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            color: qgcPal.warningText
            text: _root._errorText
        }

        QGCLabel {
            Layout.fillWidth: true
            visible: _root._manager.storageBusy || !_root._manager.storageReady
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            text: _root._manager.storageBusy
                    ? (_root._manager.storageReady ? qsTr("Saving profiles...") : qsTr("Loading profiles..."))
                    : qsTr("Profiles could not be loaded. Retry to continue.")
        }

        QGCButton {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("Retry loading")
            visible: !_root._manager.storageReady && !_root._manager.storageBusy
            enabled: !_root._manager.shutdownRequested
            onClicked: {
                _root._errorText = ""
                _root._manager.reloadConfigurations()
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            QGCFlickable {
                anchors.fill: parent
                clip: true
                visible: !_root._editing
                contentWidth: width
                contentHeight: settingsColumn.height
                flickableDirection: Flickable.VerticalFlick

                Column {
                    id: settingsColumn
                    width: _root._panelWidth
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: _root._spacing

                    Rectangle {
                        width: parent.width
                        height: statusColumn.implicitHeight + _root._spacing * 2
                        radius: _root._spacing / 2
                        color: qgcPal.windowShade

                        ColumnLayout {
                            id: statusColumn
                            anchors.fill: parent
                            anchors.margins: _root._spacing
                            spacing: _root._spacing / 2

                            RowLayout {
                                Layout.alignment: Qt.AlignHCenter
                                Layout.maximumWidth: statusColumn.width
                                spacing: _root._spacing

                                Rectangle {
                                    Layout.preferredWidth: _root._spacing
                                    Layout.preferredHeight: _root._spacing
                                    radius: width / 2
                                    color: _root._connectionColor
                                }

                                QGCLabel {
                                    text: _root.connectionStatus()
                                    font.bold: true
                                    font.pointSize: ScreenTools.mediumFontPointSize
                                    wrapMode: Text.Wrap
                                }

                                QGCLabel {
                                    Layout.fillWidth: true
                                    visible: _root._manager.connectionId.length > 0
                                    horizontalAlignment: Text.AlignRight
                                    wrapMode: Text.Wrap
                                    text: _root._manager.connectionName
                                }
                            }

                            // Separator
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 1
                                Layout.topMargin: _root._spacing / 2
                                Layout.bottomMargin: _root._spacing / 2
                                visible: _root._manager.receivingData
                                color: qgcPal.windowShadeDark
                            }

                            GridLayout {
                                Layout.fillWidth: true
                                visible: _root._manager.receivingData
                                columns: 2
                                columnSpacing: _root._spacing
                                rowSpacing: _root._spacing / 2

                                QGCLabel { text: qsTr("Base latitude"); Layout.fillWidth: true; wrapMode: Text.Wrap }
                                QGCLabel { text: Constants.ntripInfoLat.toFixed(5); Layout.alignment: Qt.AlignRight }
                                QGCLabel { text: qsTr("Base longitude"); Layout.fillWidth: true; wrapMode: Text.Wrap }
                                QGCLabel { text: Constants.ntripInfoLon.toFixed(5); Layout.alignment: Qt.AlignRight }
                                QGCLabel { text: qsTr("Base altitude"); Layout.fillWidth: true; wrapMode: Text.Wrap }
                                QGCLabel { text: qsTr("%1 m").arg(Constants.ntripInfoAlt.toFixed(2)); Layout.alignment: Qt.AlignRight }
                                QGCLabel { text: qsTr("RTCM3 messages received"); Layout.fillWidth: true; wrapMode: Text.Wrap }
                                QGCLabel { text: Constants.numM; Layout.alignment: Qt.AlignRight }
                                QGCLabel { text: qsTr("GPS satellites"); Layout.fillWidth: true; wrapMode: Text.Wrap }
                                QGCLabel { text: Constants.numGPS; Layout.alignment: Qt.AlignRight }
                                QGCLabel { text: qsTr("GLONASS satellites"); Layout.fillWidth: true; wrapMode: Text.Wrap }
                                QGCLabel { text: Constants.numGLO; Layout.alignment: Qt.AlignRight }
                            }
                        }
                    }

                    // Separator
                    Rectangle {
                        width: parent.width 
                        height: 1
                        anchors.topMargin: _root._spacing
                        anchors.bottomMargin: _root._spacing
                        color: qgcPal.windowShade
                    }

                    QGCLabel {
                        width: parent.width
                        visible: _root._manager.storageReady && !_root._profiles.length
                        horizontalAlignment: Text.AlignHCenter
                        text: qsTr("No saved profiles. Select Add to create a connection.")
                        wrapMode: Text.Wrap
                    }

                    Repeater {
                        model: _root._profiles

                        delegate: QGCButton {
                            id: profileButton
                            width: settingsColumn.width
                            text: modelData.name
                            checked: modelData.id === _root._selectedId
                            backRadius: _root._spacing / 2
                            onClicked: _root._selectedId = modelData.id

                            readonly property bool _activeProfile: modelData.id === _root._manager.connectionId
                            readonly property bool _highlighted: checked || pressed || hovered
                            readonly property color _labelColor: _highlighted ? qgcPal.buttonHighlightText : qgcPal.buttonText

                            background: Rectangle {
                                implicitHeight: ScreenTools.implicitButtonHeight
                                radius: profileButton.backRadius
                                color: profileButton._highlighted ? qgcPal.buttonHighlight : qgcPal.button
                                border.width: profileButton._activeProfile ? 2 : (profileButton.showBorder ? 1 : 0)
                                border.color: profileButton._activeProfile ? _root._connectionColor : qgcPal.buttonText
                            }

                            contentItem: ColumnLayout {
                                spacing: _root._spacing / 4

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: _root._spacing

                                    QGCLabel {
                                        Layout.fillWidth: true
                                        font.bold: true
                                        font.pointSize: ScreenTools.mediumFontPointSize
                                        elide: Text.ElideRight
                                        text: profileButton.text
                                        color: profileButton._labelColor
                                    }
                                    QGCLabel {
                                        visible: profileButton._activeProfile
                                        font.bold: true
                                        font.pointSize: ScreenTools.smallFontPointSize
                                        text: qsTr("Active")
                                        color: profileButton._labelColor
                                    }
                                }

                                QGCLabel {
                                    Layout.fillWidth: true
                                    wrapMode: Text.Wrap
                                    color: profileButton._labelColor
                                    text: {
                                        var host = modelData.hostAddress
                                        return (host.indexOf(":") >= 0 ? "[" + host + "]" : host) + " : " + modelData.port
                                    }
                                }
                            }
                        }
                    }
                }
            }

            Loader {
                id: settingsLoader
                anchors.fill: parent
                visible: _root._editing

                property string configurationId: ""
                property var editingConfiguration: ({})
            }
        }

        GridLayout {
            Layout.alignment: Qt.AlignHCenter
            visible: !_root._editing
            columns: _root._panelWidth >= profileActions.implicitWidth
                                    + connectionActions.implicitWidth + columnSpacing ? 2 : 1
            columnSpacing: _root._spacing * 2
            rowSpacing: _root._spacing

            GridLayout {
                id: profileActions
                Layout.alignment: Qt.AlignHCenter
                columns: _root._panelWidth >= addButton.implicitWidth + editButton.implicitWidth
                            + deleteButton.implicitWidth + columnSpacing * 2 ? 3 : 1
                columnSpacing: _root._spacing
                rowSpacing: _root._spacing / 2

                QGCButton {
                    id: addButton
                    Layout.fillWidth: true
                    text: qsTr("Add")
                    enabled: _root._canMutate
                    onClicked: _root.openEditor("")
                }

                QGCButton {
                    id: editButton
                    Layout.fillWidth: true
                    text: qsTr("Edit")
                    enabled: _root._canMutate && _root._selectedId.length > 0
                    onClicked: _root.openEditor(_root._selectedId)
                }

                QGCButton {
                    id: deleteButton
                    Layout.fillWidth: true
                    text: qsTr("Delete")
                    enabled: _root._canMutate && _root._selectedId.length > 0
                    onClicked: {
                        var profile = _root.selectedProfile()
                        if (profile) {
                            deleteDialog.profileId = profile.id
                            deleteDialog.profileName = profile.name
                            deleteDialog.open()
                        }
                    }
                }
            }

            GridLayout {
                id: connectionActions
                Layout.alignment: Qt.AlignHCenter
                columns: _root._panelWidth >= connectButton.implicitWidth
                            + disconnectButton.implicitWidth + columnSpacing ? 2 : 1
                columnSpacing: _root._spacing
                rowSpacing: _root._spacing / 2

                QGCButton {
                    id: connectButton
                    Layout.fillWidth: true
                    text: qsTr("Connect")
                    primary: enabled
                    enabled: _root._manager.storageReady && !_root._manager.shutdownRequested
                                && _root._selectedId.length > 0 && !_root._manager.connectionId.length
                    onClicked: {
                        _root._errorText = ""
                        _root._manager.connectConfiguration(_root._selectedId)
                    }
                }

                QGCButton {
                    id: disconnectButton
                    Layout.fillWidth: true
                    text: qsTr("Disconnect")
                    enabled: _root._manager.connectionId.length > 0
                    onClicked: _root._manager.disconnectConnection()
                }
            }
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            visible: _root._editing
            spacing: _root._spacing

            QGCButton {
                text: qsTr("Save")
                primary: true
                enabled: !!settingsLoader.item && settingsLoader.item.canSave
                onClicked: settingsLoader.item.save()
            }

            QGCButton {
                text: qsTr("Cancel")
                enabled: !_root._pendingOperation.length && !_root._manager.shutdownRequested
                onClicked: {
                    _root._errorText = ""
                    _root.closeEditor()
                }
            }
        }
    }

    MessageDialog {
        id: deleteDialog
        title: qsTr("Remove NTRIP profile")
        icon: StandardIcon.Warning
        standardButtons: StandardButton.Yes | StandardButton.No
        text: qsTr("Remove \"%1\"?").arg(profileName)
                + (profileId === _root._manager.connectionId ? "\n\n" + qsTr("The active connection will continue until you disconnect.") : "")

        property string profileId: ""
        property string profileName: ""

        onYes: _root.removeConfiguration(profileId)
    }

    Component {
        id: configurationEditor

        QGCFlickable {
            id: profileEditor
            clip: true
            contentWidth: width
            contentHeight: editorColumn.height
            flickableDirection: Flickable.VerticalFlick

            readonly property bool _creating: settingsLoader.configurationId.length === 0
            readonly property bool _canChangePassword: !_creating && settingsLoader.editingConfiguration.hasPassword
            readonly property bool _changingPassword: _canChangePassword && passwordAction.currentIndex === 1
            readonly property bool _showPasswordField: !_canChangePassword || _changingPassword
            readonly property bool canSave: _root._canMutate && nameField.text.trim().length > 0
                                            && hostField.text.trim().length > 0 && portField.acceptableInput
                                            && (!_changingPassword || passwordField.text.length > 0)

            function save() {
                var password = _showPasswordField ? passwordField.text : ""
                _root.saveConfiguration({
                    name: nameField.text,
                    hostAddress: hostField.text,
                    port: portField.text,
                    username: usernameField.text,
                    passwordAction: _canChangePassword && passwordAction.currentIndex === 2
                        ? "clear"
                        : (password.length ? "replace" : "keep"),
                    password: password,
                    mountpoint: mountpointField.text,
                    whitelist: whitelistField.text,
                    enableVRS: vrsCheckBox.checked,
                    forceNtripV1: ntripV1CheckBox.checked
                })
            }

            ColumnLayout {
                id: editorColumn
                width: _root._panelWidth
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: _root._spacing

                QGCGroupBox {
                    Layout.fillWidth: true
                    title: profileEditor._creating ? qsTr("Create NTRIP profile") : qsTr("Edit NTRIP profile")

                    GridLayout {
                        id: editorGrid
                        width: parent.width
                        columns: width < ScreenTools.defaultFontPixelWidth * 45 ? 1 : 2
                        columnSpacing: _root._spacing
                        rowSpacing: _root._spacing / 2
                        enabled: _root._canMutate

                        QGCLabel { text: qsTr("Name") }
                        QGCTextField {
                            id: nameField
                            Layout.fillWidth: true
                            text: settingsLoader.editingConfiguration.name
                            maximumLength: 128
                            placeholderText: qsTr("Profile name")
                        }

                        QGCLabel { text: qsTr("Server") }
                        QGCTextField {
                            id: hostField
                            Layout.fillWidth: true
                            text: settingsLoader.editingConfiguration.hostAddress
                            maximumLength: 253
                            placeholderText: qsTr("Hostname or IP address")
                            inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText
                        }

                        QGCLabel { text: qsTr("Port") }
                        QGCTextField {
                            id: portField
                            Layout.fillWidth: true
                            text: settingsLoader.editingConfiguration.port
                            maximumLength: 5
                            numericValuesOnly: true
                            placeholderText: qsTr("1 to 65535")
                            validator: IntValidator { bottom: 1; top: 65535 }
                        }

                        QGCLabel { text: qsTr("Username") }
                        QGCTextField {
                            id: usernameField
                            Layout.fillWidth: true
                            text: settingsLoader.editingConfiguration.username
                            maximumLength: 512
                            inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText
                        }

                        QGCLabel { text: qsTr("Password") }
                        QGCComboBox {
                            id: passwordAction
                            Layout.fillWidth: true
                            visible: profileEditor._canChangePassword
                            model: [qsTr("Keep saved password"), qsTr("Change password"), qsTr("Remove password")]
                            currentIndex: 0
                            onActivated: {
                                if (index === 0) {
                                    passwordField.text = ""
                                }
                            }
                        }

                        QGCLabel {
                            text: qsTr("New password")
                            visible: profileEditor._changingPassword
                        }
                        QGCTextField {
                            id: passwordField
                            Layout.fillWidth: true
                            visible: profileEditor._showPasswordField
                            echoMode: TextInput.Password
                            maximumLength: 4096
                            inputMethodHints: Qt.ImhHiddenText | Qt.ImhSensitiveData | Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase
                        }

                        QGCLabel { text: qsTr("Mountpoint") }
                        QGCTextField {
                            id: mountpointField
                            Layout.fillWidth: true
                            text: settingsLoader.editingConfiguration.mountpoint
                            maximumLength: 1024
                            placeholderText: qsTr("Without leading /; empty for RTCM over TCP")
                            inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText
                        }

                        QGCLabel { text: qsTr("RTCM whitelist") }
                        QGCTextField {
                            id: whitelistField
                            Layout.fillWidth: true
                            text: settingsLoader.editingConfiguration.whitelist
                            maximumLength: 32768
                            placeholderText: qsTr("Comma-separated IDs; empty to allow all")
                        }

                        QGCCheckBox {
                            id: vrsCheckBox
                            Layout.columnSpan: editorGrid.columns
                            text: qsTr("Send vehicle position (VRS)")
                            checked: settingsLoader.editingConfiguration.enableVRS
                        }
                        QGCCheckBox {
                            id: ntripV1CheckBox
                            Layout.columnSpan: editorGrid.columns
                            text: qsTr("Force NTRIP v1")
                            checked: settingsLoader.editingConfiguration.forceNtripV1
                        }
                    }
                }

                QGCLabel {
                    Layout.fillWidth: true
                    visible: !profileEditor._creating && settingsLoader.configurationId === _root._manager.connectionId
                    wrapMode: Text.WordWrap
                    text: qsTr("Changes apply on the next connection. The active connection keeps its current settings.")
                }
            }
        }
    }
}
