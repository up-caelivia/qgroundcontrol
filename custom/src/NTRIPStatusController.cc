/****************************************************************************
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "NTRIPStatusController.h"
#include "NTRIPManager.h"

NTRIPStatusController::NTRIPStatusController(
    NTRIPManager& manager,
    QObject* parent)
    : QObject(parent)
{
    const auto updateSession = [this, &manager]() {
        const bool enabled = !manager.connectionId().isEmpty();
        _parser.setNtripEnabled(enabled);

        if (!enabled) {
            _parser.setNtripReceiving(false);
        } else {
            _parser.setauthError(false);
            _parser.setmountError(false);
        }
    };

    connect(&manager, &NTRIPManager::connectionIdChanged, this, updateSession);

    connect(&manager, &NTRIPManager::receivingDataChanged,
            this, [this, &manager]() {
        _parser.setNtripReceiving(manager.receivingData());
    });

    connect(&manager, &NTRIPManager::rtcmDataReceived,
            this, [this](const QByteArray& message) {
        if (message.size() >= 6) {
            _parser.handleRTCM(message);
        }
    });

    updateSession();
    _parser.setNtripReceiving(manager.receivingData());
}
