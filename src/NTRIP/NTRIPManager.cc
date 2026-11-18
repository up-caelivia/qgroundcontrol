#include "NTRIPManager.h"
#include "NTRIPConfiguration.h"
#include "NTRIPConnection.h"
#include "RTCM/RTCMMavlink.h"
#include "NTRIPSettings.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "QGCToolbox.h"
#include "SettingsManager.h"
#include "qglobal.h"
#include "qnamespace.h"
#include "qobject.h"
#include "quuid.h"

QGC_LOGGING_CATEGORY(NTRIPManagerLog, "NTRIPManagerLog")

NTRIPManager::NTRIPManager(QGCApplication *app, QGCToolbox *toolbox)
    : QGCTool(app, toolbox) {}

static bool createConfiguration(
  NTRIPSettings* settings,
  NTRIPConfiguration& configuration,
  QString& error) {
  configuration.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
  configuration.name = QObject::tr("Existing NTRIP connection");
  configuration.hostAddress = settings->ntripServerHostAddress()->rawValue().toString();
  bool portOk = false;
  const int port = settings->ntripServerPort()->rawValue().toInt(&portOk);
  if (!portOk || port < 1 || port > 65535) {
    error = QObject::tr("NTRIP server port must be an integer between 1 and 65535");
    return false;
  }
  configuration.port = static_cast<quint16>(port);
  configuration.username = settings->ntripUsername()->rawValue().toString();
  configuration.password = settings->ntripPassword()->rawValue().toString();
  configuration.mountpoint = settings->ntripMountpoint()->rawValue().toString();

  const QString whitelist = settings->ntripWhitelist()->rawValue().toString().trimmed();
  if (!whitelist.isEmpty()) {
    for (const auto &entry : whitelist.split(",")) {
      bool msgOk = false;
      const int messageId = entry.trimmed().toInt(&msgOk);
      if (!msgOk || messageId < 0 || messageId > 4095) {
        error = QObject::tr("RTCM message IDs must be integers between 0 and 4095.");
        return false;
      }
      configuration.whitelist.insert(messageId);
      qCDebug(NTRIPManagerLog) << "whitelist: " << configuration.whitelist;
    }
  }
  configuration.enableVRS = settings->ntripEnableVRS()->rawValue().toBool();
  configuration.forceNtripV1 = settings->ntripForceNtripV1()->rawValue().toBool();
  
  return true;
}

bool NTRIPManager::connectConfiguration(const QString &connection_id)
{
  if (_tcpLink) {
    qgcApp()->showAppMessage(tr("Disconnect the current NTRIP connection first."));
    return false;
  }
  if (!_rtcmMavlink) {
    qgcApp()->showAppMessage(tr("NTRIP manager is not ready."));
    return false;
  }
  const auto it = _configurations.constFind(connection_id);
  if (it == _configurations.cend()) {
    qgcApp()->showAppMessage(tr("The selected NTRIP configuration does not exist."));
    return false;
  }

  NTRIPConnection* connection = new NTRIPConnection(it.value(), this);
  _tcpLink = connection;
  _activeConnectionId = connection_id;

  connect(connection, &NTRIPConnection::closed,
          this, [this, connection]() {
    if (_tcpLink == connection) {
      _tcpLink = nullptr;
      _activeConnectionId.clear();
      emit connectionIdChanged();
    }

    connection->deleteLater();
  });

  connect(connection, &NTRIPConnection::error,
          this, [this, connection](const QString& message){
      if (_tcpLink == connection) {
        tcpError(message);
      }
  });
  connect(connection, &NTRIPConnection::RTCMDataUpdate,
          this, [this, connection](const QByteArray& message) {
      if (_tcpLink == connection) {
        _rtcmMavlink->RTCMDataUpdate(message);
      }
  });
  connect(connection, &NTRIPConnection::RTCMDataUpdate,
          this, [this, connection](const QByteArray& message){
      if (_tcpLink == connection) {
        emit rtcmDataReceived(message);
      }
  });

  connect(connection, &NTRIPConnection::stateChanged,
          this, [this, connection]() {
      if (_tcpLink == connection) {
        emit connectionStateChanged();
      }
  });

  connect(connection, &NTRIPConnection::receivingDataChanged,
          this, [this, connection]() {
      if (_tcpLink == connection) {
        emit receivingDataChanged();
      }
  });

  emit connectionIdChanged();

  if (_tcpLink != connection) {
    return false;
  }

  connection->start();

  return true;
}

void NTRIPManager::disconnectConnection()
{
  if (_tcpLink) {
    _tcpLink->stop();
  }
}

NTRIPConnection::State NTRIPManager::connectionState() const {
  return _tcpLink ? _tcpLink->state() : NTRIPConnection::Disconnected;
}

bool NTRIPManager::receivingData() const {
  return _tcpLink && _tcpLink->receivingData();
}

QString NTRIPManager::connectionId() const {
  return _activeConnectionId;
}

void NTRIPManager::setToolbox(QGCToolbox *toolbox) {
  QGCTool::setToolbox(toolbox);

  _rtcmMavlink = new RTCMMavlink(*toolbox);

  NTRIPSettings *settings = qgcApp()->toolbox()->settingsManager()->ntripSettings();

  NTRIPConfiguration configuration;
  QString error;
  if (!createConfiguration(settings, configuration, error)) {
    qgcApp()->showAppMessage(error);
    return;
  }

  _configurations.insert(configuration.id, configuration);
}

void NTRIPManager::tcpError(const QString errorMsg) {
  emit errorOccurred(errorMsg);

  qgcApp()->showAppMessage(tr("NTRIP Server Error: %1").arg(errorMsg));
}