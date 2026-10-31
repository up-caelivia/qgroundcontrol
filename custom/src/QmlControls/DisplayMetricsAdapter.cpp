#include "DisplayMetricsAdapter.h"
#include "qglobal.h"
#include "qobject.h"
#include "qobjectdefs.h"
#include "qquickwindow.h"
#include "qscreen.h"
#include "qwindow.h"

#include <cmath>

namespace {
    constexpr qreal millimetersPerInch = 25.4;

    qreal positiveOrZero(qreal value)
    {
        return std::isfinite(value) && value > 0.0 ? value : 0.0;
    }
}

DisplayMetricsAdapter::DisplayMetricsAdapter(QObject* parent)
    : QObject(parent)
{
}


DisplayMetricsAdapter::~DisplayMetricsAdapter() 
{
    _disconnectConnections(_windowConnections); 
    _disconnectConnections(_screenConnections); 
}

QQuickWindow* DisplayMetricsAdapter::window() const
{
    return _window.data();
}

void DisplayMetricsAdapter::setWindow(QQuickWindow *window)
{
    if (_window.data() == window) {
        return;
    }
    
    _disconnectConnections(_windowConnections);
    _window = window;

    if (window) {
        _windowConnections.append(connect(
            window,
            &QWindow::widthChanged,
            this,
            &DisplayMetricsAdapter::metricsChanged
        ));
        _windowConnections.append(connect(
            window,
            &QWindow::heightChanged,
            this,
            &DisplayMetricsAdapter::metricsChanged
        ));
        _windowConnections.append(connect(
            window,
            &QWindow::screenChanged,
            this,
            [this](QScreen* screen) {
                _setScreen(screen);
                emit metricsChanged();
            }
        ));
        _windowConnections.append(connect(
            window,
            &QObject::destroyed,
            this,
            [this]() {
                _disconnectConnections(_windowConnections);
                _window.clear();
                _setScreen(nullptr);

                emit windowChanged();
                emit metricsChanged();
            }
        ));
    }

    _setScreen(window ? window->screen() : nullptr);

    emit windowChanged();
    emit metricsChanged();
}

int DisplayMetricsAdapter::viewportWidth() const
{
    return _window ? _window->width() : 0;
}

int DisplayMetricsAdapter::viewportHeight() const
{
    return _window ? _window->height() : 0;
}

qreal DisplayMetricsAdapter::logicalDpi() const
{
    if (!_screen) {
        return 0.0;
    }
    return positiveOrZero(_screen->logicalDotsPerInchY());
}

qreal DisplayMetricsAdapter::pixelsPerMillimeter() const
{
    if (!_screen) {
        return 0.0;
    }

    return positiveOrZero(
        _screen->physicalDotsPerInch() / millimetersPerInch);
}

bool DisplayMetricsAdapter::isValid() const {
    return _window &&
            _screen &&
            viewportWidth() > 0 &&
            viewportHeight() > 0 &&
            logicalDpi() > 0.0;
}

void DisplayMetricsAdapter::_setScreen(QScreen* screen) {
    if (_screen.data() == screen) {
        return;
    }

    _disconnectConnections(_screenConnections);
    _screen = screen;

    if (!screen) {
        return;
    }

    _screenConnections.append(connect(
        screen,
        &QScreen::logicalDotsPerInchChanged,
        this,
        &DisplayMetricsAdapter::metricsChanged
    ));
    _screenConnections.append(connect(
        screen,
        &QScreen::physicalDotsPerInchChanged,
        this,
        &DisplayMetricsAdapter::metricsChanged
    ));
    _screenConnections.append(connect(
        screen,
        &QScreen::geometryChanged,
        this,
        &DisplayMetricsAdapter::metricsChanged
    ));
    _screenConnections.append(connect(
        screen,
        &QObject::destroyed,
        this,
        [this]() {
            _disconnectConnections(_screenConnections);
            _screen.clear();

            emit metricsChanged();
        }
    ));
}

void DisplayMetricsAdapter::_disconnectConnections(QVector<QMetaObject::Connection>& connections) {
    for (const QMetaObject::Connection& connection : connections) {
        QObject::disconnect(connection);
    }

    connections.clear();
}