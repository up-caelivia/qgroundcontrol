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
#include "CustomAnnouncer.h"
#include "CustomToolbox.h"
// #include "JoystickManager.h"
// #include "HorizontalFactValueGrid.h"
// #include "InstrumentValueData.h"
#include <list>
#include "ParameterManager.h"
#include "GeoFenceController.h"

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

    _savParamTimer = new QTimer(this);
    connect(_savParamTimer, &QTimer::timeout, this, &CustomPlugin::_updateSavParamCoordinates);
    _savParamTimer->start(1000);
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

    connect(qgcApp()->toolbox()->multiVehicleManager(), &MultiVehicleManager::activeVehicleChanged,
        this, &CustomPlugin::onActiveVehicleChanged);
}

void CustomPlugin::sendLogMessage(const QString& text, const QString& description, const QString& severityStr)
{
    static const QMap<QString, int> severityMap {
        { "Emergency", 0 },
        { "Alert",     1 },
        { "Critical",  2 },
        { "Error",     3 },
        { "Warning",   4 },
        { "Notice",    5 },
        { "Info",      6 },
        { "Debug",     7 }
    };

    int severity = severityMap.value(severityStr.trimmed(), 6); // default to Info

    UASMessageHandler* msgHandler = qgcApp()->toolbox()->uasMessageHandler();
    if (msgHandler) {
        qDebug() << "severity" << severity;
        msgHandler->handleTextMessage(1, 1, severity, text, description);
    }
}

