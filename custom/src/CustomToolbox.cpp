#include "CustomToolbox.h"
#include "QGCApplication.h"
#include "QGCToolbox.h"
#include "NTRIP.h"
#include "SettingsManager.h"
#include "NTRIPSettings.h"
#include "ParseNTRIP.h"

#include <QTimer>
#include <QDebug>

CustomToolbox::CustomToolbox(QGCApplication* app)
    : QGCToolbox(app)
{
    _parseNTRIP = new ParseNTRIP();
    qDebug() << "[CustomToolbox] Created!";

    QTimer* linkSearchTimer = new QTimer(this);
    linkSearchTimer->setInterval(1000); // ogni secondo
    connect(linkSearchTimer, &QTimer::timeout, this, [this, linkSearchTimer]() {

        auto ntrip = qgcApp()->toolbox()->ntrip();

        if (!ntrip) {
            return;
        }


        if (ntrip->_tcpLink) {

            connect(ntrip->_tcpLink, &NTRIPTCPLink::error, this, [this](const QString &info) {
                if (info.contains("bad mountpoint")) 
                    _parseNTRIP->setmountError(true);
                _parseNTRIP->setauthError(true);
                _parseNTRIP->setNtripReceiving(false);
            });

            connect(ntrip->_tcpLink, &NTRIPTCPLink::RTCMDataUpdate, this, [this](const QByteArray &message) {
                _parseNTRIP->setNtripEnabled(true);
                _parseNTRIP->setNtripReceiving(true);
                _parseNTRIP->setauthError(false);
                _parseNTRIP->setmountError(false);
                _parseNTRIP->handleRTCM(message);
            });
            linkSearchTimer->stop();

        } else {
            qDebug() << "[CustomToolbox] Waiting for _tcpLink to be created...";
        }
    });

    linkSearchTimer->start();
}
