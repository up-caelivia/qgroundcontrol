#pragma once

#include <QObject>
#include <QSizeF>
#include <QTimer>
#include <QVariantMap>

#include "DisplayMetricsAdapter.h"
#include "UISizingPolicy.h"

class UISizingController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QQuickWindow* window
               READ window WRITE setWindow
               NOTIFY windowChanged)
    Q_PROPERTY(bool enabled
               READ isEnabled WRITE setEnabled
               NOTIFY configurationChanged)
    Q_PROPERTY(bool updatesSuspended
               READ updatesSuspended WRITE setUpdatesSuspended
               NOTIFY configurationChanged)

    Q_PROPERTY(QSizeF referenceViewportSize
               READ referenceViewportSize WRITE setReferenceViewportSize
               NOTIFY configurationChanged)
    Q_PROPERTY(qreal referenceLogicalDpi
               READ referenceLogicalDpi WRITE setReferenceLogicalDpi
               NOTIFY configurationChanged)
    Q_PROPERTY(qreal referenceFontPointSize
               READ referenceFontPointSize WRITE setReferenceFontPointSize
               NOTIFY configurationChanged)
    Q_PROPERTY(qreal minimumFontPixelSize
               READ minimumFontPixelSize WRITE setMinimumFontPixelSize
               NOTIFY configurationChanged)
    Q_PROPERTY(qreal maximumFontPixelSize
               READ maximumFontPixelSize WRITE setMaximumFontPixelSize
               NOTIFY configurationChanged)
    Q_PROPERTY(qreal userScalePercent
               READ userScalePercent WRITE setUserScalePercent
               NOTIFY configurationChanged)
    Q_PROPERTY(bool valid
               READ isValid
               NOTIFY sizingChanged)
    Q_PROPERTY(qreal baselinePointSize
               READ baselinePointSize
               NOTIFY sizingChanged)
    Q_PROPERTY(qreal effectivePointSize
               READ effectivePointSize
               NOTIFY sizingChanged)
    Q_PROPERTY(qreal appliedScalePercent
               READ appliedScalePercent
               NOTIFY sizingChanged)

public:
    explicit UISizingController(QObject* parent = nullptr);

    Q_INVOKABLE QVariantMap currentDisplayMetrics() const;

    QQuickWindow* window() const; 
    void setWindow(QQuickWindow* window);

    bool isEnabled() const { return _enabled; }
    void setEnabled(bool enabled);

    bool updatesSuspended() const { return _updatesSuspended; }
    void setUpdatesSuspended(bool suspended);

    QSizeF referenceViewportSize() const;
    void setReferenceViewportSize(const QSizeF& size);

    qreal referenceLogicalDpi() const { return _reference.logicalDpi; }
    void setReferenceLogicalDpi(qreal dpi);

    qreal referenceFontPointSize() const { return _reference.fontPointSize; }
    void setReferenceFontPointSize(qreal pointSize);

    qreal minimumFontPixelSize() const { return _limits.minimumPixelSize; }
    void setMinimumFontPixelSize(qreal pixelSize);

    qreal maximumFontPixelSize() const { return _limits.maximumPixelSize; }
    void setMaximumFontPixelSize(qreal pixelSize);

    qreal userScalePercent() const { return _userScalePercent; }
    void setUserScalePercent(qreal percent);

    bool isValid() const { return _result.valid; }
    qreal baselinePointSize() const { return _result.baselinePointSize; }
    qreal effectivePointSize() const { return _result.effectivePointSize; }
    qreal appliedScalePercent() const { return _result.appliedScalePercent; }

signals:
    void windowChanged();
    void configurationChanged();
    void sizingChanged();

private:
    void _setNumber(double& dest, double value);
    void _scheduleUpdate();
    void _updateSizing();
    void _publishResult(const UISizingPolicy::Result& result);

    DisplayMetricsAdapter _displayMetrics;
    QTimer _updateTimer;

    UISizingPolicy::Reference _reference;
    UISizingPolicy::FontLimits _limits;
    UISizingPolicy::Result _result;

    qreal _userScalePercent = 100.0;
    bool _enabled = false;
    bool _updatesSuspended = false;
};