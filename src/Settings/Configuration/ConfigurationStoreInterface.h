/****************************************************************************
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QString>
#include <QVariantMap>

class ConfigurationStoreInterface
{
public:
    enum class ReadResult {
        Loaded,
        NotFound,
        Error
    };

    virtual ~ConfigurationStoreInterface() = default;

    // Implementations must serialize concurrent filesystem operations internally.
    // Individual operations are protected; a read/write sequence is not a transaction.
    // These methods can block. Call them outside the GUI thread.
    // Documents contain JSON-compatible QVariant values.
    // Keys identify documents, not filesystem paths.

    // Only modifies document when Loaded is returned.
    virtual ReadResult read(
        const QString& key,
        QVariantMap& document,
        QString& error) const = 0;

    // Success means the replacement has been committed.
    virtual bool write(
        const QString& key,
        const QVariantMap& document,
        QString& error) = 0;

    // Removing a missing document succeeds.
    virtual bool remove(
        const QString& key,
        QString& error) = 0;
};
