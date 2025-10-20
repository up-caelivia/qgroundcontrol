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

#include <QVariant>
#include <QGeoCoordinate>
#include <QVariantMap>
#include <QtGlobal>
#include <cmath>

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

    Q_ASSERT(QThread::currentThread() == qApp->thread());

    qRegisterMetaType<QGeoCoordinate>("QGeoCoordinate");
    qRegisterMetaType<QList<QGeoCoordinate>>("QList<QGeoCoordinate>");
    qRegisterMetaType<QVariantList>("QVariantList");

    _savParamTimer = new QTimer(this);
    connect(_savParamTimer, &QTimer::timeout, this, &CustomPlugin::_updateSavParamCoordinates);
    _savParamTimer->start(1000);
}

CustomPlugin::~CustomPlugin()
{
    if (_savParamTimer) {
        // Stop in the timer's own thread (GUI) to avoid killTimer warnings
        QMetaObject::invokeMethod(_savParamTimer, "stop", Qt::BlockingQueuedConnection);
        _savParamTimer->deleteLater();
        _savParamTimer = nullptr;
    }
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

void CustomPlugin::setStart()
{
    Vehicle* v = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();
    if (!v) {
        qWarning() << "[CustomPlugin] setStart: no active vehicle";
        return;
    }

    QGeoCoordinate c = v->coordinate();
    if (!c.isValid()) {
        qWarning() << "[CustomPlugin] setStart: invalid vehicle coordinate";
        return;
    }

    // Altitudine: preferisci AGL (altitudeRelative), fallback AMSL, poi quella già in coordinate
    double alt = c.altitude();
    if (v->altitudeRelative()) {
        alt = v->altitudeRelative()->rawValue().toDouble();
    } else if (v->altitudeAMSL()) {
        alt = v->altitudeAMSL()->rawValue().toDouble();
    }
    c.setAltitude(alt);

    if (_startCoordinate != c) {
        _startCoordinate = c;
        emit startCoordinateChanged();
        qDebug() << "[CustomPlugin] setStart ->" << c;
    }
}

void CustomPlugin::setStop()
{
    Vehicle* v = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();
    if (!v) {
        qWarning() << "[CustomPlugin] setStop: no active vehicle";
        return;
    }

    QGeoCoordinate c = v->coordinate();
    if (!c.isValid()) {
        qWarning() << "[CustomPlugin] setStop: invalid vehicle coordinate";
        return;
    }

    double alt = c.altitude();
    if (v->altitudeRelative()) {
        alt = v->altitudeRelative()->rawValue().toDouble();
    } else if (v->altitudeAMSL()) {
        alt = v->altitudeAMSL()->rawValue().toDouble();
    }
    c.setAltitude(alt);

    if (_stopCoordinate != c) {
        _stopCoordinate = c;
        emit stopCoordinateChanged();
        qDebug() << "[CustomPlugin] setStop  ->" << c;
    }
}

static QGeoCoordinate interpAlong(const QGeoCoordinate& a,
                                  const QGeoCoordinate& b,
                                  double tt)
{
    const double t = std::max(0.0, std::min(1.0, tt));
    const double az = a.azimuthTo(b);
    const double d  = a.distanceTo(b);
    return a.atDistanceAndAzimuth(d * t, az);
}

QVariantList CustomPlugin::buildAbluoPath(const QGeoCoordinate& s,
                                          const QGeoCoordinate& t,
                                          double pitch_m,
                                          bool sideLeft)
{
    QVariantList out;
    if (!s.isValid() || !t.isValid()) {
        qWarning() << "[Abluo] buildAbluoPath: invalid S/T";
        return out;
    }

    // Quote: interpretiamo s.altitude()/t.altitude() come AGL (relativa)
    const double z0 = std::isfinite(s.altitude()) ? s.altitude() : 0.0;
    const double z1 = std::isfinite(t.altitude()) ? t.altitude() : 0.0;

    // rettangolo lat/lon allineato ai meridiani: BL, BR, TL, TR
    const double south = std::min(s.latitude(),  t.latitude());
    const double north = std::max(s.latitude(),  t.latitude());
    const double west  = std::min(s.longitude(), t.longitude());
    const double east  = std::max(s.longitude(), t.longitude());

    const QGeoCoordinate bl(south, west);
    const QGeoCoordinate br(south, east);
    const QGeoCoordinate tl(north, west);
    const QGeoCoordinate tr(north, east);

    if (z0 == z1) {
        // Nessuna estensione verticale: tratta come singolo segmento dal lato di S al lato di T alla stessa quota
        const bool sLeft  = sideLeft;           // S lato scelto dall'utente
        const QGeoCoordinate pS = sLeft ? interpAlong(bl, tl, 0.0) : interpAlong(br, tr, 0.0);
        QGeoCoordinate pT = sLeft ? interpAlong(br, tr, 0.0) : interpAlong(bl, tl, 0.0);
        QGeoCoordinate pS3 = pS; pS3.setAltitude(z0);
        QGeoCoordinate pT3 = pT; pT3.setAltitude(z1);

        out.append(QVariant::fromValue(pS3));
        out.append(QVariant::fromValue(pT3));
        return out;
    }

    const double zBottom = std::min(z0, z1);
    const double zTop    = std::max(z0, z1);
    const double totalH  = std::max(0.001, zTop - zBottom);
    const double stepH   = std::max(0.001, pitch_m);

    // Funzioni bordo sinistro/destro alla frazione verticale t=[0..1] (0=bottom; 1=top)
    auto leftAt  = [&](double t) { return interpAlong(bl, tl, t); };
    auto rightAt = [&](double t) { return interpAlong(br, tr, t); };

    // Lato iniziale (sinistra=west / destra=east) secondo sideLeft
    bool goToRight = sideLeft; // primo orizzontale: se S a sinistra vado a destra, viceversa

    // Altitudine corrente
    double y = z0;
    // Direzione verticale: verso T
    const double dir = (z1 > z0) ? +1.0 : -1.0;

    auto frac = [&](double z) { return (z - zBottom) / totalH; };

    // Punto iniziale S sulla parete (bordo scelto) alla quota z0
    const double t0 = frac(z0);
    QGeoCoordinate pL0 = leftAt(t0);  pL0.setAltitude(z0);
    QGeoCoordinate pR0 = rightAt(t0); pR0.setAltitude(z0);
    QGeoCoordinate pS  = sideLeft ? pL0 : pR0;

    out.append(QVariant::fromValue(pS));

    // Loop a gradini fino a T
    while ((dir > 0 && y < z1) || (dir < 0 && y > z1)) {
        // orizzontale alla quota y
        if (goToRight) {
            out.append(QVariant::fromValue(pR0));
        } else {
            out.append(QVariant::fromValue(pL0));
        }

        // step verticale verso T (ultimo step accorciato)
        const double remaining = std::abs(z1 - y);
        const double step = std::min(stepH, remaining);
        y += dir * step;

        // punto alla nuova quota
        const double tt = frac(y);
        QGeoCoordinate pL = leftAt(tt);  pL.setAltitude(y);
        QGeoCoordinate pR = rightAt(tt); pR.setAltitude(y);

        // verticale sul bordo corrente
        if (goToRight) {
            out.append(QVariant::fromValue(pR));
        } else {
            out.append(QVariant::fromValue(pL));
        }

        // prepara prossima iterazione (aggiorna pL0/pR0 alla nuova quota)
        pL0 = pL;
        pR0 = pR;
        goToRight = !goToRight;
    }

    // Chiudi esattamente su T: T è sul lato opposto rispetto a S
    const bool tIsRight = !sideLeft;
    QGeoCoordinate pTop = tIsRight ? pR0 : pL0; // pL0/pR0 sono già all’ultima quota (z1)
    // Se l’ultimo punto non è proprio T-side, aggiungi l’orizzontale finale
    if ((tIsRight && (out.isEmpty() || out.last().value<QGeoCoordinate>() != pR0)) ||
        (!tIsRight && (out.isEmpty() || out.last().value<QGeoCoordinate>() != pL0))) {
        out.append(QVariant::fromValue(pTop));
    }

    return out;
}


#include <MissionManager.h>
#include <MissionItem.h>
#include <QGeoCoordinate>
#include <QtMath>
#include <limits>

static bool coordFromVariant(const QVariant& v, QGeoCoordinate& out)
{
    if (v.canConvert<QGeoCoordinate>()) {
        out = v.value<QGeoCoordinate>();
        return out.isValid();
    }
    if (v.type() == QVariant::Map) {
        const auto m = v.toMap();
        const double lat = m.value("latitude",  m.value("lat")).toDouble();
        const double lon = m.value("longitude", m.value("lon")).toDouble();
        const double alt = m.value("altitude",  m.value("alt")).toDouble();
        if (qIsFinite(lat) && qIsFinite(lon)) { out = QGeoCoordinate(lat, lon, alt); return out.isValid(); }
    }
    return false;
}

void CustomPlugin::uploadAbluoMission(const QVariantList& points)
{
    Vehicle* vehicle = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();
    if (!vehicle) { qWarning() << "[CustomPlugin] uploadAbluoMission: no active vehicle"; return; }
    MissionManager* mm = vehicle->missionManager();
    if (!mm) { qWarning() << "[CustomPlugin] uploadAbluoMission: no MissionManager"; return; }

    if (mm->inProgress()) {
        qWarning() << "[CustomPlugin] MissionManager busy; aborting upload";
        return;
    }

    // 1) Normalizza punti -> QList<QGeoCoordinate> (finitezza garantita)
    QList<QGeoCoordinate> wps; wps.reserve(points.size());
    for (const QVariant& v : points) {
        QGeoCoordinate c;
        if (coordFromVariant(v, c) && c.isValid()) {
            const double lat = std::isfinite(c.latitude())  ? c.latitude()  : 0.0;
            const double lon = std::isfinite(c.longitude()) ? c.longitude() : 0.0;
            double alt       = std::isfinite(c.altitude())  ? c.altitude()  : 0.0;
            QGeoCoordinate fixed(lat, lon, alt);
            if (fixed.isValid()) wps.push_back(fixed);
        }
    }
    if (wps.size() < 2) { qWarning() << "[CustomPlugin] uploadAbluoMission: too few waypoints"; return; }

    // 2) Costruisci MissionItem (frame RELATIVE ALT = AGL)
    QList<MissionItem*> items; items.reserve(wps.size() + 1);
    for (int i=0;i<wps.size();++i) {
        const QGeoCoordinate& c = wps[i];
        auto* mi = new MissionItem(
            /*seq*/ i,
            /*cmd*/ MAV_CMD_NAV_WAYPOINT,
            /*frame*/ MAV_FRAME_GLOBAL_RELATIVE_ALT,
            /*p1 hold*/ 0.0,
            /*p2 accept*/ 0.0,
            /*p3 pass*/ 0.0,
            /*p4 yaw*/ 0.0,
            /*x lat*/ c.latitude(),
            /*y lon*/ c.longitude(),
            /*z alt*/ c.altitude(),
            /*autocont*/ true,
            /*isCurrent*/ false,
            /*parent*/ nullptr
        );
        items.push_back(mi);
    }

    // --- APPEND RTL at the end ---
    {
        const int seq = items.size();
        MissionItem* rtl = new MissionItem(
            /*sequenceNumber*/ seq,
            /*command*/        MAV_CMD_NAV_RETURN_TO_LAUNCH,
            /*frame*/          MAV_FRAME_MISSION,
            /*param1 hold*/    0.0,
            /*param2 accept*/  0.0,
            /*param3 pass*/    0.0,
            /*param4 yaw*/     0.0,
            /*x*/              0.0,
            /*y*/              0.0,
            /*z*/              0.0,
            /*autocontinue*/   true,
            /*isCurrentItem*/  false,
            /*parent*/         nullptr
        );
        items.push_back(rtl);
        qDebug() << "[CustomPlugin] appended RTL at sequence" << seq;
    }

    qDebug() << "[CustomPlugin] prepared" << items.size() << "MissionItems. Clear+Upload...";

    if (mm->inProgress()) {
        qWarning() << "[CustomPlugin] MissionManager became busy; aborting upload";
        qDeleteAll(items);
        return;
    }

    // Flag one-shot per evitare doppio upload da fallback/clear
    QSharedPointer<bool> started = QSharedPointer<bool>::create(false);

    QPointer<MissionManager> mmPtr(mm);
    // helper to start upload on GUI thread
    auto startUpload = [mm, items]() {
        qDebug() << "[CustomPlugin] writing mission items...";
        QObject::connect(mm, &MissionManager::sendComplete, mm, [](bool ok){
            qDebug() << "[CustomPlugin] mission upload done:" << ok;
        }, Qt::QueuedConnection);
        mm->writeMissionItems(items);   // ownership moves to MM
    };

    // Create fallback timer on GUI thread
    QPointer<QTimer> fallback = new QTimer(qApp);          // parent = qApp (GUI thread)
    fallback->setSingleShot(true);
    fallback->setInterval(2500);

    // If clear() never answers, run upload anyway
    QObject::connect(fallback, &QTimer::timeout, qApp, [mm, startUpload]() {
        if (!mm || mm->inProgress()) return;
        qWarning() << "[CustomPlugin] clear timeout, fallback upload";
        startUpload();
    }, Qt::QueuedConnection);

    fallback->start();

    // When removeAll completes, stop fallback and upload
    QObject::connect(mm, &MissionManager::removeAllComplete, qApp,
                    [fallback, startUpload](bool /*ok*/) {
        if (fallback) {
            QMetaObject::invokeMethod(fallback, "stop", Qt::QueuedConnection);
            fallback->deleteLater();
        }
        startUpload();
    }, Qt::QueuedConnection);

    mm->removeAll();

}
