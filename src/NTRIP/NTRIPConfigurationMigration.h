/****************************************************************************
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "NTRIPConfiguration.h"

namespace NTRIPConfigurationMigration {

// Reads legacy settings without changing them.
// Returns an empty profile document when no previous values were saved.
bool readLegacy(
    NTRIPProfileDocument& profileDocument,
    QString& error);

} // namespace NTRIPConfigurationMigration