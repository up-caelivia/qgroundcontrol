#pragma once

#include "NTRIPConfiguration.h"

#include <QVariantMap>

namespace NTRIPConfigurationCodec {

// Caller supplies a validated collection.
QVariantMap encode(
    const NTRIPConfigurations& configurations);

// Leaves configurations unchanged on failure.
bool decode(
    const QVariantMap& document,
    NTRIPConfigurations& configurations,
    QString& error);

} // namespace NTRIPConfigurationCodec