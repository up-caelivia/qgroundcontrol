#include "NTRIPConfigurationCodec.h"

#include "NTRIPConfiguration.h"
#include "NTRIPConfigurationValidation.h"

#include <QMetaType>
#include <QObject>
#include <QStringList>

#include <algorithm>
#include <cmath>
#include <limits>

namespace {

bool fail(QString& error, const QString& message)
{
    error = message;
    return false;
}

QString documentString(const QString& value)
{
    return value.isNull() ? QStringLiteral("") : value;
}

bool readNumber(
    const QVariant& value,
    int minimum,
    int maximum,
    int& result)
{
    switch (value.userType()) {
    case QMetaType::Int:
    case QMetaType::UInt:
    case QMetaType::LongLong:
    case QMetaType::ULongLong:
    case QMetaType::Float:
    case QMetaType::Double:
        break;

    default:
        return false;
    }

    const double number = value.toDouble();

    if (!std::isfinite(number)
        || std::floor(number) != number
        || number < minimum
        || number > maximum) {
        return false;
    }

    result = static_cast<int>(number);
    return true;
}

} // namespace

namespace NTRIPConfigurationCodec {

QVariantMap encode(
    const NTRIPProfileDocument& profileDocument)
{
    QVariantList profiles;
    profiles.reserve(profileDocument.profiles.size());

    for (const NTRIPConfiguration& configuration : profileDocument.profiles) {
        QList<int> ids = configuration.whitelist.values();
        std::sort(ids.begin(), ids.end());

        QVariantList whitelist;
        whitelist.reserve(ids.size());

        for (int id : ids) {
            whitelist.append(id);
        }

        profiles.append(QVariantMap {
            {QStringLiteral("id"), documentString(configuration.id)},
            {QStringLiteral("name"), documentString(configuration.name)},
            {
                QStringLiteral("hostAddress"),
                documentString(configuration.hostAddress)
            },
            {QStringLiteral("port"), static_cast<int>(configuration.port)},
            {
                QStringLiteral("username"),
                documentString(configuration.username)
            },
            {
                QStringLiteral("password"),
                documentString(configuration.password)
            },
            {
                QStringLiteral("mountpoint"),
                documentString(configuration.mountpoint)
            },
            {QStringLiteral("whitelist"), whitelist},
            {QStringLiteral("enableVRS"), configuration.enableVRS},
            {QStringLiteral("forceNtripV1"), configuration.forceNtripV1}
        });
    }

    QVariantMap document {
        {QStringLiteral("fileType"), QStringLiteral("NTRIPProfiles")},
        {QStringLiteral("version"), 1},
        {QStringLiteral("profiles"), profiles},
    };
    if (!profileDocument.startupProfileId.isEmpty()) {
        document.insert(QStringLiteral("startupProfileId"), profileDocument.startupProfileId);
    }

    return document;
}

bool decode(
    const QVariantMap& document,
    NTRIPProfileDocument& profileDocument,
    QString& error)
{
    error.clear();

    const QVariant fileType =
        document.value(QStringLiteral("fileType"));

    int version = 0;

    if (fileType.userType() != QMetaType::QString
        || fileType.toString() != QStringLiteral("NTRIPProfiles")
        || !readNumber(
            document.value(QStringLiteral("version")), 1, 1, version)) {
        return fail(
            error,
            QObject::tr(
                "The NTRIP configuration format or version is unsupported."));
    }

    const QVariant profilesValue =
        document.value(QStringLiteral("profiles"));

    if (profilesValue.userType() != QMetaType::QVariantList) {
        return fail(error, QObject::tr("The NTRIP profiles list is invalid."));
    }

    const QVariantList profiles = profilesValue.toList();

    if (profiles.size() > NTRIPConfigurationValidation::MaxProfiles) {
        return fail(
            error,
            QObject::tr("The NTRIP document contains too many profiles."));
    }

    NTRIPProfileDocument candidate;

    const QStringList stringFields = {
        QStringLiteral("id"),
        QStringLiteral("name"),
        QStringLiteral("hostAddress"),
        QStringLiteral("username"),
        QStringLiteral("password"),
        QStringLiteral("mountpoint")
    };

    for (const QVariant& entry : profiles) {
        if (entry.userType() != QMetaType::QVariantMap) {
            return fail(error, QObject::tr("An NTRIP profile is invalid."));
        }

        const QVariantMap profile = entry.toMap();

        for (const QString& field : stringFields) {
            if (profile.value(field).userType() != QMetaType::QString) {
                return fail(
                    error,
                    QObject::tr("Invalid stored NTRIP field: %1.").arg(field));
            }
        }

        if (profile.value(QStringLiteral("enableVRS")).userType()
                != QMetaType::Bool
            || profile.value(QStringLiteral("forceNtripV1")).userType()
                != QMetaType::Bool) {
            return fail(
                error,
                QObject::tr("The stored NTRIP options are invalid."));
        }

        int port = 0;

        if (!readNumber(
                profile.value(QStringLiteral("port")),
                0,
                std::numeric_limits<quint16>::max(),
                port)) {
            return fail(error, QObject::tr("The stored NTRIP port is invalid."));
        }

        const QVariant whitelistValue =
            profile.value(QStringLiteral("whitelist"));

        if (whitelistValue.userType() != QMetaType::QVariantList) {
            return fail(
                error,
                QObject::tr("The stored RTCM whitelist must be an array."));
        }

        const QVariantList whitelist = whitelistValue.toList();

        if (whitelist.size()
            > NTRIPConfigurationValidation::MaxWhitelistEntries) {
            return fail(
                error,
                QObject::tr("The stored RTCM whitelist is too large."));
        }

        NTRIPConfiguration configuration;

        configuration.id = profile.value(QStringLiteral("id")).toString();
        configuration.name = profile.value(QStringLiteral("name")).toString();
        configuration.hostAddress =
            profile.value(QStringLiteral("hostAddress")).toString();
        configuration.port = static_cast<quint16>(port);
        configuration.username =
            profile.value(QStringLiteral("username")).toString();
        configuration.password =
            profile.value(QStringLiteral("password")).toString();
        configuration.mountpoint =
            profile.value(QStringLiteral("mountpoint")).toString();
        configuration.enableVRS =
            profile.value(QStringLiteral("enableVRS")).toBool();
        configuration.forceNtripV1 =
            profile.value(QStringLiteral("forceNtripV1")).toBool();

        for (const QVariant& value : whitelist) {
            int id = 0;

            if (!readNumber(
                    value,
                    std::numeric_limits<int>::min(),
                    std::numeric_limits<int>::max(),
                    id)) {
                return fail(
                    error,
                    QObject::tr("A stored RTCM message ID is not an integer."));
            }

            configuration.whitelist.insert(id);
        }

        if (candidate.profiles.contains(configuration.id)) {
            return fail(
                error,
                QObject::tr("The NTRIP document contains duplicated profile IDs."));
        }

        candidate.profiles.insert(configuration.id, configuration);
    }

    if (!NTRIPConfigurationValidation::validate(candidate.profiles, error)) {
        return false;
    }

    const QVariant startupValue = document.value(QStringLiteral("startupProfileId"));
    const QString startupId = startupValue.userType() == QMetaType::QString
        ? startupValue.toString() : QString{};
    if (candidate.profiles.contains(startupId)) {
        candidate.startupProfileId = startupId;
    }

    profileDocument = candidate;
    return true;
}

} // namespace NTRIPConfigurationCodec