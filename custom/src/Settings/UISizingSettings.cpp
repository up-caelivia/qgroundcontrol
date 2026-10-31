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

bool UISizingSettings::saveReference(double viewportWidth, double viewportHeight, double logicalDpi, double fontPointSize)
{
    const double values[] = {
        viewportWidth,
        viewportHeight,
        logicalDpi,
        fontPointSize
    };

    for (double value : values) {
        if (!std::isfinite(value) || value <= 0.0) {
            return false;
        }
    }

    referenceViewportWidth()->setRawValue(viewportWidth);
    referenceViewportHeight()->setRawValue(viewportHeight);
    referenceLogicalDpi()->setRawValue(logicalDpi);
    referenceFontPointSize()->setRawValue(fontPointSize);

    return true;
}