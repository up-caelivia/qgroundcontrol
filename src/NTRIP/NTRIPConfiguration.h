/****************************************************************************
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QMap>
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

using NTRIPConfigurations = QMap<QString, NTRIPConfiguration>;

struct NTRIPProfileDocument
{
    NTRIPConfigurations profiles;
    QString startupProfileId;
};