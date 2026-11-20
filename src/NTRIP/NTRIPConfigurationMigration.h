#pragma once

#include "NTRIPConfiguration.h"

namespace NTRIPConfigurationMigration {

// Reads legacy settings without changing them.
// Returns an empty collection when no previous values were saved.
bool readLegacy(
    NTRIPConfigurations& configurations,
    QString& error);

} // namespace NTRIPConfigurationMigration