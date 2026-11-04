#pragma once

class UISizingPolicy
{
public:
    // Configuration describing the desired appearance at 100%
    struct Reference {
        double viewportWidth = 0.0;
        double viewportHeight = 0.0;
        double logicalDpi = 0.0;
        double fontPointSize = 0.0;
    };
    
    // Current device metrics and user's preferences
    struct Input {
        double viewportWidth = 0.0;
        double viewportHeight = 0.0;
        double logicalDpi = 0.0;
        double userScalePercent = 100.0;
    };

    // Bounds expressed in QT font pixels 
    struct FontLimits {
        double minimumPixelSize = 0.0;
        double maximumPixelSize = 0.0;
    };

    struct Result {
        bool valid = false;
        double viewportScale = 0.0;
        double baselinePointSize = 0.0;
        double effectivePointSize = 0.0;
        double appliedScalePercent = 0.0;
    };

    static Result calculate(
        const Input& input,
        const Reference& reference,
        const FontLimits& limits
    );
};