/****************************************************************************
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "NTRIPConfigurationQmlAdapter.h"

#include "NTRIPConfigurationValidation.h"

#include <QMetaType>
#include <QObject>
#include <QStringList>

#include <algorithm>
#include <limits>

namespace {

constexpr int MaxInputTextLength = 32768;

bool fail(QString& error, const QString& message)
{
    error = message;
    return false;
}

} // namespace

namespace NTRIPConfigurationQmlAdapter {

bool fromValues(
    const QVariantMap& values,
    Input& input,
    QString& error)
{
    error.clear();

    const QStringList textFields = {
        QStringLiteral("name"),
        QStringLiteral("hostAddress"),
        QStringLiteral("port"),
        QStringLiteral("username"),
        QStringLiteral("mountpoint"),
        QStringLiteral("whitelist")
    };

    for (const QString& field : textFields) {
        const QVariant value = values.value(field);

        if (value.userType() != QMetaType::QString) {
            return fail(
                error,
                QObject::tr("Invalid NTRIP input field: %1.").arg(field));
        }

        if (value.toString().size() > MaxInputTextLength) {
            return fail(
                error,
                QObject::tr("NTRIP input field is too long: %1.").arg(field));
        }
    }

    if (values.value(QStringLiteral("enableVRS")).userType()
            != QMetaType::Bool
        || values.value(QStringLiteral("forceNtripV1")).userType()
            != QMetaType::Bool) {
        return fail(
            error,
            QObject::tr("The NTRIP options must be boolean values."));
    }

    Input candidate;

    candidate.values.name =
        values.value(QStringLiteral("name")).toString().trimmed();

    candidate.values.hostAddress =
        values.value(QStringLiteral("hostAddress")).toString().trimmed();

    candidate.values.username =
        values.value(QStringLiteral("username")).toString();

    candidate.values.mountpoint =
        values.value(QStringLiteral("mountpoint")).toString();

    candidate.values.enableVRS =
        values.value(QStringLiteral("enableVRS")).toBool();

    candidate.values.forceNtripV1 =
        values.value(QStringLiteral("forceNtripV1")).toBool();

    const QString portText =
        values.value(QStringLiteral("port")).toString().trimmed();

    bool portOk = false;
    const uint port = portText.toUInt(&portOk, 10);

    if (!portOk || port > std::numeric_limits<quint16>::max()) {
        return fail(
            error,
            QObject::tr("Enter an integer NTRIP port."));
    }

    candidate.values.port = static_cast<quint16>(port);

    const QString whitelist =
        values.value(QStringLiteral("whitelist")).toString().trimmed();

    if (!whitelist.isEmpty()) {
        const QStringList entries = whitelist.split(',');

        if (entries.size()
            > NTRIPConfigurationValidation::MaxWhitelistEntries) {
            return fail(
                error,
                QObject::tr("The RTCM whitelist contains too many entries."));
        }

        candidate.values.whitelist.reserve(entries.size());

        for (const QString& entry : entries) {
            bool ok = false;
            const int id = entry.trimmed().toInt(&ok, 10);

            if (!ok) {
                return fail(
                    error,
                    QObject::tr(
                        "The RTCM whitelist must contain comma-separated integers."));
            }

            candidate.values.whitelist.insert(id);
        }
    }

    const QVariant actionValue = values.value(
        QStringLiteral("passwordAction"),
        QStringLiteral("keep"));

    if (actionValue.userType() != QMetaType::QString) {
        return fail(error, QObject::tr("Invalid password operation."));
    }

    QString password;

    if (values.contains(QStringLiteral("password"))) {
        const QVariant passwordValue =
            values.value(QStringLiteral("password"));

        if (passwordValue.userType() != QMetaType::QString
            || passwordValue.toString().size() > MaxInputTextLength) {
            return fail(error, QObject::tr("Invalid password input."));
        }

        password = passwordValue.toString();
    }

    const QString action = actionValue.toString();

    if (action == QStringLiteral("keep")) {
        if (!password.isEmpty()) {
            return fail(
                error,
                QObject::tr(
                    "Select password replacement to save a new password."));
        }

        candidate.passwordAction = PasswordAction::Keep;
    } else if (action == QStringLiteral("replace")) {
        if (password.isEmpty()) {
            return fail(
                error,
                QObject::tr(
                    "Enter a replacement password, "
                    "or explicitly select password removal."));
        }

        candidate.passwordAction = PasswordAction::Replace;
        candidate.values.password = password;
    } else if (action == QStringLiteral("clear")) {
        if (!password.isEmpty()) {
            return fail(
                error,
                QObject::tr(
                    "Password replacement and removal cannot be requested together."));
        }

        candidate.passwordAction = PasswordAction::Clear;
    } else {
        return fail(error, QObject::tr("Invalid password operation."));
    }

    input = candidate;
    return true;
}

QVariantMap toValues(
    const NTRIPConfiguration& configuration)
{
    QList<int> ids = configuration.whitelist.values();
    std::sort(ids.begin(), ids.end());

    QStringList whitelist;

    for (int id : ids) {
        whitelist.append(QString::number(id));
    }

    return {
        {QStringLiteral("id"), configuration.id},
        {QStringLiteral("name"), configuration.name},
        {QStringLiteral("hostAddress"), configuration.hostAddress},
        {QStringLiteral("port"), QString::number(configuration.port)},
        {QStringLiteral("username"), configuration.username},
        {QStringLiteral("mountpoint"), configuration.mountpoint},
        {QStringLiteral("whitelist"), whitelist.join(',')},
        {QStringLiteral("enableVRS"), configuration.enableVRS},
        {QStringLiteral("forceNtripV1"), configuration.forceNtripV1},
        {QStringLiteral("hasPassword"), !configuration.password.isEmpty()},
        {QStringLiteral("passwordAction"), QStringLiteral("keep")}
    };
}

} // namespace NTRIPConfigurationQmlAdapter