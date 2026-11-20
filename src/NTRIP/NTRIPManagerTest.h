/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "UnitTest.h"

class NTRIPManagerTest : public UnitTest
{
    Q_OBJECT

public:
    NTRIPManagerTest() = default;

protected:
    void init() final;

private slots:
    void _rejectsBeforeInitialization();
    void _commitsOnlyAfterSuccessfulWrite();
    void _failedWritePreservesCommittedState();
    void _rejectsInvalidAndMissingProfiles();
    void _addsAndRemovesProfiles();
    void _passwordActions_data();
    void _passwordActions();
    void _shutdownWaitsForCommit();
    void _shutdownFailureCanRetry();
    void _connectionDuringPendingDeletion();
    void _unsupportedDocumentIsNotOverwritten();
};
