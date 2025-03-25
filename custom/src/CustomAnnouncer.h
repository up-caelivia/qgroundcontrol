#pragma once

#include <QObject>

class CustomAnnouncer : public QObject
{
    Q_OBJECT
public:
    explicit CustomAnnouncer(QObject* parent = nullptr) : QObject(parent) {}

    Q_INVOKABLE void announceAltitude();
};