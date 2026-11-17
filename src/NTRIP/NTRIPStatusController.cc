#include "NTRIPStatusController.h"
#include "NTRIPManager.h"

NTRIPStatusController::NTRIPStatusController(
    NTRIPManager& manager,
    QObject* parent)
    :  QObject(parent)
{
    connect(&manager, &NTRIPManager::rtcmDataReceived, 
            this, [this](const QByteArray& message) {
        _parser.setNtripEnabled(true);
        _parser.setauthError(false);
        _parser.setmountError(false);
        _parser.handleRTCM(message);
    });

    connect(&manager, &NTRIPManager::errorOccurred,
        this, [this](const QString&) {
            _parser.setNtripReceiving(false);
    });
}