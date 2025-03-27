#pragma once

#include "QGCToolbox.h"
#include "ParseNTRIP.h"
#include "constants.h"

class CustomToolbox : public QGCToolbox {
    Q_OBJECT
public:
    CustomToolbox(QGCApplication* app);

private:
    ParseNTRIP* _parseNTRIP = nullptr;
   //Constants* constants;
};
