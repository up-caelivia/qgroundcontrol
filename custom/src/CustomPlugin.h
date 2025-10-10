/****************************************************************************
 *
 * (c) 2009-2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 *   @brief Custom QGCCorePlugin Declaration
 *   @author Gus Grubba <gus@auterion.com>
 */

#pragma once

#include "QGCCorePlugin.h"
#include "QGCOptions.h"
#include <QObject>
#include "QmlComponentInfo.h"
#include "CustomToolbox.h"
#include <QGeoCoordinate>
#include "Vehicle.h"

class CustomOptions;
class CustomPlugin;


class CustomFlyViewOptions : public QGCFlyViewOptions
{
public:
    CustomFlyViewOptions(CustomOptions* options, QObject* parent = nullptr);

    // Overrides from CustomFlyViewOptions
    bool                    showInstrumentPanel         (void) const final {return false;}
};


class CustomOptions : public QGCOptions
{
public:
    CustomOptions(CustomPlugin*, QObject* parent = nullptr) : QGCOptions(parent) {}

    // Overrides from QGCOptions
    QGCFlyViewOptions*      flyViewOptions(void) final;

private:
    CustomFlyViewOptions* _flyViewOptions = nullptr;
};


class CustomPlugin : public QGCCorePlugin
{
    Q_OBJECT
    Q_PROPERTY(QString speedMessage READ speedMessage WRITE setSpeedMessage NOTIFY speedMessageChanged)
    Q_PROPERTY(QVariantList savParamCoordinates READ savParamCoordinates NOTIFY savParamCoordinatesChanged)
    Q_PROPERTY(bool isSAVenabled READ isSAVenabled WRITE setSAVenabled NOTIFY savEnableChanged)
    Q_PROPERTY(bool isSAVexist READ isSAVexist NOTIFY isSAVexistChanged)
    Q_PROPERTY(int rc6Value READ rc6Value NOTIFY rc6ValueChanged)
    Q_PROPERTY(bool isAbluoMapPlanEnabled READ isAbluoMapPlanEnabled WRITE setAbluoMapPlanEnabled NOTIFY abluoMapPlanChanged)
    
public:
    CustomPlugin(QGCApplication* app, QGCToolbox *toolbox);
    ~CustomPlugin() {}

    // Overrides from QGCCorePlugin
    QGCOptions*             options                         (void) final {return _options;}
    QString                 brandImageIndoor                (void) const final { return QStringLiteral("/custom/img/CustomAppIcon.png");}
    QString                 brandImageOutdoor               (void) const final {    return QStringLiteral("/custom/img/CustomAppIcon.png");}
    bool                    overrideSettingsGroupVisibility (QString name) final;
    void                    paletteOverride                 (QString colorName, QGCPalette::PaletteColorInfo_t& colorInfo) final;
    bool        adjustSettingMetaData                  (const QString& settingsGroup, FactMetaData& metaData) override;
    QQmlApplicationEngine*  createQmlApplicationEngine      (QObject* parent) override;
    void registerQmlTypes();
    QVariantList&           settingsPages() override;
    void setToolbox(QGCToolbox* toolbox) override;
    Q_INVOKABLE void sendLogMessage(const QString& text, const QString& description = "", const QString& severityStr = "Info");
    void onActiveVehicleChanged(Vehicle* vehicle);
    void handleMavlinkMessage(const mavlink_message_t& message);

    Q_INVOKABLE QVariantList getSavParamCoordinates(Vehicle* vehicle);
    QVariantList savParamCoordinates() const;
    Q_INVOKABLE void savButtonPressed(const QString& label);
    Q_INVOKABLE void updateFence(QObject* controllerObj, bool isSAVenabled);

    Q_INVOKABLE bool isSAVenabled();
    Q_INVOKABLE bool isSAVexist();

    Q_INVOKABLE bool isAbluoMapPlanEnabled();

    QString speedMessage() const { return _speedMessage; }
    void setSpeedMessage(const QString& msg);
    void setSAVenabled(const bool& msg);
    int rc6Value() const { return _rc6Value; }
    void setAbluoMapPlanEnabled(const bool& msg);

signals:
    void speedMessageChanged();
    void savParamCoordinatesChanged();
    void savEnableChanged();
    void isSAVexistChanged();
    void rc6ValueChanged();
    void abluoMapPlanChanged();

private slots:
    void _updateSavParamCoordinates();

private:
    CustomOptions*  _options = nullptr;
    QmlComponentInfo* _ntripSettings = nullptr;
    QmlComponentInfo* _aboutSettings = nullptr;
    QVariantList      _customSettingsList;
    CustomToolbox* _customToolbox = nullptr;
    QString _speedMessage;
    uint32_t _lastTimeBootMs = 0;
    QMetaObject::Connection _vehicleConnection;
    bool _vehicleListenerConnected = false;
    bool _audioMuteScheduled = false;
    
    QVariantList _savParamCoordinates;
    QTimer* _savParamTimer = nullptr;
    bool _isSAVenabled = false;
    bool _isSAVexist = false;
    int _rc6Value = 0;
    bool _isAbluoMapPlanEnabled = false;
};
