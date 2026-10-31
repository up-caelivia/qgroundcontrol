#pragma once

#include "SettingsGroup.h"
#include "qobjectdefs.h"

class UISizingSettings : public SettingsGroup
{
    Q_OBJECT

public:
    explicit UISizingSettings(QObject* parent = nullptr);
    DEFINE_SETTING_NAME_GROUP()

    DEFINE_SETTINGFACT(referenceViewportWidth)
    DEFINE_SETTINGFACT(referenceViewportHeight)
    DEFINE_SETTINGFACT(referenceLogicalDpi)
    DEFINE_SETTINGFACT(referenceFontPointSize)

    DEFINE_SETTINGFACT(minimumFontPixelSize)
    DEFINE_SETTINGFACT(maximumFontPixelSize)

    DEFINE_SETTINGFACT(userScalePercent)

    Q_INVOKABLE bool saveReference(
        double viewportWidth,
        double viewportHeight,
        double logicalDpi,
        double fontPointSize
    );
};