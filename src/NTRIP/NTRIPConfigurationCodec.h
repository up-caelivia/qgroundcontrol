/****************************************************************************
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

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