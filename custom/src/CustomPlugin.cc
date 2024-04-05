/****************************************************************************
 *
 * (c) 2009-2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 *   @brief Custom QGCCorePlugin Implementation
 *   @author Gus Grubba <gus@auterion.com>
 */

#include <QtQml>
// #include <QQmlEngine>
// #include <QDateTime>
// #include "QGCSettings.h"
// #include "MAVLinkLogManager.h"
#include "MockLink.h"

#include "CustomPlugin.h"
#include "qcoreapplication.h"

// #include "MultiVehicleManager.h"
// #include "QGCApplication.h"
#include "SettingsManager.h"
// #include "AppMessages.h"
// #include "QmlComponentInfo.h"
// #include "QGCPalette.h"

// QGC_LOGGING_CATEGORY(CustomLog, "CustomLog")

CustomFlyViewOptions::CustomFlyViewOptions(CustomOptions* options, QObject* parent)
    : QGCFlyViewOptions(options, parent)
{

}

// // This custom build does not support conecting multiple vehicles to it. This in turn simplifies various parts of the QGC ui.
// bool CustomFlyViewOptions::showMultiVehicleList(void) const
// {
//     return false;
// }

// // This custom build has it's own custom instrument panel. Don't show regular one.
// bool CustomFlyViewOptions::showInstrumentPanel(void) const
// {
//     return false;
// }

CustomOptions::CustomOptions(CustomPlugin*, QObject* parent)
    : QGCOptions(parent)
{
}

QGCFlyViewOptions* CustomOptions::flyViewOptions(void)
{
    if (!_flyViewOptions) {
        _flyViewOptions = new CustomFlyViewOptions(this, this);
    }
    return _flyViewOptions;
}

// // Firmware upgrade page is only shown in Advanced Mode.
// bool CustomOptions::showFirmwareUpgrade() const
// {
//     return qgcApp()->toolbox()->corePlugin()->showAdvancedUI();
// }

// // Normal QGC needs to work with an ESP8266 WiFi thing which is remarkably crappy. This in turns causes PX4 Pro calibration to fail
// // quite often. There is a warning in regular QGC about this. Overriding the and returning true means that your custom vehicle has
// // a reliable WiFi connection so don't show that warning.
// bool CustomOptions::wifiReliableForCalibration(void) const
// {
//     return true;
// }

CustomPlugin::CustomPlugin(QGCApplication *app, QGCToolbox* toolbox)
    : QGCCorePlugin(app, toolbox)
{
    _options = new CustomOptions(this, this);
    // _showAdvancedUI = false;

    QCoreApplication::setApplicationName(QStringLiteral(QGC_APPLICATION_NAME));

}

CustomPlugin::~CustomPlugin()
{
}

// void CustomPlugin::setToolbox(QGCToolbox* toolbox)
// {
//     QGCCorePlugin::setToolbox(toolbox);

//     // Allows us to be notified when the user goes in/out out advanced mode
//     connect(qgcApp()->toolbox()->corePlugin(), &QGCCorePlugin::showAdvancedUIChanged, this, &CustomPlugin::_advancedChanged);
// }

// void CustomPlugin::_advancedChanged(bool changed)
// {
//     // Firmware Upgrade page is only show in Advanced mode
//     emit _options->showFirmwareUpgradeChanged(changed);
// }

// //-----------------------------------------------------------------------------
// void CustomPlugin::_addSettingsEntry(const QString& title, const char* qmlFile, const char* iconFile)
// {
//     Q_CHECK_PTR(qmlFile);
//     // 'this' instance will take ownership on the QmlComponentInfo instance
//     _customSettingsList.append(QVariant::fromValue(
//         new QmlComponentInfo(title,
//                 QUrl::fromUserInput(qmlFile),
//                 iconFile == nullptr ? QUrl() : QUrl::fromUserInput(iconFile),
//                 this)));
// }

// //-----------------------------------------------------------------------------
// QVariantList&
// CustomPlugin::settingsPages()
// {
//     if(_customSettingsList.isEmpty()) {
//         _addSettingsEntry(tr("General"),     "qrc:/qml/GeneralSettings.qml",     "qrc:/res/gear-white.svg");
//         _addSettingsEntry(tr("Comm Links"),  "qrc:/qml/LinkSettings.qml",        "qrc:/res/waves.svg");
//         _addSettingsEntry(tr("Offline Maps"),"qrc:/qml/OfflineMap.qml",          "qrc:/res/waves.svg");
//         _addSettingsEntry(tr("MAVLink"),     "qrc:/qml/MavlinkSettings.qml",     "qrc:/res/waves.svg");
//         _addSettingsEntry(tr("Console"),     "qrc:/qml/QGroundControl/Controls/AppMessages.qml");
// #if defined(QT_DEBUG)
//         //-- These are always present on Debug builds
//         _addSettingsEntry(tr("Mock Link"),   "qrc:/qml/MockLink.qml");
// #endif
//     }
//     return _customSettingsList;
// }

