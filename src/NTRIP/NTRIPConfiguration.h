#pragma once

#include <QtGlobal>
#include <QSet>
#include <QString>

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