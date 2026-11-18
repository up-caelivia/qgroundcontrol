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
        QString& error
    ) const override;

    bool load(
        const QString& key,
        QVariantMap& document,
        QString& error
    ) override;

    bool remove(
        const QString& key,
        QString& error
    ) override;

private:
    bool _preparePath(
        const QString& key,
        QString& path,
        QString& error) const;

    const QString _directory;

    mutable QMutex _mutex;
    mutable std::unique_ptr<QLockFile> _lock;
};