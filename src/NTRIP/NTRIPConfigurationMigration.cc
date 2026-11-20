#include "NTRIPConfigurationMigration.h"

#include "NTRIPConfigurationValidation.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaType>
#include <QObject>
#include <QSettings>
#include <QStringList>
#include <QUuid>

#include <limits>

namespace {

bool fail(QString& error, const QString& message)
{
    error = message;
    return false;
}

bool readBoolean(const QVariant& value, bool& result)
{
    if (value.userType() == QMetaType::Bool) {
        result = value.toBool();
        return true;
    }

    if (value.userType() != QMetaType::QString) {
        return false;
    }

    const QString text = value.toString().trimmed();

    if (text.compare(QStringLiteral("true"), Qt::CaseInsensitive) == 0
        || text == QStringLiteral("1")) {
        result = true;
        return true;
    }

    if (text.compare(QStringLiteral("false"), Qt::CaseInsensitive) == 0
        || text == QStringLiteral("0")) {
        result = false;
        return true;
    }

    return false;
}

} // namespace

namespace NTRIPConfigurationMigration {

bool readLegacy(
    NTRIPConfigurations& configurations,
    QString& error)
{
    error.clear();

    const QStringList keys = {
        QStringLiteral("ntripServerHostAddress"),
        QStringLiteral("ntripServerPort"),
        QStringLiteral("ntripUsername"),
        QStringLiteral("ntripPassword"),
        QStringLiteral("ntripMountpoint"),
        QStringLiteral("ntripWhitelist"),
        QStringLiteral("ntripEnableVRS"),
        QStringLiteral("ntripForceNtripV1")
    };

    QSettings settings;
    settings.beginGroup(QStringLiteral("NTRIP"));

    bool found =
        settings.contains(QStringLiteral("ntripServerConnectEnabled"));

    for (const QString& key : keys) {
        found |= settings.contains(key);
    }

    if (settings.status() != QSettings::NoError) {
        return fail(error, QObject::tr("Cannot read previous NTRIP settings."));
    }

    if (!found) {
        configurations.clear();
        return true;
    }

    QFile metadataFile(
        QStringLiteral(":/json/NTRIP.SettingsGroup.json"));

    if (!metadataFile.open(QIODevice::ReadOnly)) {
        return fail(error, QObject::tr("Cannot read previous NTRIP defaults."));
    }

    const QByteArray bytes = metadataFile.readAll();

    if (metadataFile.error() != QFileDevice::NoError) {
        return fail(error, QObject::tr("Cannot read previous NTRIP defaults."));
    }

    QJsonParseError parseError;
    const QJsonDocument metadata =
        QJsonDocument::fromJson(bytes, &parseError);

    if (parseError.error != QJsonParseError::NoError
        || !metadata.isObject()) {
        return fail(error, QObject::tr("Previous NTRIP defaults are invalid."));
    }

    const QJsonValue factsValue =
        metadata.object().value(QStringLiteral("QGC.MetaData.Facts"));

    if (!factsValue.isArray()) {
        return fail(error, QObject::tr("Previous NTRIP defaults are invalid."));
    }

    QVariantMap defaults;

    for (const QJsonValue& value : factsValue.toArray()) {
        const QJsonObject fact = value.toObject();

        if (fact.contains(QStringLiteral("default"))) {
            defaults.insert(
                fact.value(QStringLiteral("name")).toString(),
                fact.value(QStringLiteral("default")).toVariant());
        }
    }

    QVariantMap previous;

    for (const QString& key : keys) {
        if (!defaults.contains(key)) {
            return fail(error, QObject::tr("A previous NTRIP default is missing."));
        }

        previous.insert(key, settings.value(key, defaults.value(key)));
    }

    if (settings.status() != QSettings::NoError) {
        return fail(error, QObject::tr("Cannot read previous NTRIP settings."));
    }

    NTRIPConfiguration configuration;

    configuration.id =
        QUuid::createUuid().toString(QUuid::WithoutBraces);
    configuration.name = QObject::tr("Imported NTRIP connection");
    configuration.hostAddress =
        previous.value(QStringLiteral("ntripServerHostAddress"))
            .toString().trimmed();
    configuration.username =
        previous.value(QStringLiteral("ntripUsername")).toString();
    configuration.password =
        previous.value(QStringLiteral("ntripPassword")).toString();
    configuration.mountpoint =
        previous.value(QStringLiteral("ntripMountpoint")).toString();

    bool portOk = false;
    const uint port =
        previous.value(QStringLiteral("ntripServerPort"))
            .toString().trimmed().toUInt(&portOk, 10);

    if (!portOk || port > std::numeric_limits<quint16>::max()) {
        return fail(error, QObject::tr("The previous NTRIP port is invalid."));
    }

    configuration.port = static_cast<quint16>(port);

    if (!readBoolean(
            previous.value(QStringLiteral("ntripEnableVRS")),
            configuration.enableVRS)
        || !readBoolean(
            previous.value(QStringLiteral("ntripForceNtripV1")),
            configuration.forceNtripV1)) {
        return fail(error, QObject::tr("A previous NTRIP option is invalid."));
    }

    const QString whitelist =
        previous.value(QStringLiteral("ntripWhitelist")).toString().trimmed();

    if (whitelist.size() > 32768) {
        return fail(error, QObject::tr("The previous RTCM whitelist is too large."));
    }

    if (!whitelist.isEmpty()) {
        const QStringList entries = whitelist.split(',');

        if (entries.size()
            > NTRIPConfigurationValidation::MaxWhitelistEntries) {
            return fail(
                error,
                QObject::tr("The previous RTCM whitelist is too large."));
        }

        for (const QString& entry : entries) {
            bool ok = false;
            const int id = entry.trimmed().toInt(&ok, 10);

            if (!ok) {
                return fail(
                    error,
                    QObject::tr("A previous RTCM message ID is invalid."));
            }

            configuration.whitelist.insert(id);
        }
    }

    NTRIPConfigurations candidate;
    candidate.insert(configuration.id, configuration);

    if (!NTRIPConfigurationValidation::validate(candidate, error)) {
        return false;
    }

    configurations = candidate;
    return true;
}

} // namespace NTRIPConfigurationMigration
