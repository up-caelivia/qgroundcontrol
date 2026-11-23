/****************************************************************************
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "NTRIPConfigurationValidation.h"

#include <QHostAddress>
#include <QObject>
#include <QUuid>

namespace {

bool fail(QString& error, const QString& message)
{
    error = message;
    return false;
}

} // namespace

namespace NTRIPConfigurationValidation {

bool validate(
    const NTRIPConfiguration& configuration,
    QString& error)
{
    error.clear();

    const QUuid uuid(configuration.id);

    if (uuid.isNull()
        || uuid.toString(QUuid::WithoutBraces) != configuration.id) {
        return fail(error, QObject::tr("The NTRIP profile ID is invalid."));
    }

    if (configuration.name.size() > 128
        || configuration.name.trimmed().isEmpty()) {
        return fail(
            error,
            QObject::tr(
                "The configuration name must contain 1 to 128 characters."));
    }

    if (configuration.hostAddress.isEmpty()
        || configuration.hostAddress.size() > 253) {
        return fail(
            error,
            QObject::tr("Enter a valid NTRIP host address."));
    }

    for (QChar character : configuration.hostAddress) {
        if (!character.isPrint()
            || character.isSpace()
            || QStringLiteral("/\\?#@[]").contains(character)) {
            return fail(
                error,
                QObject::tr(
                    "Enter a hostname or IP address without a URL "
                    "scheme or path. Enter IPv6 addresses without brackets."));
        }
    }

    if (configuration.hostAddress.contains(':')) {
        QHostAddress address;

        if (!address.setAddress(configuration.hostAddress)) {
            return fail(
                error,
                QObject::tr("The IPv6 address is invalid."));
        }
    }

    if (configuration.port == 0) {
        return fail(
            error,
            QObject::tr(
                "The NTRIP port must be between 1 and 65535."));
    }

    if (configuration.username.size() > 512) {
        return fail(
            error,
            QObject::tr("The NTRIP username is too long."));
    }

    if (!configuration.mountpoint.isEmpty()
        && configuration.username.contains(':')) {
        return fail(
            error,
            QObject::tr(
                "An NTRIP authentication username cannot contain a colon."));
    }

    if (configuration.password.size() > 4096) {
        return fail(
            error,
            QObject::tr("The NTRIP password is too long."));
    }

    if (configuration.mountpoint.size() > 1024
        || configuration.mountpoint.startsWith('/')) {
        return fail(
            error,
            QObject::tr(
                "Enter a mountpoint without a leading slash, "
                "or leave it empty for RTCM over TCP."));
    }

    for (QChar character : configuration.mountpoint) {
        const ushort code = character.unicode();

        if (code <= 0x20 || code >= 0x7f
            || character == '?' || character == '#') {
            return fail(
                error,
                QObject::tr(
                    "The mountpoint must contain printable ASCII "
                    "characters without spaces, '?' or '#'."));
        }
    }

    if (configuration.whitelist.size() > MaxWhitelistEntries) {
        return fail(
            error,
            QObject::tr("The RTCM whitelist contains too many entries."));
    }

    for (int id : configuration.whitelist) {
        if (id < 0 || id > 4095) {
            return fail(
                error,
                QObject::tr(
                    "RTCM message IDs must be between 0 and 4095."));
        }
    }

    return true;
}

bool validate(
    const NTRIPConfigurations& configurations,
    QString& error)
{
    error.clear();

    if (configurations.size() > MaxProfiles) {
        return fail(
            error,
            QObject::tr(
                "The maximum number of NTRIP configurations has been reached."));
    }

    for (auto it = configurations.cbegin();
         it != configurations.cend(); ++it) {
        if (it.key() != it.value().id) {
            return fail(
                error,
                QObject::tr("The NTRIP profile identity is inconsistent."));
        }

        if (!validate(it.value(), error)) {
            return false;
        }
    }

    return true;
}

} // namespace NTRIPConfigurationValidation