void CustomPlugin::setSpeedMessage(const QString& msg)
{
    if (_speedMessage != msg) {
        _speedMessage = msg;
        emit speedMessageChanged();
    }
    if (!msg.isEmpty()){
        qgcApp()->toolbox()->audioOutput()->say(msg);
    }
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

// We override this so we can get access to QQmlApplicationEngine and use it to register our qml module
QQmlApplicationEngine* CustomPlugin::createQmlApplicationEngine(QObject* parent)
{
    QQmlApplicationEngine* qmlEngine = QGCCorePlugin::createQmlApplicationEngine(parent);
    qmlEngine->addImportPath("qrc:/Custom/Widgets");
    qmlEngine->addImportPath("qrc:/Custom/Constants");

    qmlRegisterSingletonType<Constants>("Constants", 1, 0, "Constants", Constants::constants_singleton_provider);

    qmlEngine->rootContext()->setContextProperty("CustomPlugin", this); 

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

QVariantList CustomPlugin::getSavParamCoordinates(Vehicle* vehicle) {
    QVariantList coordinates;
    if (!vehicle || !vehicle->parameterManager()) return coordinates;
    if (_isSAVenabled == false || _isSAVexist == false) {
        return coordinates;
    }

    QStringList suffixes = { "WP_SG","WP_MAR", "A","1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11" };
    int componentId = FactSystem::defaultComponentId;

    for (QString& s : suffixes) {
        QString latName = QString("SAV_%1_LAT").arg(s);
        QString lonName = QString("SAV_%1_LON").arg(s);
        QString altName = QString("SAV_%1_ALT").arg(s);
        QString speedName = QString("SAV_%1_SPEED").arg(s);

        auto* paramMgr = vehicle->parameterManager();

        if (paramMgr->parameterExists(componentId, latName) &&
            paramMgr->parameterExists(componentId, lonName) &&
            paramMgr->parameterExists(componentId, altName) &&
            paramMgr->parameterExists(componentId, speedName))
        {
            Fact* latFact = paramMgr->getParameter(componentId, latName);
            Fact* lonFact = paramMgr->getParameter(componentId, lonName);
            Fact* altFact = paramMgr->getParameter(componentId, altName);
            Fact* speedFact = paramMgr->getParameter(componentId, speedName);

            if (latFact && lonFact &&
                latFact->rawValue().isValid() &&
                lonFact->rawValue().isValid() &&
                altFact->rawValue().isValid() &&
                speedFact->rawValue().isValid())
            {
                QVariantMap entry;
                entry["label"] = s.replace("WP_", "");
                entry["latitude"] = latFact->rawValue().toDouble();
                entry["longitude"] = lonFact->rawValue().toDouble();
                entry["altitude"] = altFact->rawValue().toDouble();
                entry["speed"] = speedFact->rawValue().toDouble();
                coordinates.append(entry);
            }
        }  
    }

    return coordinates;
}

QVariantList CustomPlugin::savParamCoordinates() const {
    return _savParamCoordinates;
}

void CustomPlugin::_updateSavParamCoordinates() {
    Vehicle* vehicle = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();
    if (!vehicle || !vehicle->parameterManager()) return;

    if (vehicle->parameterManager()->parameterExists(FactSystem::defaultComponentId, "SAV_ENABLE")){
        Fact* enableFact = vehicle->parameterManager()->getParameter(FactSystem::defaultComponentId, "SAV_ENABLE");
        if (enableFact->rawValue().isValid()){
            if ((enableFact->rawValue().toDouble() > 0) != _isSAVenabled) {
                _isSAVenabled = (enableFact->rawValue().toDouble() > 0);
                emit savEnableChanged();
            }
        }
    }

    if (vehicle->parameterManager()->parameterExists(FactSystem::defaultComponentId, "SAV_NUM_TORR")){
        Fact* existFact = vehicle->parameterManager()->getParameter(FactSystem::defaultComponentId, "SAV_NUM_TORR");
        if (existFact->rawValue().isValid()){
            if ((existFact->rawValue().toDouble() > 0) != _isSAVexist) {
                _isSAVexist = (existFact->rawValue().toDouble() > 0);
                emit isSAVexistChanged();
            }
        }
    }

    QVariantList newCoords = getSavParamCoordinates(vehicle);
    if (newCoords != _savParamCoordinates) {
        _savParamCoordinates = newCoords;
        emit savParamCoordinatesChanged();
    }
}

void CustomPlugin::updateFence(QObject* controllerObj, bool isSAVenabled) {
    GeoFenceController* controller = qobject_cast<GeoFenceController*>(controllerObj);
    if (!controller) {
        qWarning() << "Invalid GeoFenceController passed to updateFence";
        return;
    }

    QString missionPath = qgcApp()->toolbox()->settingsManager()->appSettings()->missionSavePath();
    QDir loadDir(missionPath);

    QString savFile = loadDir.absoluteFilePath("sav.json");
    QString noSavFile = loadDir.absoluteFilePath("no_sav.json");
    if (isSAVenabled) {
        {
            QJsonObject json;
            controller->save(json); 
            QJsonDocument doc(json);
            QFile saveFile(noSavFile);
            if (saveFile.open(QIODevice::WriteOnly)) {
                saveFile.write(doc.toJson());
                saveFile.close();
            } else {
                qWarning() << "cannot save no_sav.json!";
            }
        }
        controller->removeAll();
        QFile file(savFile);
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray bytes = file.readAll();
            QJsonDocument doc = QJsonDocument::fromJson(bytes);
            QJsonObject root  = doc.object();
            QJsonObject jsonToLoad;

        if (root.contains("polygons") && root["polygons"].isArray()) {
            // Formato 1: usa direttamente
            jsonToLoad = root;
        } else if (root.contains("geoFence") && root["geoFence"].isObject()) {
            // Formato 2: estrai sotto-oggetto "geoFence"
            QJsonObject geoFence = root["geoFence"].toObject();
            // Simula il formato 1 ricreando il QJsonObject con le chiavi attese
            QJsonObject simulatedFormat1;
            if (geoFence.contains("polygons"))
                simulatedFormat1.insert("polygons", geoFence["polygons"]);
            if (geoFence.contains("circles"))
                simulatedFormat1.insert("circles", geoFence["circles"]);
            if (geoFence.contains("version"))
                simulatedFormat1.insert("version", geoFence["version"]);

            jsonToLoad = simulatedFormat1;
        }

            QString errorString;
            controller->load(jsonToLoad, errorString);
            if (!errorString.isEmpty()) {
                qgcApp()->showCriticalVehicleMessage(tr("Critical Warning: %1").arg(errorString));
            }
        } else {
            qgcApp()->showCriticalVehicleMessage(tr("Critical Warning: cannot open and load SAV GeoFence file (sav.json)"));
        }
    } else {
        controller->removeAll();
        QFile file(noSavFile);
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray bytes = file.readAll();
            QJsonDocument doc = QJsonDocument::fromJson(bytes);
            QJsonObject json = doc.object();
            QString errorString;
            controller->load(json, errorString);
            if (!errorString.isEmpty()) {
                qgcApp()->showCriticalVehicleMessage(tr("cannot load GeoFence file (no_sav.json): %1").arg(errorString));
            }
        } else {
            qgcApp()->showCriticalVehicleMessage(tr("cannot open and load GeoFence file (no_sav.json)"));
        }
    }
    controller->sendToVehicle();
}


void CustomPlugin::savButtonPressed(const QString& label)
{
    Vehicle* vehicle = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();
    if (!vehicle) {
        qWarning() << "No active vehicle";
        return;
    }

    int value = 0;
    bool ok = false;
    value = label.toInt(&ok);
    int id = 0;

    if (!ok) {
        if (label == "A") id = 1;
        else id = 0;  // default
    }
    else id = (1<<(value));

    vehicle->sendMavCommand(vehicle->defaultComponentId(), MAV_CMD_USER_1, false, id );
}

