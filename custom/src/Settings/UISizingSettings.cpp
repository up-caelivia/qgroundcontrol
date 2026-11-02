#include "UISizingSettings.h"
#include "SettingsFact.h"

#include <cmath>

DECLARE_SETTINGGROUP(UISizing, "UISizing")
{
}

DECLARE_SETTINGSFACT(UISizingSettings, referenceViewportWidth)
DECLARE_SETTINGSFACT(UISizingSettings, referenceViewportHeight)
DECLARE_SETTINGSFACT(UISizingSettings, referenceLogicalDpi)
DECLARE_SETTINGSFACT(UISizingSettings, referenceFontPointSize)

DECLARE_SETTINGSFACT(UISizingSettings, minimumFontPixelSize)
DECLARE_SETTINGSFACT(UISizingSettings, maximumFontPixelSize)

DECLARE_SETTINGSFACT(UISizingSettings, userScalePercent)