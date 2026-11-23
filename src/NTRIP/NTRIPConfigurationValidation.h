/****************************************************************************
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "NTRIPConfiguration.h"

namespace NTRIPConfigurationValidation {

constexpr int MaxProfiles = 64;
constexpr int MaxWhitelistEntries = 4096;

bool validate(
    const NTRIPConfiguration& configuration,
    QString& error);

bool validate(
    const NTRIPConfigurations& configurations,
    QString& error);

} // namespace NTRIPConfigurationValidation