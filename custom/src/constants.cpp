#include "constants.h"

QObject* Constants::constants_singleton_provider(QQmlEngine* engine, QJSEngine* scriptEngine) {
    Q_UNUSED(engine)
    Q_UNUSED(scriptEngine)

    static Constants* constants = nullptr;

    if (constants == nullptr)
        constants = new Constants();

    return constants;
}
