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
#include <list>
#include "ParameterManager.h"
#include "GeoFenceController.h"
#include "Fact.h"
#include "FactSystem.h"

#include <MissionManager.h>
#include <MissionItem.h>
#include <QGeoCoordinate>
#include <QtMath>
#include <limits>

#include <QVariant>
#include <QGeoCoordinate>
#include <QVariantMap>
#include <QtGlobal>
#include <cmath>
#include <limits>
#include <QTimer>

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

    // Initialize speed cache
    _wpnavSpeedMps = std::numeric_limits<double>::quiet_NaN();
}

CustomPlugin::~CustomPlugin()
{
    if (_savParamTimer) {
        if (_savParamTimer->thread() == QThread::currentThread()) {
            _savParamTimer->stop();          
        } else {
            QMetaObject::invokeMethod(_savParamTimer, "stop", Qt::QueuedConnection);
        }
        _savParamTimer->deleteLater();
        _savParamTimer = nullptr;
    }
    _detachWpnavWatcher();
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
    Constants constant;

    if (constant.developer())
        return true;

#ifndef ABLUO_APP
    if (name == BrandImageSettings::name || name == AutoConnectSettings::name || name == ADSBVehicleManagerSettings::name || name == RTKSettings::name || name == PlanViewSettings::name) {
#else
    if (name == BrandImageSettings::name || name == AutoConnectSettings::name || name == UnitsSettings::name || name == ADSBVehicleManagerSettings::name || name == RTKSettings::name || name == PlanViewSettings::name) {
#endif
        return false;
    }

    return true;
}

void CustomPlugin::paletteOverride(QString colorName, QGCPalette::PaletteColorInfo_t& colorInfo)
{
#ifdef ABLUO_APP
    QColor color1 = QColor("#004F9F");
    QColor color2 = QColor("#76BEEA");
    QColor color3 = QColor("#76BEEA");
    QColor color4 = QColor("#004F9F");
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

        // add NTRIP page
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

        // add About page
        _aboutSettings = new QmlComponentInfo(
            tr("About"),
            QUrl::fromUserInput("qrc:/Custom/Widgets/AboutUP.qml"),
            QUrl::fromUserInput("qrc:/res/gear-white.svg")
        );

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
            // Save current fence to no_sav.json
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
            // Format 1: use as-is
            jsonToLoad = root;
        } else if (root.contains("geoFence") && root["geoFence"].isObject()) {
            // Format 2: extract nested "geoFence"
            QJsonObject geoFence = root["geoFence"].toObject();
            // Simulate format 1 by reconstructing keys that controller expects
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
        _detachWpnavWatcher();  // reset WPNAV watcher
        _wpnavSpeedMps = std::numeric_limits<double>::quiet_NaN();
        emit wpnavSpeedMpsChanged();
        setAbluoCurrentWp(-1);
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

    // attach/re-attach WPNAV_SPEED watcher
    _attachWpnavWatcher(vehicle);

    // Hook A: MissionManager::currentIndexChanged (works across versions)
    if (vehicle->missionManager()) {
        // initial state
        setAbluoCurrentWp(vehicle->missionManager()->currentIndex());
        qDebug() << "[Abluo] initial mission index =" << vehicle->missionManager()->currentIndex();

        // updates
        connect(vehicle->missionManager(), &MissionManager::currentIndexChanged,
                this, [this](int idx){
                    setAbluoCurrentWp(idx);
                },
                Qt::UniqueConnection);
    }

    // Hook B: raw MAVLink – **single-argument signal** in this branch
    connect(vehicle, &Vehicle::mavlinkMessageReceived,
            this,
            [this](const mavlink_message_t& msg){
                if (msg.msgid == MAVLINK_MSG_ID_MISSION_CURRENT) {
                    mavlink_mission_current_t cur{};
                    mavlink_msg_mission_current_decode(&msg, &cur);
                    setAbluoCurrentWp(static_cast<int>(cur.seq));
                }
            },
            Qt::UniqueConnection);
}

void CustomPlugin::handleMavlinkMessage(const mavlink_message_t& message)
{
    if (message.msgid == MAVLINK_MSG_ID_RC_CHANNELS) {
        mavlink_rc_channels_t rc;
        mavlink_msg_rc_channels_decode(&message, &rc);

        int newRC6 = rc.chan6_raw;  // channel 6 (1-based index)
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
    if (message.msgid == MAVLINK_MSG_ID_MISSION_CURRENT) {
        mavlink_mission_current_t cur{};
        mavlink_msg_mission_current_decode(&message, &cur);
        setAbluoCurrentWp(static_cast<int>(cur.seq));
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

    // Altitude: prefer AGL (altitudeRelative), fallback AMSL, then use existing coordinate altitude
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

[[maybe_unused]]
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
                                          double pitch_m)
{
    QVariantList out;
    if (!s.isValid() || !t.isValid()) return out;

    auto pushIfDiff = [](QVariantList& lst, const QGeoCoordinate& c) {
        if (lst.isEmpty()) { lst << QVariant::fromValue(c); return; }
        const QGeoCoordinate last = lst.last().value<QGeoCoordinate>();
        // minimum difference: ~1 cm in planimetry or 1 cm in altitude
        if ( (!last.isValid() || !c.isValid()) ||
             last.distanceTo(c) >= 0.01 ||
             std::abs(last.altitude() - c.altitude()) >= 0.01 ) {
            lst << QVariant::fromValue(c);
        }
    };

    const double z0  = std::isfinite(s.altitude()) ? s.altitude() : 0.0;
    const double z1  = std::isfinite(t.altitude()) ? t.altitude() : 0.0;
    const double dz  = std::max(0.001, pitch_m);
    const double dir = (z1 >= z0) ? +1.0 : -1.0;

    // --- 0) trivial case: S and T have same XY → vertical-only steps ---
    if (std::abs(s.latitude()  - t.latitude())  < 1e-12 &&
        std::abs(s.longitude() - t.longitude()) < 1e-12)
    {
        QGeoCoordinate cur = s; cur.setAltitude(z0);
        pushIfDiff(out, cur);
        while ( (dir > 0 && cur.altitude() < z1) || (dir < 0 && cur.altitude() > z1) ) {
            const double rem  = std::abs(z1 - cur.altitude());
            const double step = std::min(dz, rem);
            cur.setAltitude(cur.altitude() + dir*step);
            pushIfDiff(out, cur);
        }
        // snap exactly to T (same XY and altitude z1)
        QGeoCoordinate tt = t; tt.setAltitude(z1);
        pushIfDiff(out, tt);
        return out;
    }

    // --- 1) starting point: exact S ---
    QGeoCoordinate cur = s; cur.setAltitude(z0);
    pushIfDiff(out, cur);

    // we are on “S side” (XY = S). This flag marks on which XY the next vertical happens.
    bool atSideS = true;

    // --- 2) first horizontal: S → XY(T) at same altitude z0 ---
    {
        QGeoCoordinate h = cur;
        h.setLatitude (t.latitude());
        h.setLongitude(t.longitude());
        // same altitude (z0)
        pushIfDiff(out, h);
        cur = h;
        atSideS = false; // now we are on T side
    }

    // --- 3) loop: vertical step toward z1 on current side, then horizontal to the opposite side ---
    while ( (dir > 0 && cur.altitude() < z1) || (dir < 0 && cur.altitude() > z1) ) {

        // 3a) VERTICAL on current side: change only altitude
        {
            const double rem  = std::abs(z1 - cur.altitude());
            const double step = std::min(dz, rem);
            QGeoCoordinate v = cur;
            v.setAltitude(cur.altitude() + dir*step);
            pushIfDiff(out, v);
            cur = v;
        }

        // If we reached z1, exit: we'll fix XY to T below.
        if (std::abs(cur.altitude() - z1) < 1e-9) break;

        // 3b) HORIZONTAL to the opposite side at same altitude
        {
            QGeoCoordinate h = cur;
            if (atSideS) {
                // we were on S side → go to XY(T)
                h.setLatitude (t.latitude());
                h.setLongitude(t.longitude());
            } else {
                // we were on T side → go to XY(S)
                h.setLatitude (s.latitude());
                h.setLongitude(s.longitude());
            }
            // same altitude
            pushIfDiff(out, h);
            cur = h;
            atSideS = !atSideS;   // flip side
        }
    }

    // --- 4) closure: ensure we end exactly on T (XY(T), z1) ---
    {
        QGeoCoordinate last = cur;
        // if last XY is not T, do a final horizontal at altitude z1
        if (std::abs(last.latitude()  - t.latitude())  > 1e-12 ||
            std::abs(last.longitude() - t.longitude()) > 1e-12)
        {
            QGeoCoordinate h = last;
            h.setLatitude (t.latitude());
            h.setLongitude(t.longitude());
            // altitude should already be z1 (if not, fix it)
            h.setAltitude(z1);
            pushIfDiff(out, h);
            last = h;
        }
        // ensure altitude is exactly z1 (usually already correct)
        if (std::abs(last.altitude() - z1) > 1e-9) {
            QGeoCoordinate v = last; v.setAltitude(z1);
            pushIfDiff(out, v);
        }
    }

    return out;
}




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

    // 1) Normalize points -> QList<QGeoCoordinate> (finite values guaranteed)
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

    // --- Failsafe: enforce first leg horizontal --------------------------------
    // Dynamically choose the “horizontal” axis: the one with the largest span (in meters).
    auto dist_m = [](double lat1, double lon1, double lat2, double lon2) {
        QGeoCoordinate a(lat1, lon1), b(lat2, lon2);
        return a.distanceTo(b);
    };

    double minLat =  std::numeric_limits<double>::infinity();
    double maxLat = -std::numeric_limits<double>::infinity();
    double minLon =  std::numeric_limits<double>::infinity();
    double maxLon = -std::numeric_limits<double>::infinity();

    for (const auto& c : wps) {
        if (std::isfinite(c.latitude()))  { minLat = std::min(minLat, c.latitude());  maxLat = std::max(maxLat, c.latitude()); }
        if (std::isfinite(c.longitude())) { minLon = std::min(minLon, c.longitude()); maxLon = std::max(maxLon, c.longitude()); }
    }

    // estimate span in meters of both axes at mid-latitude
    double midLat = (std::isfinite(minLat) && std::isfinite(maxLat)) ? (0.5 * (minLat + maxLat)) : 0.0;
    double spanLon_m = (std::isfinite(minLon) && std::isfinite(maxLon))
        ? dist_m(midLat, minLon, midLat, maxLon) : 0.0;
    double spanLat_m = (std::isfinite(minLat) && std::isfinite(maxLat))
        ? dist_m(minLat, 0.0,  maxLat, 0.0)       : 0.0;

    // choose horizontal axis: true → use longitude, false → use latitude
    bool horizByLon = spanLon_m >= spanLat_m;

    // if the first two WPs end up on the same side along the chosen axis, move WP1 to the opposite side
    if (wps.size() >= 2 && std::isfinite(minLon) && std::isfinite(maxLon) && std::isfinite(minLat) && std::isfinite(maxLat)) {
        QGeoCoordinate& p0 = wps[0];
        QGeoCoordinate& p1 = wps[1];

        auto almostEqual = [](double a, double b) { return std::abs(a - b) < 1e-7; };

        if (horizByLon) {
            if (almostEqual(p0.longitude(), p1.longitude())) {
                // push p1 to opposite LONGITUDE side, keeping p0 lat/alt
                double dToMin = std::abs(p0.longitude() - minLon);
                double dToMax = std::abs(p0.longitude() - maxLon);
                double oppLon = (dToMin < dToMax) ? maxLon : minLon;
                p1.setLatitude(p0.latitude());
                p1.setLongitude(oppLon);
                p1.setAltitude(p0.altitude());
                qDebug() << "[CustomPlugin] forced horizontal (lon) first leg";
            }
        } else {
            if (almostEqual(p0.latitude(), p1.latitude())) {
                // push p1 to opposite LATITUDE side, keeping p0 lon/alt
                double dToMin = std::abs(p0.latitude() - minLat);
                double dToMax = std::abs(p0.latitude() - maxLat);
                double oppLat = (dToMin < dToMax) ? maxLat : minLat;
                p1.setLatitude(oppLat);
                p1.setLongitude(p0.longitude());
                p1.setAltitude(p0.altitude());
                qDebug() << "[CustomPlugin] forced horizontal (lat) first leg";
            }
        }
    }
    // ----------------------------------------------------------------------------

    QList<MissionItem*> items; items.reserve(wps.size());
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

    qDebug() << "[CustomPlugin] prepared" << items.size() << "MissionItems. Clear+Upload...";

    if (mm->inProgress()) {
        qWarning() << "[CustomPlugin] MissionManager became busy; aborting upload";
        qDeleteAll(items);
        return;
    }

    // One-shot flag to avoid double upload from fallback/clear
    QSharedPointer<bool> started = QSharedPointer<bool>::create(false);

    // Fallback timer (if removeAll does not respond)
    QPointer<QTimer> fallback = new QTimer(qApp);
    fallback->setSingleShot(true);
    fallback->setInterval(3000);

    auto stopFallback = [fallback]() {
        if (fallback) {
            QMetaObject::invokeMethod(fallback, "stop", Qt::QueuedConnection);
            fallback->deleteLater();
        }
    };

    // ⬇️ Do NOT capture mutable here
    auto startUpload = [mm, items, started, stopFallback]() {
        if (!mm || mm->inProgress()) return;
        if (*started) return;
        *started = true;
        stopFallback();

        qDebug() << "[CustomPlugin] writing mission items...";

        QObject::connect(mm, &MissionManager::sendComplete, qApp, [=](bool ok){
            qDebug() << "[CustomPlugin] mission upload done:" << ok;
            // You may reset any global state flag here if you keep one
        }, Qt::QueuedConnection);

        // (optional) also hook an error signal if present in your QGC branch:
        // QObject::connect(mm, &MissionManager::error, qApp, [](int code, const QString& err){
        //     qWarning() << "[CustomPlugin] Mission upload error:" << code << err;
        // }, Qt::QueuedConnection);

        mm->writeMissionItems(items);   // ownership -> MissionManager
        // Do NOT items.clear() here: not needed and would require mutable capture
    };

    // If clear() does not respond, start anyway
    QObject::connect(fallback, &QTimer::timeout, qApp, [startUpload]() {
        qWarning() << "[CustomPlugin] clear timeout, fallback upload";
        startUpload();
    }, Qt::QueuedConnection);

    fallback->start();

    // When removeAll completes: stop fallback and upload
    QObject::connect(mm, &MissionManager::removeAllComplete, qApp, [startUpload, stopFallback](bool /*ok*/) {
        stopFallback();
        startUpload();
    }, Qt::QueuedConnection);

    mm->removeAll();
    setAbluoCurrentWp(-1);
}

void CustomPlugin::clearAbluoMission()
{
    Vehicle* vehicle = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();
    if (!vehicle) {
        qWarning() << "[CustomPlugin] clearAbluoMission: no active vehicle";
        return;
    }
    MissionManager* mm = vehicle->missionManager();
    if (!mm) {
        qWarning() << "[CustomPlugin] clearAbluoMission: no MissionManager";
        return;
    }

    if (mm->inProgress()) {
        // If an operation is already in progress, wait for it to finish and then retry
        qWarning() << "[CustomPlugin] clearAbluoMission: MissionManager busy, will retry";
        QPointer<MissionManager> mmPtr(mm);
        QObject::connect(mm, &MissionManager::inProgressChanged, qApp, [mmPtr]() {
            if (!mmPtr || mmPtr->inProgress()) return;
            QObject::disconnect(mmPtr, &MissionManager::inProgressChanged, nullptr, nullptr);
            mmPtr->removeAll();
        }, Qt::QueuedConnection);
        return;
    }

    mm->removeAll();
    setAbluoCurrentWp(-1);
}


// ======= NEW: WPNAV_SPEED handling =======
void CustomPlugin::_detachWpnavWatcher()
{
    if (_wpnavConnection) {
        disconnect(_wpnavConnection);
        _wpnavConnection = QMetaObject::Connection{};
    }
    _wpnavSpeedFact = nullptr;

    if (_wpnavProbeTimer) {
        _wpnavProbeTimer->stop();
        _wpnavProbeTimer->deleteLater();
        _wpnavProbeTimer = nullptr;
    }
}

void CustomPlugin::_refreshWpnavFromFact()
{
    if (!_wpnavSpeedFact) return;
    bool ok = false;
    const double raw_cms = _wpnavSpeedFact->rawValue().toDouble(&ok); // ArduPilot: cm/s
    const double mps = ok ? (raw_cms / 100.0) : std::numeric_limits<double>::quiet_NaN();
    const bool bothNaN = (std::isnan(_wpnavSpeedMps) && std::isnan(mps));
    if (!bothNaN && !qFuzzyCompare(_wpnavSpeedMps + 1.0, mps + 1.0)) {
        _wpnavSpeedMps = mps;
        emit wpnavSpeedMpsChanged();
    }
}

void CustomPlugin::_startWpnavProbeTimer(ParameterManager* pm)
{
    if (_wpnavProbeTimer) return; // already active

    _wpnavProbeTimer = new QTimer(this);
    _wpnavProbeTimer->setInterval(500); // half a second
    _wpnavProbeTimer->setSingleShot(false);

    connect(_wpnavProbeTimer, &QTimer::timeout, this, [this, pm]() {
        if (!pm) return;
        const int comp = FactSystem::defaultComponentId;
        if (pm->parameterExists(comp, QStringLiteral("WPNAV_SPEED"))) {
            _wpnavSpeedFact = pm->getParameter(comp, QStringLiteral("WPNAV_SPEED"));
            if (_wpnavSpeedFact) {
                _refreshWpnavFromFact();
                _wpnavConnection = connect(_wpnavSpeedFact, &Fact::rawValueChanged, this, [this]() {
                    _refreshWpnavFromFact();
                });
                // found: stop & cleanup timer
                if (_wpnavProbeTimer) {
                    _wpnavProbeTimer->stop();
                    _wpnavProbeTimer->deleteLater();
                    _wpnavProbeTimer = nullptr;
                }
            }
        }
    });

    _wpnavProbeTimer->start();
}

void CustomPlugin::_attachWpnavWatcher(Vehicle* v)
{
    _detachWpnavWatcher();

    if (!v || !v->parameterManager()) {
        _wpnavSpeedMps = std::numeric_limits<double>::quiet_NaN();
        emit wpnavSpeedMpsChanged();
        return;
    }

    ParameterManager* pm = v->parameterManager();
    const int comp = FactSystem::defaultComponentId;

    // Try immediately
    if (pm->parameterExists(comp, QStringLiteral("WPNAV_SPEED"))) {
        _wpnavSpeedFact = pm->getParameter(comp, QStringLiteral("WPNAV_SPEED"));
        if (_wpnavSpeedFact) {
            _refreshWpnavFromFact();
            _wpnavConnection = connect(_wpnavSpeedFact, &Fact::rawValueChanged, this, [this]() {
                _refreshWpnavFromFact();
            });
            return;
        }
    }

    // If not available yet, start periodic probing until the parameter arrives
    _startWpnavProbeTimer(pm);
}

void CustomPlugin::cacheResumeIndex(int index)
{
    if (index < 0) index = 0;
    if (_cachedResumeIndex != index) {
        _cachedResumeIndex = index;
        emit cachedResumeIndexChanged();
        qDebug() << "[CustomPlugin] cached resume index =" << _cachedResumeIndex;
    }
}

int CustomPlugin::getCachedResumeIndex() const
{
    return _cachedResumeIndex;
}

void CustomPlugin::setAbluoCurrentWp(int v)
{
    _abluoCurrentWp = v;
    emit abluoCurrentWpChanged();
}
