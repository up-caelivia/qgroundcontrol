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

#include "AutoConnectSettings.h"
#include "VideoSettings.h"
#include "AppSettings.h"
#include "QGCApplication.h"
#include "QGCToolbox.h"
#include "MultiVehicleManager.h"
#include "constants.h"
#include "KmlPolygonLoader.h"
#include "CustomAnnouncer.h"
#include "CustomToolbox.h"
// #include "JoystickManager.h"
// #include "HorizontalFactValueGrid.h"
// #include "InstrumentValueData.h"
#include <list>

void CustomPlugin::registerQmlTypes()
{
    qmlRegisterSingletonType<CustomAnnouncer>("CustomAnnouncer", 1, 0, "CustomAnnouncer",
        [](QQmlEngine*, QJSEngine*) -> QObject* {
            return new CustomAnnouncer();
        });
}


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
    QCoreApplication::setApplicationName(QStringLiteral(QGC_APPLICATION_NAME));  // set the folder on document to save the options

    #ifdef Q_OS_WIN
        QApplication::setWindowIcon(QIcon(":/res/resources/icons/qgroundcontrol.ico"));
    #endif
}

void CustomPlugin::setToolbox(QGCToolbox* toolbox)
{
    QGCCorePlugin::setToolbox(toolbox);
    qDebug() << "[CustomPlugin] setToolbox() called";

    qmlRegisterSingletonType<CustomAnnouncer>("CustomAnnouncer", 1, 0, "CustomAnnouncer",
        [](QQmlEngine*, QJSEngine*) -> QObject* {
            return new CustomAnnouncer();
        });

    _customToolbox = new CustomToolbox(qgcApp());
}


