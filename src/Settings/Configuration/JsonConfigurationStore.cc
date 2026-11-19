#include "JsonConfigurationStore.h"
#include "Configuration/ConfigurationStoreInterface.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLockFile>
#include <QMetaType>
#include <QMutexLocker>
#include <QObject>
#include <QRegularExpression>
#include <QSaveFile>

#include <cmath>
#include <utility>

namespace {

constexpr qint64 MaxDocumentBytes = 1024 * 1024;
constexpr int MaxDocumentDepth = 16;

bool isJsonCompatible(const QVariant& value, int depth = 0)
{
    if (depth > MaxDocumentDepth) {
        return false;
    }

    if (!value.isValid()) {
        return true;
    }

    switch (value.userType()) {
        case QMetaType::Nullptr:
        case QMetaType::Bool:
        case QMetaType::QString:
        case QMetaType::QStringList:
        case QMetaType::Int:
        case QMetaType::UInt:
            return true;
        
        case QMetaType::LongLong: {
            const qlonglong number = value.toLongLong();

            return number >= -9007199254740991LL
                && number <= 9007199254740991LL;
        }

        case QMetaType::Float:
        case QMetaType::Double:
            return std::isfinite(value.toDouble());

        case QMetaType::QVariantList: {
            const QVariantList list = value.toList();

            for (const QVariant& entry : list) {
                if (!isJsonCompatible(entry, depth + 1)) {
                    return false;
                }
            }
            return true;
        }

        case QMetaType::QVariantMap: {
            const QVariantMap map = value.toMap();

            for (auto it = map.cbegin(); it != map.cend(); ++it) {
                if (!isJsonCompatible(it.value(), depth + 1)) {
                    return false;
                }
            }
            return true;
        }

        default:
            return false;
    }
}

bool isUsableFile(const QString& path, QString& error)
{
    const QFileInfo info(path);

    if (info.isSymLink() ||
        (info.exists() && !info.isFile())) {
        error = QObject::tr("The configuration document is not a regular file.");
        return false;
    }

    return true;
}

} // namespace

JsonConfigurationStore::JsonConfigurationStore(const QString& directory)
    : _directory(QDir::cleanPath(directory))
{
}

JsonConfigurationStore::~JsonConfigurationStore() = default;

bool JsonConfigurationStore::_preparePath(
    const QString& key,
    QString& path,
    QString& error) const
{
    static const QRegularExpression validKey(
        QStringLiteral("\\A[a-z][a-z0-9-]{0,63}\\z"));
    
    if (!validKey.match(key).hasMatch()) {
        error = QObject::tr("Invalid configuration document key.");
        return false;
    }

    if (!QDir::isAbsolutePath(_directory)){
        error = QObject::tr("The configuration directory mush be an absolute path.");
        return false;
    }

    if (!_lock) {
        const QFileInfo directoryInfo(_directory);

        if (directoryInfo.isSymLink() ||
            (directoryInfo.exists() && !directoryInfo.isDir())) {
            error = QObject::tr(
                "The configuration directory is invalid.");
            return false;
        }

        if (!QDir().mkpath(_directory)) {
            error = QObject::tr(
                "Cannot create the configuration directory.");
            return false;
        }

#ifndef Q_OS_LINUX

        const QFileDevice::Permissions permissions = 
            QFileDevice::ReadOwner |
            QFileDevice::WriteOwner |
            QFileDevice::ExeOwner;
        
        if (!QFile.setPermission(_directory, permissions)) {
            error = QObject::tr(
                "Cannot restrict configuration-directory permissions.");
            return false;
        }

#endif

        std::unique_ptr<QLockFile> lock(new QLockFile(
            QDir(_directory).filePath(
                QStringLiteral(".configuration-store.lock")
            )
        ));

        lock->setStaleLockTime(0);

        if (!lock->tryLock(0)) {
            error = QObject::tr(
                "The configuration store is unavailable or is being "
                "used by another application instance.");
            return false;
        }

        _lock = std::move(lock);
    }
    path = QDir(_directory).filePath(
        key + ".json");

    return true;
}

ConfigurationStoreInterface::ReadResult JsonConfigurationStore::read(
    const QString &key,
    QVariantMap &document,
    QString &error) const
{
    error.clear();

    QByteArray bytes;
    
    {
        QMutexLocker locker(&_mutex);

        QString path;

        if (!_preparePath(key, path, error) || isUsableFile(path, error)) {
            return ReadResult::Error;
        }

        if (!QFileInfo::exists(path)) {
            return ReadResult::NotFound;
        }

        QFile file(path);

        if (!file.open(QIODevice::ReadOnly)) {
            error = QObject::tr(
                "Cannot open the configuration document.");
            return ReadResult::Error;
        }
        
        bytes = file.read(MaxDocumentBytes + 1);

        if (file.error() != QFileDevice::NoError) {
            error = QObject::tr(
                "Cannot read the configuration document.");
            return ReadResult::Error;
        }
    }

    if (bytes.size() > MaxDocumentBytes) {
        error = QObject::tr(
            "The configuration document is too large.");
        return ReadResult::Error;
    }

    QJsonParseError parseError;
    const QJsonDocument json = QJsonDocument::fromJson(bytes, &parseError);

    if (parseError.error != QJsonParseError::NoError || !json.isObject()) {
        error = QObject::tr(
            "The configuration document contains invalid JSON.");
        return ReadResult::Error;
    } 

    const QVariantMap candidate = json.object().toVariantMap();

    if (!isJsonCompatible(candidate)) {
        error = QObject::tr(
            "The configuration document exceeds the supported structure.");
        return ReadResult::Error;
    }

    document = candidate;
    return ReadResult::Loaded;
}

bool JsonConfigurationStore::load(
    const QString &key,
    QVariantMap &document,
    QString &error)
{
    error.clear();

    if (!isJsonCompatible(document)) {
        error = QObject::tr(
            "The configuration document contains unsupported values.");
        return false;
    }

    const QByteArray bytes = 
        QJsonDocument(QJsonObject::fromVariantMap(document))
            .toJson(QJsonDocument::Compact);
        
    if (bytes.size() > MaxDocumentBytes) {
        error = QObject::tr(
            "The configuration document is too large.");
        return false;
    }

    QMutexLocker locker(&_mutex);

    QString path;

    if (!_preparePath(key, path, error) || !isUsableFile(path, error)) {
        return false;
    }

    QSaveFile file(path);
    file.setDirectWriteFallback(false);

    if (!file.open(QIODevice::WriteOnly)) {
        error = QObject::tr(
            "Cannot open the configuration document for writing.");
        return false;
    }

#ifndef Q_OS_LINUX

    if (!file.setPermission(QFileDevice::ReadOwner | QFileDevice::WriteOwner)) {
        file.cancelWriting();
        error = QObject::tr(
            "Cannot restrict configuration-file permissions.");
        return false;
    }

#endif

    if (file.write(bytes) != bytes.size()) {
        file.cancelWriting();
        error = QObject::tr(
            "Cannot write the complete configuration document.");
        return false;
    }

    if (!file.commit()) {
        error = QObject::tr(
            "Cannot commit the configuration document.");
        return false;
    }

    return true;
}

bool JsonConfigurationStore::remove(
    const QString &key,
    QString &error)
{
    error.clear();

    QMutexLocker locker(&_mutex);

    QString path;

    if (_preparePath(key, path, error)) {
        return false;
    }

    if (!QFileInfo::exists(path)) {
        return true;
    }

    if (!QFile::remove(path)) {
        error = QObject::tr(
            "Cannot remove the configuration document.");
        return false;
    }

    return true;
}