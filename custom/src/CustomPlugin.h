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

class CustomOptions;
class CustomPlugin;
// class CustomSettings;


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
public:
    CustomPlugin(QGCApplication* app, QGCToolbox *toolbox);
    ~CustomPlugin() {}

//     // Overrides from QGCCorePlugin
    QGCOptions*             options                         (void) final {return _options;}
    QString                 brandImageIndoor                (void) const final { return QStringLiteral("/custom/img/CustomAppIcon.png");}
    QString                 brandImageOutdoor               (void) const final {    return QStringLiteral("/custom/img/CustomAppIcon.png");}
    bool                    overrideSettingsGroupVisibility (QString name) final;
    // void                    paletteOverride                 (QString colorName, QGCPalette::PaletteColorInfo_t& colorInfo) final;
    QQmlApplicationEngine*  createQmlApplicationEngine      (QObject* parent) final;

    private:
    CustomOptions*  _options = nullptr;
};
