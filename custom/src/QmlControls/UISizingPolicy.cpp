#include "UISizingPolicy.h"

#include <algorithm>
#include <cmath>

namespace {
    constexpr double pointsPerInch = 72.0;

    bool isPositiveFinite(double value)
    {
        return std::isfinite(value) && value > 0.0;
    }

    double clamp(double value, double minimum, double maximum)
    {
        return std::max(minimum, std::min(value, maximum));
    }
}

UISizingPolicy::Result UISizingPolicy::calculate(
    const Input &input,
    const Reference &reference, 
    const FontLimits &limits)
{
    Result result;
    if (!isPositiveFinite(input.viewportWidth) ||
        !isPositiveFinite(input.viewportHeight) ||
        !isPositiveFinite(input.logicalDpi) ||
        !isPositiveFinite(input.userScalePercent) ||
        !isPositiveFinite(reference.viewportWidth) ||
        !isPositiveFinite(reference.viewportHeight) ||
        !isPositiveFinite(reference.logicalDpi) ||
        !isPositiveFinite(reference.fontPointSize) ||
        !isPositiveFinite(limits.minimumPixelSize) ||
        !isPositiveFinite(limits.maximumPixelSize) ||
        limits.maximumPixelSize < limits.minimumPixelSize) {
        return result;
    }

    const double viewportScale = std::min(
        input.viewportWidth / reference.viewportWidth,
        input.viewportHeight / reference.viewportHeight
    );

    const double referencePixelScale = 
        reference.fontPointSize * reference.logicalDpi / pointsPerInch;
    
    const double requestedBaselinePixelSize =
        referencePixelScale * viewportScale;

    if (!isPositiveFinite(viewportScale) ||
        !isPositiveFinite(requestedBaselinePixelSize)) {
        return result;
    }

    const double baselinePixelSize = clamp(
        requestedBaselinePixelSize,
        limits.minimumPixelSize,
        limits.maximumPixelSize
    );

    const double requestedEffectivePixelSize =
        baselinePixelSize * (input.userScalePercent / 100.0);

    if (!isPositiveFinite(requestedEffectivePixelSize)) {
        return result;
    }

    const double effectivePixelSize = clamp(
        requestedEffectivePixelSize,
        limits.minimumPixelSize,
        limits.maximumPixelSize
    );

    const double pixelsPerPoint = input.logicalDpi / pointsPerInch;
    const double baselinePointSize = baselinePixelSize / pixelsPerPoint;
    const double effectivePointSize = effectivePixelSize / pixelsPerPoint;
    const double appliedScalePercent =
        effectivePixelSize / baselinePixelSize * 100.0;

    if (!isPositiveFinite(baselinePointSize)
            || !isPositiveFinite(effectivePointSize)
            || !isPositiveFinite(appliedScalePercent)) {
        return result;
    }

    result.valid = true;
    result.viewportScale = viewportScale;
    result.baselinePointSize = baselinePointSize;
    result.effectivePointSize = effectivePointSize;
    result.appliedScalePercent = appliedScalePercent;

    return result;
}