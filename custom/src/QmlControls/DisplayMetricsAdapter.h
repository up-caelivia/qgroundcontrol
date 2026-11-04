#pragma once

#include <QObject>
#include <QPointer>
#include <QMetaObject>
#include <QQuickWindow>
#include <QScreen>
#include <QVector>

class DisplayMetricsAdapter : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QQuickWindow* window READ window WRITE setWindow NOTIFY windowChanged)
    Q_PROPERTY(int viewportWidth READ viewportWidth NOTIFY metricsChanged)
    Q_PROPERTY(int viewportHeight READ viewportHeight NOTIFY metricsChanged)
    Q_PROPERTY(qreal logicalDpi READ logicalDpi NOTIFY metricsChanged)
    Q_PROPERTY(qreal pixelsPerMillimeter READ pixelsPerMillimeter NOTIFY metricsChanged)
    Q_PROPERTY(bool valid READ isValid NOTIFY metricsChanged)

public: 
    explicit DisplayMetricsAdapter(QObject* parent = nullptr);
    ~DisplayMetricsAdapter() override;

    QQuickWindow* window() const;
    void setWindow(QQuickWindow* window);

    int viewportWidth() const;
    int viewportHeight() const;
    qreal logicalDpi() const;
    qreal pixelsPerMillimeter() const;
    bool isValid() const;

signals:
    void windowChanged();
    void metricsChanged();

private:
    void _setScreen(QScreen* screen);
    
    static void _disconnectConnections(
        QVector<QMetaObject::Connection>& connections);

    QPointer<QQuickWindow> _window;
    QPointer<QScreen> _screen;

    QVector<QMetaObject::Connection> _windowConnections;
    QVector<QMetaObject::Connection> _screenConnections;
};