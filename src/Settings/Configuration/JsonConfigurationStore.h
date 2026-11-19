#pragma once

#include "ConfigurationStoreInterface.h"

#include <QMutex>

#include <memory>

class QLockFile;

class JsonConfigurationStore final : public ConfigurationStoreInterface
{
public:
    explicit JsonConfigurationStore(const QString& directory);
    ~JsonConfigurationStore() override;

    ReadResult read(
        const QString& key,
        QVariantMap& document,
        QString& error) const override;

    bool write(
        const QString& key,
        const QVariantMap& document,
        QString& error) override;

    bool remove(
        const QString& key,
        QString& error) override;

private:
    // Caller must hold _mutex.
    bool _preparePath(
        const QString& key,
        QString& path,
        QString& error) const;

    const QString _directory;

    mutable QMutex _mutex;
    mutable std::unique_ptr<QLockFile> _lock;
};
