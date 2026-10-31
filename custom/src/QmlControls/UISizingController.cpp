#include "UISizingController.h"
#include "qstringliteral.h"

UISizingController::UISizingController(QObject* parent)
    : QObject(parent)
    , _displayMetrics(this)
    , _updateTimer(this)
{
    _updateTimer.setSingleShot(true);
    _updateTimer.setInterval(0);

    connect(
        &_displayMetrics,
        &DisplayMetricsAdapter::windowChanged,
        this,
        &UISizingController::windowChanged
    );
    connect(
        &_displayMetrics,
        &DisplayMetricsAdapter::metricsChanged,
        this,
        &UISizingController::_scheduleUpdate
    );
    connect(
        &_updateTimer,
        &QTimer::timeout,
        this,
        &UISizingController::_updateSizing
    );
}

QQuickWindow* UISizingController::window() const
{
    return _displayMetrics.window();
}

void UISizingController::setWindow(QQuickWindow *window)
{
    _displayMetrics.setWindow(window);
}

void UISizingController::setEnabled(bool enabled)
{
    if (_enabled == enabled) {
        return;
    }

    _enabled = enabled;

    emit configurationChanged();
    _scheduleUpdate();
}

void UISizingController::setUpdatesSuspended(bool suspended)
{
    if (_updatesSuspended == suspended) {
        return;
    }

    _updatesSuspended = suspended;

    emit configurationChanged();
    _scheduleUpdate();
}

QSizeF UISizingController::referenceViewportSize() const
{
    return QSizeF(
        _reference.viewportWidth,
        _reference.viewportHeight
    );
}

void UISizingController::setReferenceViewportSize(const QSizeF &size)
{
    if (_reference.viewportWidth == size.width() && _reference.viewportHeight == size.height()) {
        return;
    }

    _reference.viewportWidth = size.width();
    _reference.viewportHeight = size.height();

    emit configurationChanged();
    _scheduleUpdate();
}

void UISizingController::setReferenceLogicalDpi(qreal dpi)
{
    return UISizingController::_setNumber(_reference.logicalDpi, dpi);
}


void UISizingController::setReferenceFontPointSize(qreal pointSize)
{
    return UISizingController::_setNumber(_reference.fontPointSize, pointSize);
}

void UISizingController::setMinimumFontPixelSize(qreal pixelSize)
{
    return UISizingController::_setNumber(_limits.minimumPixelSize, pixelSize);
}

void UISizingController::setMaximumFontPixelSize(qreal pixelSize)
{
    return UISizingController::_setNumber(_limits.maximumPixelSize, pixelSize);
}

void UISizingController::setUserScalePercent(qreal percent)
{
    return UISizingController::_setNumber(_userScalePercent, percent);
}

void UISizingController::_setNumber(double& dest, double value)
{
    if (dest == value) {
        return;
    }

    dest = value;

    emit configurationChanged();
    _scheduleUpdate();
}

void UISizingController::_scheduleUpdate()
{
    if (!_enabled || !_displayMetrics.isValid()) {
        _updateTimer.stop();
        _publishResult(UISizingPolicy::Result{});
        return;
    }

    if (_updatesSuspended) {
        _updateTimer.stop();
        return;
    }

    if (!_updateTimer.isActive()) {
        _updateTimer.start();
    }
}

void UISizingController::_updateSizing()
{
    if (!_enabled || !_displayMetrics.isValid()) {
        _publishResult(UISizingPolicy::Result{});
        return;
    }

    if (_updatesSuspended) {
        return;
    }

    UISizingPolicy::Input input;
    input.viewportWidth = _displayMetrics.viewportWidth();
    input.viewportHeight = _displayMetrics.viewportHeight();
    input.logicalDpi = _displayMetrics.logicalDpi();
    input.userScalePercent = _userScalePercent;

    _publishResult(UISizingPolicy::calculate(input, _reference, _limits));
}

void UISizingController::_publishResult(const UISizingPolicy::Result& result)
{
    if (_result.valid == result.valid &&
            _result.viewportScale == result.viewportScale &&
            _result.baselinePointSize == result.baselinePointSize &&
            _result.effectivePointSize == result.effectivePointSize &&
            _result.appliedScalePercent == result.appliedScalePercent) {
        return;
    }

    _result = result;

    emit sizingChanged();
}

QVariantMap UISizingController::currentDisplayMetrics() const
{
    if (!_displayMetrics.isValid()) {
        return {};
    }

    return {
        {QStringLiteral("viewportWidth"), _displayMetrics.viewportWidth()},
        {QStringLiteral("viewportHeight"), _displayMetrics.viewportHeight()},
        {QStringLiteral("logicalDpi"), _displayMetrics.logicalDpi()},
    };
}