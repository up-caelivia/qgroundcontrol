#pragma once

#include "NTRIPConfiguration.h"

#include <QVariantMap>

namespace NTRIPConfigurationCodec {

// Caller supplies a validated profile document.
QVariantMap encode(
    const NTRIPProfileDocument& profileDocument);

// Leaves profileDocument unchanged on failure.
bool decode(
    const QVariantMap& document,
    NTRIPProfileDocument& profileDocument,
    QString& error);

} // namespace NTRIPConfigurationCodec