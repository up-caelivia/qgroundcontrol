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
#include "MockLink.h"

#include "CustomPlugin.h"
#include "qcoreapplication.h"
#include "SettingsManager.h"


CustomFlyViewOptions::CustomFlyViewOptions(CustomOptions* options, QObject* parent)
    : QGCFlyViewOptions(options, parent) {}


QGCFlyViewOptions* CustomOptions::flyViewOptions(void)
{
    if (!_flyViewOptions) {
        _flyViewOptions = new CustomFlyViewOptions(this, this);
    }
    return _flyViewOptions;
}


CustomPlugin::CustomPlugin(QGCApplication *app, QGCToolbox* toolbox)
    : QGCCorePlugin(app, toolbox)
{
    _options = new CustomOptions(this, this);
    QCoreApplication::setApplicationName(QStringLiteral("QGroundControlUP"));  // set the folder on document to save the options
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


// // This modifies QGC colors palette to match possible custom corporate branding
// void CustomPlugin::paletteOverride(QString colorName, QGCPalette::PaletteColorInfo_t& colorInfo)
// {

//     if (colorName == QStringLiteral("buttonHighlight")) {
//         colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#07916d");
//         colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#495057");
//         colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#aeebd0");
//         colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#e4e4e4");
//     }
//     else if (colorName == QStringLiteral("buttonHighlightText")) {
//         colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#ffffff");
//         colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#777c89");
//         colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#212529");
//         colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#2c2c2c");
//     }
//     else if (colorName == QStringLiteral("primaryButton")) {
//         colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#12b886");
//         colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#495057");
//         colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#aeebd0");
//         colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#585858");
//     }
//     else if (colorName == QStringLiteral("mapButtonHighlight")) {
//         colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#07916d");
//         colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#585858");
//         colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#be781c");
//         colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#585858");
//     }
//     else if (colorName == QStringLiteral("mapIndicator")) {
//         colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#9dda4f");
//         colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#585858");
//         colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#be781c");
//         colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#585858");
//     }
//     else if (colorName == QStringLiteral("mapIndicatorChild")) {
//         colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#527942");
//         colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#585858");
//         colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#766043");
//         colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#585858");
//     }
//     else if (colorName == QStringLiteral("colorGreen")) {
//         colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#27bf89");
//         colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#0ca678");
//         colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#009431");
//         colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#009431");
//     }
//     else if (colorName == QStringLiteral("colorOrange")) {
//         colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#f7b24a");
//         colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#f6921e");
//         colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#b95604");
//         colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#b95604");
//     }
//     else if (colorName == QStringLiteral("alertBackground")) {
//         colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#d4b106");
//         colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#d4b106");
//         colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#fffb8f");
//         colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#b45d48");
//     }
//     else if (colorName == QStringLiteral("alertBorder")) {
//         colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#876800");
//         colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#876800");
//         colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#808080");
//         colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#808080");
//     }
//     else if (colorName == QStringLiteral("hoverColor")) {
//         colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#07916d");
//         colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#33c494");
//         colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#aeebd0");
//         colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#464f5a");
//     }
//     else if (colorName == QStringLiteral("brandingPurple")) {
//         colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#33c494");
//         colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#33c494");
//         colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#33c494");
//         colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#33c494");
//     }
//     else if (colorName == QStringLiteral("brandingBlue")) {
//         colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#33c494");
//         colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#33c494");
//         colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#33c494");
//         colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#33c494");
//     }
// }

// // // We override this so we can get access to QQmlApplicationEngine and use it to register our qml module
QQmlApplicationEngine* CustomPlugin::createQmlApplicationEngine(QObject* parent)
{
    QQmlApplicationEngine* qmlEngine = QGCCorePlugin::createQmlApplicationEngine(parent);
    qmlEngine->addImportPath("qrc:/Custom/Widgets");

    #ifdef QT_DEBUG     // start with a custom connection only in debug build!
        MockLink::startPX4MockLink(true);
    #endif

    return qmlEngine;
}
