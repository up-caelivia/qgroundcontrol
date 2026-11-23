/****************************************************************************
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QObject>
#include "ParseNTRIP.h"

class NTRIPManager;

class NTRIPStatusController : public QObject
{
    Q_OBJECT
    
public:
    explicit NTRIPStatusController(
        NTRIPManager& manager,
        QObject* parent = nullptr
    );

private:
    ParseNTRIP _parser;
};