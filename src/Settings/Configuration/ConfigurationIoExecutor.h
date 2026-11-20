#pragma once

#include <QObject>
#include <QString>
#include <QThreadPool>
#include <QVariantMap>

#include <functional>

struct ConfigurationIoResult
{
    bool success = false;
    QVariantMap document;
    QString error;
};

class ConfigurationIoExecutor final : public QObject
{
public:
    using Job = std::function<ConfigurationIoResult()>;
    using Completion =
        std::function<void(const ConfigurationIoResult&)>;

    explicit ConfigurationIoExecutor(QObject* parent = nullptr);
    ~ConfigurationIoExecutor() override;

    // All public methods are called on this object's thread.
    bool submit(
        QObject* context,
        Job job,
        Completion completion);

    bool isAccepting() const { return _accepting; }

    // Reject new jobs and finish accepted jobs.
    void stopAndWait();

private:
    QThreadPool _pool;
    bool _accepting = true;
};