bool CustomPlugin::overrideSettingsGroupVisibility(QString name)
{
    // We have set up our own specific brand imaging. Hide the brand image settings such that the end user
    // can't change it.

    Constants constant;

    if(constant.developer())
        return true;


    //qDebug() << "Setting name: " << name;
#ifndef ABLUO_APP
    if (name == BrandImageSettings::name || name == AutoConnectSettings::name || name == ADSBVehicleManagerSettings::name || name == RTKSettings::name || name == PlanViewSettings::name) {
#else
    if (name == BrandImageSettings::name || name == AutoConnectSettings::name || name == UnitsSettings::name || name == ADSBVehicleManagerSettings::name || name == RTKSettings::name || name == PlanViewSettings::name) {
#endif
        return false;
    }


    return true;
}


// // This modifies QGC colors palette to match possible custom corporate branding
void CustomPlugin::paletteOverride(QString colorName, QGCPalette::PaletteColorInfo_t& colorInfo)
{


  //   DECLARE_QGC_COLOR(window,               "#ffffff", "#ffffff", "#222222", "#222222")
  //   DECLARE_QGC_COLOR(windowShadeLight,     "#909090", "#828282", "#707070", "#626262")
  //   DECLARE_QGC_COLOR(windowShade,          "#d9d9d9", "#d9d9d9", "#333333", "#333333")
  //   DECLARE_QGC_COLOR(windowShadeDark,      "#bdbdbd", "#bdbdbd", "#282828", "#282828")
  //   DECLARE_QGC_COLOR(text,                 "#9d9d9d", "#000000", "#707070", "#ffffff")
  //   DECLARE_QGC_COLOR(warningText,          "#cc0808", "#cc0808", "#f85761", "#f85761")
  //   DECLARE_QGC_COLOR(button,               "#ffffff", "#ffffff", "#707070", "#626270")
  //   DECLARE_QGC_COLOR(buttonText,           "#9d9d9d", "#000000", "#A6A6A6", "#ffffff")
  //   DECLARE_QGC_COLOR(primaryButtonText,    "#2c2c2c", "#000000", "#2c2c2c", "#000000")
  //   DECLARE_QGC_COLOR(textField,            "#ffffff", "#ffffff", "#707070", "#ffffff")
  //   DECLARE_QGC_COLOR(textFieldText,        "#808080", "#000000", "#000000", "#000000")
  //   DECLARE_QGC_COLOR(mapButton,            "#585858", "#000000", "#585858", "#000000")


  //   DECLARE_QGC_COLOR(colorRed,             "#ed3939", "#ed3939", "#f32836", "#f32836")
  //   DECLARE_QGC_COLOR(colorGrey,            "#808080", "#808080", "#bfbfbf", "#bfbfbf")
  //   DECLARE_QGC_COLOR(colorBlue,            "#1a72ff", "#1a72ff", "#536dff", "#536dff")

  //   DECLARE_QGC_COLOR(alertText,            "#000000", "#000000", "#000000", "#000000")
  //   DECLARE_QGC_COLOR(missionItemEditor,    "#585858", "#dbfef8", "#585858", "#585d83")
  //   DECLARE_QGC_COLOR(toolStripHoverColor,  "#585858", "#9D9D9D", "#585858", "#585d83")
  //   DECLARE_QGC_COLOR(statusFailedText,     "#9d9d9d", "#000000", "#707070", "#ffffff")
  //   DECLARE_QGC_COLOR(statusPassedText,     "#9d9d9d", "#000000", "#707070", "#ffffff")
  //   DECLARE_QGC_COLOR(statusPendingText,    "#9d9d9d", "#000000", "#707070", "#ffffff")
  //   DECLARE_QGC_COLOR(toolbarBackground,    "#ffffff", "#ffffff", "#222222", "#222222")

#ifdef ABLUO_APP
    QColor color1 = QColor("#004F9F");
    QColor color2 = QColor("#76BEEA");
    QColor color3 = QColor("#76BEEA");
    QColor color4 = QColor("#004F9F");
    if (colorName == QStringLiteral("buttonText")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#2d2d2d");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#7d7d7d");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#A6A6A6");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#ffffff");
    } else
#else
    QColor color1 = QColor("#E73444");
    QColor color2 = QColor("#73969D");
    QColor color3 = QColor("#61848C");
    QColor color4 = QColor("#E73444");
#endif

    if (colorName == QStringLiteral("buttonHighlight")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = color1;
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = color1;
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = color1;
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = color1;
    }
    else if (colorName == QStringLiteral("buttonHighlightText")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#ffffff");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#777c89");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#212529");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#2c2c2c");
    }
    else if (colorName == QStringLiteral("primaryButton")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = color1;
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = color1;
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = color1;
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = color1;
    }
    else if (colorName == QStringLiteral("mapButtonHighlight")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = color1;
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = color1;
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = color1;
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = color1;
    }

    else if (colorName == QStringLiteral("toolStripHoverColor")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = color2;
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = color2;
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = color2;
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = color2;
    }

    else if (colorName == QStringLiteral("missionItemEditor")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = color2;
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = color2;
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = color2;
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = color2;
    }

    else if (colorName == QStringLiteral("button")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = color3;
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = color3;
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#ffffff");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#ffffff");
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
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = color4;
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = color4;
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = color4;
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = color4;
    }
    else if (colorName == QStringLiteral("brandingPurple")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = color4;
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = color4;
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = color4;
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = color4;
    }
}

static QObject* kmlPolygonLoader_singletontype_provider(QQmlEngine* engine, QJSEngine* scriptEngine)
{
    Q_UNUSED(engine)
    Q_UNUSED(scriptEngine)
    return KmlPolygonLoader::instance();
}

// We override this so we can get access to QQmlApplicationEngine and use it to register our qml module
QQmlApplicationEngine* CustomPlugin::createQmlApplicationEngine(QObject* parent)
{
    QQmlApplicationEngine* qmlEngine = QGCCorePlugin::createQmlApplicationEngine(parent);
    qmlEngine->addImportPath("qrc:/Custom/Widgets");
    qmlEngine->addImportPath("qrc:/Custom/Constants");
    qmlEngine->addImportPath("qrc:/Custom/GeoAwareness");

    qmlRegisterSingletonType<Constants>("Constants", 1, 0, "Constants", Constants::constants_singleton_provider);

 

    qmlRegisterSingletonType<KmlPolygonLoader>("QGroundControl.KML", 1, 0, "KmlPolygonLoader", kmlPolygonLoader_singletontype_provider);

    #ifdef QT_DEBUG     // start with a custom connection only in debug build!
        MockLink::startAPMArduCopterMockLink(false);
    #endif

    return qmlEngine;
}


bool CustomPlugin::adjustSettingMetaData(const QString& settingsGroup, FactMetaData& metaData)
{
    if (settingsGroup == AppSettings::settingsGroup) {
        // //-- Default herelink fontsize of 10, it is a nice starting point
        // if (metaData.name() == AppSettings::appFontPointSizeName) {
        //     uint32_t fontSize = 10;
        //     metaData.setRawDefaultValue(fontSize);
        //     // Show setting in ui
        //     return true;
        // }

        //-- Default Palette Dark
        if (metaData.name() == AppSettings::indoorPaletteName) {
            QVariant outdoorPalette;
            outdoorPalette = 1;
            metaData.setRawDefaultValue(outdoorPalette);
            return true;
        }
    }

    if (settingsGroup == AutoConnectSettings::settingsGroup) {
        // We have to adjust the Herelink UDP autoconnect settings for the AirLink
        if (metaData.name() == AutoConnectSettings::udpListenPortName) {
            metaData.setRawDefaultValue(14551);
        } else if (metaData.name() == AutoConnectSettings::udpTargetHostIPName) {
            metaData.setRawDefaultValue(QStringLiteral("127.0.0.1"));
        } else if (metaData.name() == AutoConnectSettings::udpTargetHostPortName) {
            metaData.setRawDefaultValue(15552);
        } else {
            // Disable all the other autoconnect types
            const std::list<const char *> disabledAndHiddenSettings = {
                AutoConnectSettings::autoConnectPixhawkName,
                AutoConnectSettings::autoConnectSiKRadioName,
                AutoConnectSettings::autoConnectPX4FlowName,
                AutoConnectSettings::autoConnectRTKGPSName,
                AutoConnectSettings::autoConnectLibrePilotName,
                AutoConnectSettings::autoConnectNmeaPortName,
                AutoConnectSettings::autoConnectZeroConfName,
            };
            for (const char * disabledAndHiddenSetting : disabledAndHiddenSettings) {
                if (disabledAndHiddenSetting == metaData.name()) {
                    metaData.setRawDefaultValue(false);
                }
            }
        }
    } else if (settingsGroup == VideoSettings::settingsGroup) {
        if (metaData.name() == VideoSettings::rtspTimeoutName) {
            metaData.setRawDefaultValue(60);
        } else if (metaData.name() == VideoSettings::videoSourceName) {
            metaData.setRawDefaultValue(VideoSettings::videoSourceHerelinkAirUnit);
        }
    } 

    return true; // Show all settings in ui
}

QVariantList& CustomPlugin::settingsPages()
{
    Constants constant;

    if (_customSettingsList.isEmpty()) {
        // Get default pages from QGCCorePlugin
        QVariantList baseSettings = QGCCorePlugin::settingsPages();
        _customSettingsList = baseSettings;

        //add NTRIP page
        _ntripSettings = new QmlComponentInfo(
            tr("NTRIP"),
            QUrl::fromUserInput("qrc:/Custom/Widgets/CustomNTRIP.qml"),
            QUrl::fromUserInput("qrc:/res/gear-white.svg")
        );

        // Find "Offline Maps" position and insert NTRIP page after it
        int insertIndex = -1;
        for (int i = 0; i < _customSettingsList.count(); ++i) {
            QmlComponentInfo* info = _customSettingsList[i].value<QmlComponentInfo*>();
            if (info && info->title() == tr("Offline Maps")) {
                insertIndex = i + 1;
                break;
            }
        }

        // Insert NTRIP page
        if (insertIndex >= 0 && insertIndex <= _customSettingsList.count()) {
            _customSettingsList.insert(insertIndex, QVariant::fromValue(_ntripSettings));
        } else {
            _customSettingsList.append(QVariant::fromValue(_ntripSettings));
        }

        //add About page
        _aboutSettings = new QmlComponentInfo(
            tr("About"),
            QUrl::fromUserInput("qrc:/Custom/Widgets/AboutUP.qml"),
            QUrl::fromUserInput("qrc:/res/gear-white.svg")
        );

        // Insert About page
        _customSettingsList.append(QVariant::fromValue(_aboutSettings));
    } 
    
    Constants* constants = Constants::getInstance();
    if (!constants->developer()) {
        qDebug() << "finding Comm Links page";
        for (int i = 0; i < _customSettingsList.count(); ++i) {
            QmlComponentInfo* info = _customSettingsList[i].value<QmlComponentInfo*>();
            if (info && info->title() == tr("Comm Links")) {
                _customSettingsList.removeAt(i);
                break;
            }
        }
    } else {
        for (int i = 0; i < _customSettingsList.count(); ++i) {
            QmlComponentInfo* _commLinksSettings = _customSettingsList[i].value<QmlComponentInfo*>();
            if (_commLinksSettings && _commLinksSettings->title() == tr("Comm Links")) {
                return _customSettingsList;
            }
        }

        // Add the "Comm Links" page if developer mode is active
        QmlComponentInfo* _commLinksSettings = new QmlComponentInfo(tr("Comm Links"),
            QUrl::fromUserInput("qrc:/qml/LinkSettings.qml"),
            QUrl::fromUserInput("qrc:/res/waves.svg")
        );
        // Insert Comm Links page
        _customSettingsList.insert(1, QVariant::fromValue(_commLinksSettings));   
    }
    return _customSettingsList;
}













