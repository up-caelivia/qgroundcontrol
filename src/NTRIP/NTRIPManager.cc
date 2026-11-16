#include "NTRIPManager.h"
#include "NTRIPConnection.h"
#include "RTCM/RTCMMavlink.h"
#include "NTRIPSettings.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "QGCToolbox.h"
#include "SettingsManager.h"

QGC_LOGGING_CATEGORY(NTRIPManagerLog, "NTRIPManagerLog")

NTRIPManager::NTRIPManager(QGCApplication *app, QGCToolbox *toolbox)
    : QGCTool(app, toolbox) {}

void NTRIPManager::setToolbox(QGCToolbox *toolbox) {
  QGCTool::setToolbox(toolbox);

  NTRIPSettings *settings =
      qgcApp()->toolbox()->settingsManager()->ntripSettings();
  if (settings->ntripServerConnectEnabled()->rawValue().toBool()) {
    qCDebug(NTRIPManagerLog) << settings->ntripEnableVRS()->rawValue().toBool();
    _rtcmMavlink = new RTCMMavlink(*toolbox);

    _tcpLink = new NTRIPConnection(
        settings->ntripServerHostAddress()->rawValue().toString(),
        settings->ntripServerPort()->rawValue().toInt(),
        settings->ntripUsername()->rawValue().toString(),
        settings->ntripPassword()->rawValue().toString(),
        settings->ntripMountpoint()->rawValue().toString(),
        settings->ntripWhitelist()->rawValue().toString(),
        settings->ntripEnableVRS()->rawValue().toBool());
    connect(_tcpLink, &NTRIPConnection::error, this, &NTRIPManager::_tcpError,
            Qt::QueuedConnection);
    connect(_tcpLink, &NTRIPConnection::RTCMDataUpdate, _rtcmMavlink,
            &RTCMMavlink::RTCMDataUpdate);
  } else {
    qCDebug(NTRIPManagerLog) << "NTRIP Server is not enabled";
  }
}

void NTRIPManager::_tcpError(const QString errorMsg) {
  qgcApp()->showAppMessage(tr("NTRIP Server Error: %1").arg(errorMsg));
}