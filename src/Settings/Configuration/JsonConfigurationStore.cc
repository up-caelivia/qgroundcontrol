/****************************************************************************
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "JsonConfigurationStore.h"

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

#ifdef Q_OS_UNIX
#include <cerrno>
#include <sys/stat.h>
#elif defined(Q_OS_WIN)
#include <qt_windows.h>
#endif

namespace {

constexpr qint64 MaxDocumentBytes = 1024 * 1024;
constexpr int MaxDocumentDepth = 16;

enum class DocumentStatus {
    Missing,
    Present,
    Error
};

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

    case QMetaType::ULongLong:
        return value.toULongLong() <= 9007199254740991ULL;

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

DocumentStatus documentStatus(const QString& path, QString& error)
{
#ifdef Q_OS_UNIX
    struct stat status {};
    const QByteArray encodedPath = QFile::encodeName(path);

    if (::lstat(encodedPath.constData(), &status) == 0) {
        if (!S_ISREG(status.st_mode)) {
            error = QObject::tr(
                "The configuration document is not a regular file.");
            return DocumentStatus::Error;
        }

        return DocumentStatus::Present;
    }

    const int nativeError = errno;

    if (nativeError == ENOENT) {
        return DocumentStatus::Missing;
    }

    error = QObject::tr("Cannot inspect the configuration document.");
    return DocumentStatus::Error;
#elif defined(Q_OS_WIN)
    const DWORD attributes = GetFileAttributesW(
        reinterpret_cast<LPCWSTR>(path.utf16()));

    if (attributes == INVALID_FILE_ATTRIBUTES) {
        if (GetLastError() == ERROR_FILE_NOT_FOUND) {
            return DocumentStatus::Missing;
        }

        error = QObject::tr("Cannot inspect the configuration document.");
        return DocumentStatus::Error;
    }

    if (attributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) {
        error = QObject::tr(
            "The configuration document is not a regular file.");
        return DocumentStatus::Error;
    }

    return DocumentStatus::Present;
#else
    Q_UNUSED(path);
    error = QObject::tr("Configuration storage is unsupported on this platform.");
    return DocumentStatus::Error;
#endif
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

    if (!QDir::isAbsolutePath(_directory)) {
        error = QObject::tr(
            "The configuration directory must be an absolute path.");
        return false;
    }

    if (!_lock) {
        const QFileInfo directoryInfo(_directory);

        if (directoryInfo.isSymLink()
            || (directoryInfo.exists() && !directoryInfo.isDir())) {
            error = QObject::tr("The configuration directory is invalid.");
            return false;
        }

        if (!QDir().mkpath(_directory)) {
            error = QObject::tr("Cannot create the configuration directory.");
            return false;
        }

#ifdef Q_OS_UNIX
        const QFileDevice::Permissions permissions =
            QFileDevice::ReadOwner
            | QFileDevice::WriteOwner
            | QFileDevice::ExeOwner;

        if (!QFile::setPermissions(_directory, permissions)) {
            error = QObject::tr(
                "Cannot restrict configuration-directory permissions.");
            return false;
        }
#endif

        std::unique_ptr<QLockFile> lock(new QLockFile(
            QDir(_directory).filePath(
                QStringLiteral(".configuration-store.lock"))));

        // Retain the single-writer lock from before loading until destruction.
        lock->setStaleLockTime(0);

        if (!lock->tryLock(0)) {
            error = QObject::tr(
                "The configuration store is unavailable or is being "
                "used by another application instance.");
            return false;
        }

        _lock = std::move(lock);
    }

    path = QDir(_directory).filePath(key + QStringLiteral(".json"));
    return true;
}

ConfigurationStoreInterface::ReadResult JsonConfigurationStore::read(
    const QString& key,
    QVariantMap& document,
    QString& error) const
{
    error.clear();
    QByteArray bytes;

    {
        QMutexLocker locker(&_mutex);
        QString path;

        if (!_preparePath(key, path, error)) {
            return ReadResult::Error;
        }

        const DocumentStatus status = documentStatus(path, error);

        if (status == DocumentStatus::Error) {
            return ReadResult::Error;
        }

        if (status == DocumentStatus::Missing) {
            return ReadResult::NotFound;
        }

        QFile file(path);

        if (!file.open(QIODevice::ReadOnly)) {
            error = QObject::tr("Cannot open the configuration document.");
            return ReadResult::Error;
        }

        bytes = file.read(MaxDocumentBytes + 1);

        if (file.error() != QFileDevice::NoError) {
            error = QObject::tr("Cannot read the configuration document.");
            return ReadResult::Error;
        }
    }

    if (bytes.size() > MaxDocumentBytes) {
        error = QObject::tr("The configuration document is too large.");
        return ReadResult::Error;
    }

    QJsonParseError parseError;
    const QJsonDocument json = QJsonDocument::fromJson(bytes, &parseError);

    if (parseError.error != QJsonParseError::NoError || !json.isObject()) {
        error = QObject::tr("The configuration document contains invalid JSON.");
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

bool JsonConfigurationStore::write(
    const QString& key,
    const QVariantMap& document,
    QString& error)
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
        error = QObject::tr("The configuration document is too large.");
        return false;
    }

    QMutexLocker locker(&_mutex);
    QString path;

    if (!_preparePath(key, path, error)
        || documentStatus(path, error) == DocumentStatus::Error) {
        return false;
    }

    QSaveFile file(path);
    file.setDirectWriteFallback(false);

    if (!file.open(QIODevice::WriteOnly)) {
        error = QObject::tr(
            "Cannot open the configuration document for writing.");
        return false;
    }

#ifdef Q_OS_UNIX
    if (!file.setPermissions(
            QFileDevice::ReadOwner | QFileDevice::WriteOwner)) {
        file.cancelWriting();
        error = QObject::tr("Cannot restrict configuration-file permissions.");
        return false;
    }
#endif

    if (file.write(bytes) != bytes.size()) {
        file.cancelWriting();
        error = QObject::tr("Cannot write the complete configuration document.");
        return false;
    }

    if (!file.commit()) {
        error = QObject::tr("Cannot commit the configuration document.");
        return false;
    }

    return true;
}

bool JsonConfigurationStore::remove(const QString& key, QString& error)
{
    error.clear();
    QMutexLocker locker(&_mutex);
    QString path;

    if (!_preparePath(key, path, error)) {
        return false;
    }

    const DocumentStatus status = documentStatus(path, error);

    if (status == DocumentStatus::Error) {
        return false;
    }

    if (status == DocumentStatus::Missing) {
        return true;
    }

    if (!QFile::remove(path)) {
        error = QObject::tr("Cannot remove the configuration document.");
        return false;
    }

    return true;
}
