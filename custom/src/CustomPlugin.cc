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

#include <QSettings>


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
    QCoreApplication::setApplicationName(QStringLiteral(QGC_APPLICATION_NAME));  // set the document folder where to save options

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

    qmlEngine->rootContext()->setContextProperty("CustomPlugin", this);

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

    return true; // Show all settings in UI
}

QVariantList& CustomPlugin::settingsPages()
{
    Constants constant;

    if (_customSettingsList.isEmpty()) {
        // Get default pages from QGCCorePlugin
        QVariantList baseSettings = QGCCorePlugin::settingsPages();
        _customSettingsList = baseSettings;

        // Add NTRIP page
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

        // Add About page
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
    _detachGpsFixWatcher();
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
        setAbluoMissionCount(0);
        _detachTripWatchers();

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
    _attachTripWatchers(vehicle);

    // ---------------------------------------------------------------------
    // Hook MissionManager
    // ---------------------------------------------------------------------
    MissionManager* mm = vehicle->missionManager();
    if (mm) {

        // local helper function: count how many waypoints the current mission has
        auto updateCount = [this, mm]() {
            // missionItems() is the internal MissionManager list of MissionItem*
            const auto items = mm->missionItems();
            const int count = items.count();
            setAbluoMissionCount(count);
        };

        // initial state: current waypoint index and number of WPs
        setAbluoCurrentWp(mm->currentIndex());

        updateCount(); // immediately set abluoMissionCount() when attaching

        // runtime updates of the current waypoint index
        connect(
            mm,
            &MissionManager::currentIndexChanged,
            this,
            [this](int idx) {
                setAbluoCurrentWp(idx);
            },
            Qt::UniqueConnection
        );

        // when the mission is written to the vehicle (upload complete)
        connect(
            mm,
            &MissionManager::sendComplete,
            this,
            [updateCount](bool ok){
                updateCount();
            },
            Qt::UniqueConnection
        );

        // when the mission is cleared from the vehicle
        connect(
            mm,
            &MissionManager::removeAllComplete,
            this,
            [updateCount](bool ok){
                updateCount();
            },
            Qt::UniqueConnection
        );

    } else {
        // no MissionManager = no mission
        setAbluoCurrentWp(-1);
        setAbluoMissionCount(0);
    }

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

    _attachGpsFixWatcher(vehicle);
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
                                          double pitch_m,
                                          int orientationMode)
{
    QVariantList out;
    if (!s.isValid() || !t.isValid()) return out;

    auto pushIfDiff = [](QVariantList& lst, const QGeoCoordinate& c) {
        if (lst.isEmpty()) { lst << QVariant::fromValue(c); return; }
        const QGeoCoordinate last = lst.last().value<QGeoCoordinate>();
        // minimum difference: ~1 cm in XY or 1 cm in altitude
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

    // --- 0) trivial case: S and T share the same XY → vertical-only steps ---
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

    // ================== VERTICAL MODE (sweep along S→T) ==================
    if (orientationMode == 1) {
        // geometry along the geodesic from S to T
        const double L      = s.distanceTo(t);
        if (!(L > 0.0)) {
            // fallback: same as trivial
            QGeoCoordinate cur = s; cur.setAltitude(z0);
            pushIfDiff(out, cur);
            out << QVariant::fromValue(cur);
            QGeoCoordinate tt = t; tt.setAltitude(z1);
            pushIfDiff(out, tt);
            return out;
        }
        const double bearing = s.azimuthTo(t);

        // 1) S(z0) -> vertical climb/descend to z1
        QGeoCoordinate cur = s; cur.setAltitude(z0);
        pushIfDiff(out, cur);
        out << QVariant::fromValue(cur);
        if (std::abs(z1 - z0) > 1e-9) {
            QGeoCoordinate v = cur; v.setAltitude(z1);
            pushIfDiff(out, v);
            cur = v;
        }
        double currentAlt = z1;

        // 2) intermediate points every pitch_m along S->T
        //    d = pitch, 2*pitch, ..., < L
        for (double d = dz; d < L - 1e-6; d += dz) {
            QGeoCoordinate pk = s.atDistanceAndAzimuth(d, bearing);
            // horizontal in XY to pk at the current altitude
            pk.setAltitude(currentAlt);
            pushIfDiff(out, pk);

            // alternate altitude: z1 -> z0 -> z1 -> ...
            currentAlt = (std::abs(currentAlt - z1) < 1e-9) ? z0 : z1;
            QGeoCoordinate pv = pk; pv.setAltitude(currentAlt);
            pushIfDiff(out, pv);
            cur = pv;
        }

        // 3) go to T at the current altitude
        QGeoCoordinate tPlan = t; tPlan.setAltitude(currentAlt);
        pushIfDiff(out, tPlan);

        // 4) final snap to T(z1)
        if (std::abs(currentAlt - z1) > 1e-9) {
            QGeoCoordinate tZ = t; tZ.setAltitude(z1);
            pushIfDiff(out, tZ);
        }
        return out;
    }

    // ================== HORIZONTAL MODE (the original logic) ==================
    // 1) starting point: exact S
    QGeoCoordinate cur = s; cur.setAltitude(z0);
    pushIfDiff(out, cur);
    out << QVariant::fromValue(cur);

    // "first horizontal": S -> T's XY at altitude z0
    {
        QGeoCoordinate h = cur;
        h.setLatitude (t.latitude());
        h.setLongitude(t.longitude());
        // same altitude (z0)
        pushIfDiff(out, h);
        cur = h;
    }

    // 3) loop: go vertically toward z1 on current side, then horizontally to the opposite side
    bool atSideS = false; // after the first horizontal, we are on T side
    while ((dir > 0 && cur.altitude() < z1) || (dir < 0 && cur.altitude() > z1)) {
        // vertical step
        {
            const double rem  = std::abs(z1 - cur.altitude());
            const double step = std::min(dz, rem);
            QGeoCoordinate v = cur; v.setAltitude(cur.altitude() + dir*step);
            pushIfDiff(out, v);
            cur = v;
        }
        if (std::abs(cur.altitude() - z1) < 1e-9) break;

        // horizontal translation to the opposite side (flip between S and T XY) at same altitude
        QGeoCoordinate h = cur;
        if (atSideS) {
            h.setLatitude (t.latitude());
            h.setLongitude(t.longitude());
        } else {
            h.setLatitude (s.latitude());
            h.setLongitude(s.longitude());
        }
        pushIfDiff(out, h);
        cur = h;
        atSideS = !atSideS;
    }

    // 4) closure: make sure we end exactly on T (XY(T), z1)
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

void CustomPlugin::uploadAbluoMission(const QVariantList& points, int orientationMode)
{
    Vehicle* vehicle = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();
    if (!vehicle) { qWarning() << "[CustomPlugin] uploadAbluoMission: no active vehicle"; return; }
    MissionManager* mm = vehicle->missionManager();
    if (!mm) { qWarning() << "[CustomPlugin] uploadAbluoMission: no MissionManager"; return; }

    if (mm->inProgress()) {
        qWarning() << "[CustomPlugin] MissionManager busy; aborting upload";
        return;
    }

    // 1) Normalize points -> QList<QGeoCoordinate> (guarantee finite values)
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
    setAbluoMissionCount(wps.size());

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
    cacheResumeIndex(-1);
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
    setAbluoMissionCount(0);
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

void CustomPlugin::setAbluoMissionCount(int c) {
    if (_abluoMissionCount == c) return;
    _abluoMissionCount = c;
    emit abluoMissionCountChanged();
}

void CustomPlugin::_detachGpsFixWatcher()
{
    if (_gpsFixConn) {
        disconnect(_gpsFixConn);
        _gpsFixConn = QMetaObject::Connection{};
    }
    _lastFixType = -1;
}

void CustomPlugin::_attachGpsFixWatcher(Vehicle* v)
{
    _detachGpsFixWatcher();
    if (!v) return;

    FactGroup* gps = v->gpsFactGroup();
    if (!gps) return;

    Fact* lockFact = gps->getFact(QStringLiteral("lock"));
    if (!lockFact) {
        QTimer::singleShot(500, this, [this, v](){ _attachGpsFixWatcher(v); });
        return;
    }

    _lastFixType = lockFact->rawValue().toInt(); 

    _gpsFixConn = connect(lockFact, &Fact::rawValueChanged, this, [this, lockFact]() {
        const int newFix = lockFact->rawValue().toInt();

        if (_lastFixType < 0) {     
            _lastFixType = newFix;
            return;
        }

        const bool toFixed    = (_lastFixType < 6 && newFix == 6);
        const bool fromFixed  = (_lastFixType == 6 && newFix < 6);

        if (toFixed) {
            sendLogMessage(tr("GPS RTK fixed reached."), QString(), QStringLiteral("Warning")); 
            qgcApp()->toolbox()->audioOutput()->say("GPS RTK fixed reached");
        } else if (fromFixed) {
            sendLogMessage(tr("GPS RTK fixed lost."), QString(), QStringLiteral("Critical"));
            qgcApp()->toolbox()->audioOutput()->say("GPS RTK fixed lost");
        }

        _lastFixType = newFix;
    });
}

QString CustomPlugin::_tripKey(Vehicle* v)
{
    // Per single-vehicle va benissimo.
    // Se vuoi più robustezza, possiamo includere anche sysid (se accessibile).
    return QStringLiteral("TripFlight/%1").arg(v ? v->id() : -1);
}

QString CustomPlugin::_fmtHhMmSs(qulonglong s)
{
    const qulonglong h = s / 3600ULL;
    const qulonglong m = (s % 3600ULL) / 60ULL;
    const qulonglong sec = s % 60ULL;
    return QString::asprintf("%04llu:%02llu:%02llu",
                             (unsigned long long)h,
                             (unsigned long long)m,
                             (unsigned long long)sec);
}

void CustomPlugin::_saveTripBaseline(Vehicle* v, qulonglong flt, qulonglong boot)
{
    if (!v) return;
    QSettings s;
    const QString k = _tripKey(v);
    s.setValue(k + "/valid", true);
    s.setValue(k + "/baseline_flt", QVariant::fromValue<qulonglong>(flt));
    s.setValue(k + "/baseline_boot", QVariant::fromValue<qulonglong>(boot));

    _tripBaselineValid = true;
    _tripBaselineFlt = flt;
    _tripBaselineBoot = boot;
}

void CustomPlugin::_clearTripBaseline(Vehicle* v)
{
    if (!v) return;
    QSettings s;
    s.remove(_tripKey(v));

    _tripBaselineValid = false;
    _tripBaselineFlt = 0;
    _tripBaselineBoot = 0;
}

void CustomPlugin::_recomputeTrip(Vehicle* v)
{
    if (!v || !v->armed() || !_tripBaselineValid || !_statFltTimeFact) {
        if (_tripFlightTimeSec != 0 || _tripFlightTimeStr != QStringLiteral("0000:00:00")) {
            _tripFlightTimeSec = 0;
            _tripFlightTimeStr = QStringLiteral("0000:00:00");
            emit tripFlightTimeChanged();
        }
        return;
    }

    const qulonglong nowFlt = _statFltTimeFact->rawValue().toULongLong();
    qulonglong trip = 0;
    if (nowFlt >= _tripBaselineFlt)
        trip = nowFlt - _tripBaselineFlt;

    const QString str = _fmtHhMmSs(trip);
    if (_tripFlightTimeSec != trip || _tripFlightTimeStr != str) {
        _tripFlightTimeSec = trip;
        _tripFlightTimeStr = str;
        emit tripFlightTimeChanged();
    }
}

void CustomPlugin::_tryRestoreTripBaseline(Vehicle* v)
{
    if (!v || !v->armed()) return;
    if (!_statFltTimeFact || !_statBootCntFact) return;

    QSettings s;
    const QString k = _tripKey(v);
    if (!s.value(k + "/valid", false).toBool())
        return;

    const qulonglong savedFlt  = s.value(k + "/baseline_flt").toULongLong();
    const qulonglong savedBoot = s.value(k + "/baseline_boot").toULongLong();

    const qulonglong nowBoot = _statBootCntFact->rawValue().toULongLong();
    const qulonglong nowFlt  = _statFltTimeFact->rawValue().toULongLong();

    // Sicurezza: se autopilot reboot o dato incoerente -> non ripristinare
    if (nowBoot != savedBoot) return;
    if (nowFlt < savedFlt) return;

    _tripBaselineValid = true;
    _tripBaselineFlt = savedFlt;
    _tripBaselineBoot = savedBoot;
}

void CustomPlugin::_detachTripWatchers()
{
    if (_fltTimeConn) { disconnect(_fltTimeConn); _fltTimeConn = {}; }
    if (_bootCntConn) { disconnect(_bootCntConn); _bootCntConn = {}; }
    if (_armedConn)   { disconnect(_armedConn);   _armedConn = {}; }

    _statFltTimeFact = nullptr;
    _statBootCntFact = nullptr;

    if (_tripProbeTimer) {
        _tripProbeTimer->stop();
        _tripProbeTimer->deleteLater();
        _tripProbeTimer = nullptr;
    }

    // Non azzerare baseline qui: se QGC cambia veicolo ok, ma
    // se crasha e riparte la baseline deve restare in QSettings.
    _tripBaselineValid = false;
    _tripBaselineFlt = 0;
    _tripBaselineBoot = 0;

    // reset UI
    _tripFlightTimeSec = 0;
    _tripFlightTimeStr = QStringLiteral("0000:00:00");
    emit tripFlightTimeChanged();
    _stopTripTickTimer();
}

void CustomPlugin::_startTripProbeTimer(ParameterManager* pm)
{
    if (_tripProbeTimer) return;

    _tripProbeTimer = new QTimer(this);
    _tripProbeTimer->setInterval(500);
    _tripProbeTimer->setSingleShot(false);

    connect(_tripProbeTimer, &QTimer::timeout, this, [this, pm]() {
        if (!pm) return;

        const int comp = FactSystem::defaultComponentId;

        if (!_statFltTimeFact && pm->parameterExists(comp, QStringLiteral("STAT_FLTTIME"))) {
            _statFltTimeFact = pm->getParameter(comp, QStringLiteral("STAT_FLTTIME"));
            if (_statFltTimeFact) {
                _fltTimeConn = connect(_statFltTimeFact, &Fact::rawValueChanged, this, [this]() {
                    Vehicle* av = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();
                    _recomputeTrip(av);
                });
            }
        }

        if (!_statBootCntFact && pm->parameterExists(comp, QStringLiteral("STAT_BOOTCNT"))) {
            _statBootCntFact = pm->getParameter(comp, QStringLiteral("STAT_BOOTCNT"));
        }

        if (_statFltTimeFact && _statBootCntFact) {
            _tripProbeTimer->stop();
            _tripProbeTimer->deleteLater();
            _tripProbeTimer = nullptr;

            Vehicle* av = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();

            _tryRestoreTripBaseline(av);

            if (av && av->armed() && !_tripBaselineValid) {
                const qulonglong flt  = _statFltTimeFact->rawValue().toULongLong();
                const qulonglong boot = _statBootCntFact->rawValue().toULongLong();
                qDebug() << "[Trip] probe: creating baseline now flt=" << flt << "boot=" << boot;
                _saveTripBaseline(av, flt, boot);
            }

            if (av && av->armed() && _tripBaselineValid) {
                if (_statFltTimeFact) {
                    const qulonglong statNow = _statFltTimeFact->rawValue().toULongLong();
                    _tripLocalSec = (statNow >= _tripBaselineFlt) ? (statNow - _tripBaselineFlt) : 0;
                    _lastStatFltSeen = statNow;
                } else {
                    _tripLocalSec = 0;
                    _lastStatFltSeen = 0;
                }
                _startTripTickTimer();
            }

            // 4) update UI
            _recomputeTrip(av);
        }

    });

    _tripProbeTimer->start();
}

void CustomPlugin::_attachTripWatchers(Vehicle* v)
{
    _detachTripWatchers();
    if (!v || !v->parameterManager()) return;

    ParameterManager* pm = v->parameterManager();
    const int comp = FactSystem::defaultComponentId;

    // hook armedChanged
    _armedConn = connect(v, &Vehicle::armedChanged, this, [this](bool armed) {
        Vehicle* av = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();
        if (!av) return;

        if (!armed) {
            _stopTripTickTimer();
            _clearTripBaseline(av);

            _tripLocalSec = 0;
            _lastStatFltSeen = 0;
            _tripUsingLocalTick = false;

            _tripFlightTimeSec = 0;
            _tripFlightTimeStr = QStringLiteral("0000:00:00");
            emit tripFlightTimeChanged();
            return;
        }

        // armed=true
        // reset tick counters for this flight
        _tripLocalSec = 0;
        _tripUsingLocalTick = false;
        _lastStatFltSeen = _statFltTimeFact ? _statFltTimeFact->rawValue().toULongLong() : 0;

        if (_statFltTimeFact && _statBootCntFact) {
            const qulonglong flt  = _statFltTimeFact->rawValue().toULongLong();
            const qulonglong boot = _statBootCntFact->rawValue().toULongLong();
            _saveTripBaseline(av, flt, boot);
        } 

        if (_tripBaselineValid) {
            _startTripTickTimer();
        }
    }, Qt::UniqueConnection);


    // try immediately
    if (pm->parameterExists(comp, QStringLiteral("STAT_FLTTIME"))) {
        _statFltTimeFact = pm->getParameter(comp, QStringLiteral("STAT_FLTTIME"));
        if (_statFltTimeFact) {
            _fltTimeConn = connect(_statFltTimeFact, &Fact::rawValueChanged, this, [this]() {
                Vehicle* av = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();
                _recomputeTrip(av);
            });
        }
    }
    if (pm->parameterExists(comp, QStringLiteral("STAT_BOOTCNT"))) {
        _statBootCntFact = pm->getParameter(comp, QStringLiteral("STAT_BOOTCNT"));
    }

    if (!(_statFltTimeFact && _statBootCntFact)) {
        _startTripProbeTimer(pm);
        return;
    }

    _tryRestoreTripBaseline(v);

    if (v && v->armed()) {
        if (!_tripBaselineValid) {
            const qulonglong flt  = _statFltTimeFact->rawValue().toULongLong();
            const qulonglong boot = _statBootCntFact->rawValue().toULongLong();
            _saveTripBaseline(v, flt, boot);
        }

        if (_tripBaselineValid) {
            _tripLocalSec = 0;
            _lastStatFltSeen = _statFltTimeFact->rawValue().toULongLong();
            _startTripTickTimer();
        }
    }

    _recomputeTrip(v);
}

void CustomPlugin::_startTripTickTimer()
{
    if (_tripTickTimer) return;

    _tripTickTimer = new QTimer(this);
    _tripTickTimer->setInterval(1000);
    _tripTickTimer->setSingleShot(false);

    connect(_tripTickTimer, &QTimer::timeout, this, [this]() {
        qDebug() << "[TripTick] local =" << _tripLocalSec;
        Vehicle* v = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();
        if (!v || !v->armed() || !_tripBaselineValid) {
            return;
        }
        _tripLocalSec += 1;
        if (_statFltTimeFact) {
            const qulonglong statNow = _statFltTimeFact->rawValue().toULongLong();

            if (statNow != _lastStatFltSeen) {
                _lastStatFltSeen = statNow;

                if (statNow >= _tripBaselineFlt) {
                    const qulonglong statTrip = statNow - _tripBaselineFlt;

                    if (qAbs((qint64)statTrip - (qint64)_tripLocalSec) > 2) {
                        _tripLocalSec = statTrip;
                    }
                }
            }
        }
        
        const QString str = _fmtHhMmSs(_tripLocalSec);
        if (_tripFlightTimeSec != _tripLocalSec || _tripFlightTimeStr != str) {
            _tripFlightTimeSec = _tripLocalSec;
            _tripFlightTimeStr = str;
            emit tripFlightTimeChanged();
        }
    });



    _tripTickTimer->start();
}

void CustomPlugin::_stopTripTickTimer()
{
    if (!_tripTickTimer) return;
    _tripTickTimer->stop();
    _tripTickTimer->deleteLater();
    _tripTickTimer = nullptr;
}
