#include "ConfigurationIoExecutor.h"

#include <QFutureWatcher>
#include <QPointer>
#include <QThread>
#include <QtConcurrent/QtConcurrentRun>

#include <utility>

ConfigurationIoExecutor::ConfigurationIoExecutor(QObject* parent)
    : QObject(parent)
{
    _pool.setMaxThreadCount(1);
}

ConfigurationIoExecutor::~ConfigurationIoExecutor()
{
    stopAndWait();
}

bool ConfigurationIoExecutor::submit(
    QObject* context,
    Job job,
    Completion completion)
{
    Q_ASSERT(QThread::currentThread() == thread());
    Q_ASSERT(context);
    Q_ASSERT(context->thread() == thread());

    if (!_accepting) {
        return false;
    }

    auto* watcher = new QFutureWatcher<ConfigurationIoResult>(this);
    const QPointer<QObject> receiver(context);

    connect(
        watcher,
        &QFutureWatcher<ConfigurationIoResult>::finished,
        watcher,
        [watcher, receiver, completion = std::move(completion)]() {
            ConfigurationIoResult result;

            try {
                result = watcher->result();
            } catch (...) {
                result.error = QObject::tr(
                    "An unexpected configuration-storage error occurred.");
            }

            watcher->deleteLater();

            if (receiver) {
                completion(result);
            }
        });

    watcher->setFuture(QtConcurrent::run(
        &_pool,
        [job = std::move(job)]() {
            return job();
        }));

    return true;
}

void ConfigurationIoExecutor::stopAndWait()
{
    Q_ASSERT(QThread::currentThread() == thread());

    _accepting = false;
    _pool.waitForDone();
}
