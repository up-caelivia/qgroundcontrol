#include "CustomAnnouncer.h"
#include "QGCApplication.h"
#include "AudioOutput.h"
#include <QString>
#include <QObject>

void CustomAnnouncer::announceAltitude()
{
    QString msg = QString( "Warning: above 120 meters");
    qgcApp()->toolbox()->audioOutput()->say(msg);
}