QGCOptions* CustomPlugin::options()
{
    return _options;
}

QString CustomPlugin::brandImageIndoor(void) const
{
    return QStringLiteral("/custom/img/CustomAppIcon.png");
}

QString CustomPlugin::brandImageOutdoor(void) const
{
    return QStringLiteral("/custom/img/CustomAppIcon.png");
}

bool CustomPlugin::overrideSettingsGroupVisibility(QString name)
{
    // We have set up our own specific brand imaging. Hide the brand image settings such that the end user
    // can't change it.
    if (name == BrandImageSettings::name) {
        return false;
    }
    return true;
}

// This allows you to override/hide QGC Application settings
// bool CustomPlugin::adjustSettingMetaData(const QString& settingsGroup, FactMetaData& metaData)
// {
//     bool parentResult = QGCCorePlugin::adjustSettingMetaData(settingsGroup, metaData);

//     if (settingsGroup == AppSettings::settingsGroup) {
//         // This tells QGC than when you are creating Plans while not connected to a vehicle
//         // the specific firmware/vehicle the plan is for.
//         if (metaData.name() == AppSettings::offlineEditingFirmwareClassName) {
//             metaData.setRawDefaultValue(QGCMAVLink::FirmwareClassPX4);
//             return false;
//         } else if (metaData.name() == AppSettings::offlineEditingVehicleClassName) {
//             metaData.setRawDefaultValue(QGCMAVLink::VehicleClassMultiRotor);
//             return false;
//         }
//     }

//     return parentResult;
// }

// This modifies QGC colors palette to match possible custom corporate branding
void CustomPlugin::paletteOverride(QString colorName, QGCPalette::PaletteColorInfo_t& colorInfo)
{

    // qDebug() << "colorName" << colorName;

    if (colorName == QStringLiteral("buttonHighlight")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#07916d");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#495057");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#aeebd0");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#e4e4e4");
    }
    else if (colorName == QStringLiteral("buttonHighlightText")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#ffffff");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#777c89");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#212529");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#2c2c2c");
    }
    else if (colorName == QStringLiteral("primaryButton")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#12b886");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#495057");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#aeebd0");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#585858");
    }
    else if (colorName == QStringLiteral("mapButtonHighlight")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#07916d");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#585858");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#be781c");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#585858");
    }
    else if (colorName == QStringLiteral("mapIndicator")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#9dda4f");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#585858");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#be781c");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#585858");
    }
    else if (colorName == QStringLiteral("mapIndicatorChild")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#527942");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#585858");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#766043");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#585858");
    }
    else if (colorName == QStringLiteral("colorGreen")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#27bf89");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#0ca678");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#009431");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#009431");
    }
    else if (colorName == QStringLiteral("colorOrange")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#f7b24a");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#f6921e");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#b95604");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#b95604");
    }
    else if (colorName == QStringLiteral("alertBackground")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#d4b106");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#d4b106");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#fffb8f");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#b45d48");
    }
    else if (colorName == QStringLiteral("alertBorder")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#876800");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#876800");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#808080");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#808080");
    }
    else if (colorName == QStringLiteral("hoverColor")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#07916d");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#33c494");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#aeebd0");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#464f5a");
    }
    else if (colorName == QStringLiteral("brandingPurple")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#33c494");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#33c494");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#33c494");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#33c494");
    }
    else if (colorName == QStringLiteral("brandingBlue")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#33c494");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#33c494");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#33c494");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#33c494");
    }
}

// We override this so we can get access to QQmlApplicationEngine and use it to register our qml module
QQmlApplicationEngine* CustomPlugin::createQmlApplicationEngine(QObject* parent)
{
    QQmlApplicationEngine* qmlEngine = QGCCorePlugin::createQmlApplicationEngine(parent);
    qmlEngine->addImportPath("qrc:/Custom/Widgets");

#ifdef QT_DEBUG     // start with a custom connection only in debug build!
    MockLink::startPX4MockLink(true);
#endif

    return qmlEngine;
}
