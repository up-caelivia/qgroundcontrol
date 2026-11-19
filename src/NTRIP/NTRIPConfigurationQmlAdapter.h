#pragma once

#include "NTRIPConfiguration.h"

#include <QVariantMap>

namespace NTRIPConfigurationQmlAdapter {

enum class PasswordAction {
    Keep,
    Replace,
    Clear
};

struct Input {
    NTRIPConfiguration values;
    PasswordAction passwordAction = PasswordAction::Keep;
};

bool fromValues(
    const QVariantMap& values,
    Input& input,
    QString& error);

QVariantMap toValues(
    const NTRIPConfiguration& configuration);

} // namespace NTRIPConfigurationQmlAdapter