bool CustomPlugin::isSAVenabled() {
    return _isSAVenabled;
}

bool CustomPlugin::isSAVexist() {
    return _isSAVexist;
}

void CustomPlugin::setSAVenabled(const bool& msg) {
    Vehicle* vehicle = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();
    if (!vehicle || !vehicle->parameterManager()) return;

    if (vehicle->parameterManager()->parameterExists(FactSystem::defaultComponentId, "SAV_ENABLE")){
        Fact* existFact = vehicle->parameterManager()->getParameter(FactSystem::defaultComponentId, "SAV_ENABLE");
        _isSAVexist = msg;    
        existFact->setRawValue(_isSAVexist);
        emit savEnableChanged();  
    }
}


bool CustomPlugin::isAbluoMapPlanEnabled() {
    return _isAbluoMapPlanEnabled;
}

void  CustomPlugin::setAbluoMapPlanEnabled(const bool& msg){
    Vehicle* vehicle = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();
    if (!vehicle || !vehicle->parameterManager()) return;

    _isAbluoMapPlanEnabled = msg;    
    emit abluoMapPlanChanged();  
}


void CustomPlugin::onActiveVehicleChanged(Vehicle* vehicle)
{
    qDebug() << "onActiveVehicleChanged";
    if (!vehicle) {
        qDebug() << "!vehicle";
        if (_vehicleListenerConnected) {
            disconnect(_vehicleConnection);
            _vehicleListenerConnected = false;
        }
        return;
    }

    if (!_vehicleListenerConnected) {
        // Reconnect to the new vehicle's mavlink messages
        _vehicleConnection = connect(vehicle, &Vehicle::mavlinkMessageReceived, this, &CustomPlugin::handleMavlinkMessage);
        _vehicleListenerConnected = true;
        _lastTimeBootMs = 0;
        _audioMuteScheduled = false;
        VehicleLinkManager* vlm = vehicle->vehicleLinkManager();
        connect(vlm, &VehicleLinkManager::communicationLostChanged,
        this, [this, vehicle](bool lost) {
            qDebug() << "Vehicle communication lost changed! Lost:" << lost;
            if (!lost) {
                this->_vehicleConnection = connect(vehicle, &Vehicle::mavlinkMessageReceived, this, &CustomPlugin::handleMavlinkMessage);
                this->_lastTimeBootMs = 0;
                this->_audioMuteScheduled = false;
            }
        });
    }
}

void CustomPlugin::handleMavlinkMessage(const mavlink_message_t& message)
{
    if (message.msgid == MAVLINK_MSG_ID_RC_CHANNELS) {
        mavlink_rc_channels_t rc;
        mavlink_msg_rc_channels_decode(&message, &rc);

        int newRC6 = rc.chan6_raw;  // canale 6 (indice base 1)
        if (_rc6Value != newRC6) {
            _rc6Value = newRC6;
            emit rc6ValueChanged();
        }
        return;
    }
    if (message.msgid == MAVLINK_MSG_ID_SYSTEM_TIME) {
        mavlink_system_time_t sysTime;
        mavlink_msg_system_time_decode(&message, &sysTime);
        _lastTimeBootMs = sysTime.time_boot_ms;

        // Only once, if time_boot_ms < 30000
        if (_lastTimeBootMs > 0 && !_audioMuteScheduled) {
            _audioMuteScheduled = true;
            qgcApp()->toolbox()->settingsManager()->appSettings()->audioMuted()->setRawValue(true);
            UASMessageHandler* msgHandler = qgcApp()->toolbox()->uasMessageHandler();
            if (msgHandler) {
                msgHandler->handleTextMessage(1, 1, 6, "Audio messages deactivated", "");
            }
            int unmuteDelay = 30000 - _lastTimeBootMs;
            if (unmuteDelay < 10000)
                unmuteDelay = 10000; // Ensure at least 10 seconds delay
            if (unmuteDelay > 0) {
                QTimer::singleShot(unmuteDelay, qgcApp(), []() {
                    qgcApp()->toolbox()->settingsManager()->appSettings()->audioMuted()->setRawValue(false);
                    UASMessageHandler* msgHandler = qgcApp()->toolbox()->uasMessageHandler();
                    if (msgHandler) {
                        msgHandler->handleTextMessage(1, 1, 6, "Audio messages re-activated", "");
                    }
                });
            }
        }
        else {
        }
    }
}








