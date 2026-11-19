#pragma once

#include <QSet>
#include <QString>
#include <QtGlobal>

struct NTRIPConfiguration
{
    QString id;
    QString name;
    QString hostAddress;
    quint16 port = 0;

    QString username;
    QString password;
    QString mountpoint;

    QSet<int> whitelist;

    bool enableVRS = true;
    bool forceNtripV1 = false;
};