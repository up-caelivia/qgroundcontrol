/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "NTRIPManagerTest.h"

#include "Configuration/ConfigurationStoreInterface.h"
#include "NTRIPConfigurationCodec.h"
#include "NTRIPConfigurationQmlAdapter.h"
#include "NTRIPManager.h"
#include "QGCApplication.h"

#include <QHostAddress>
#include <QMutex>
#include <QMutexLocker>
#include <QSemaphore>
#include <QSignalSpy>
#include <QStringList>
#include <QTcpServer>
#include <QTcpSocket>

#include <memory>
#include <utility>

namespace {

NTRIPConfiguration profile(quint16 port = 2101)
{
    NTRIPConfiguration configuration;
    configuration.id = QStringLiteral("11111111-1111-4111-8111-111111111111");
    configuration.name = QStringLiteral("Original");
    configuration.hostAddress = QStringLiteral("127.0.0.1");
    configuration.port = port;
    configuration.username = QStringLiteral("test-user");
    configuration.password = QStringLiteral("original-secret");
    configuration.enableVRS = false;
    return configuration;
}

QVariantMap profileDocument(const NTRIPConfiguration& configuration)
{
    NTRIPConfigurations profiles;
    profiles.insert(configuration.id, configuration);
    return NTRIPConfigurationCodec::encode(profiles);
}

class ControlledStore final : public ConfigurationStoreInterface
{
public:
    explicit ControlledStore(QVariantMap document)
        : _document(std::move(document))
    {
    }

    ReadResult read(
        const QString& key,
        QVariantMap& document,
        QString& error) const override
    {
        QMutexLocker locker(&_mutex);

        if (key != QStringLiteral("ntrip")) {
            error = QStringLiteral("Unexpected test document key.");
            return ReadResult::Error;
        }

        error.clear();
        document = _document;
        return ReadResult::Loaded;
    }

    bool write(
        const QString& key,
        const QVariantMap& document,
        QString& error) override
    {
        bool pause = false;
        bool fail = false;

        {
            QMutexLocker locker(&_mutex);

            if (key != QStringLiteral("ntrip")) {
                error = QStringLiteral("Unexpected test document key.");
                return false;
            }

            ++_writeCount;
            pause = _pauseNextWrite;
            fail = _failNextWrite;
            _pauseNextWrite = false;
            _failNextWrite = false;
            _writeBlocked = pause;
        }

        if (pause) {
            _release.acquire();
            QMutexLocker locker(&_mutex);
            _writeBlocked = false;
        }

        if (fail) {
            error = QStringLiteral("Simulated configuration write failure.");
            return false;
        }

        QMutexLocker locker(&_mutex);
        _document = document;
        error.clear();
        return true;
    }

    bool remove(const QString&, QString& error) override
    {
        QMutexLocker locker(&_mutex);
        ++_removeCount;
        _document.clear();
        error.clear();
        return true;
    }

    void pauseNextWrite(bool fail = false)
    {
        QMutexLocker locker(&_mutex);
        _pauseNextWrite = true;
        _failNextWrite = fail;
    }

    bool writeBlocked() const
    {
        QMutexLocker locker(&_mutex);
        return _writeBlocked;
    }

    void releaseWrite()
    {
        _release.release();
    }

    QVariantMap document() const
    {
        QMutexLocker locker(&_mutex);
        return _document;
    }

    int writeCount() const
    {
        QMutexLocker locker(&_mutex);
        return _writeCount;
    }

    int removeCount() const
    {
        QMutexLocker locker(&_mutex);
        return _removeCount;
    }

private:
    mutable QMutex _mutex;
    QSemaphore _release;
    QVariantMap _document;
    bool _pauseNextWrite = false;
    bool _failNextWrite = false;
    bool _writeBlocked = false;
    int _writeCount = 0;
    int _removeCount = 0;
};

// Release even when a QVERIFY/QCOMPARE returns early from the test.
class WriteGate
{
public:
    explicit WriteGate(std::shared_ptr<ControlledStore> store)
        : _store(std::move(store))
    {
    }

    ~WriteGate()
    {
        release();
    }

    void release()
    {
        if (!_released) {
            _released = true;
            _store->releaseWrite();
        }
    }

private:
    Q_DISABLE_COPY(WriteGate)

    std::shared_ptr<ControlledStore> _store;
    bool _released = false;
};

} // namespace

void NTRIPManagerTest::init()
{
    UnitTest::init();
    qRegisterMetaType<NTRIPManager::MutationResult>("NTRIPManager::MutationResult");
}

void NTRIPManagerTest::_rejectsBeforeInitialization()
{
    const auto configuration = profile();
    auto store = std::make_shared<ControlledStore>(profileDocument(configuration));
    NTRIPManager manager(qgcApp(), qgcApp()->toolbox(), store);
    QSignalSpy rejected(&manager, &NTRIPManager::operationRejected);

    QCOMPARE(
        manager.addConfiguration(NTRIPConfigurationQmlAdapter::toValues(configuration)),
        NTRIPManager::NotReady);
    QCOMPARE(rejected.count(), 1);
    QCOMPARE(store->writeCount(), 0);
    QVERIFY(!manager.storageBusy());
}

void NTRIPManagerTest::_commitsOnlyAfterSuccessfulWrite()
{
    const auto configuration = profile();
    auto store = std::make_shared<ControlledStore>(profileDocument(configuration));
    NTRIPManager manager(qgcApp(), qgcApp()->toolbox(), store);
    manager.setToolbox(qgcApp()->toolbox());
    QTRY_VERIFY(manager.storageReady() && !manager.storageBusy());

    QCOMPARE(manager.connectionState(), NTRIPConnection::Disconnected);
    QVERIFY(manager.connectionId().isEmpty());
    QCOMPARE(store->writeCount(), 0);

    QSignalSpy changed(&manager, &NTRIPManager::configurationsChanged);
    QSignalSpy finished(&manager, &NTRIPManager::configurationOperationFinished);
    QSignalSpy rejected(&manager, &NTRIPManager::operationRejected);

    QVariantMap values = manager.configuration(configuration.id);
    values.insert(QStringLiteral("name"), QStringLiteral("Updated"));
    store->pauseNextWrite();
    WriteGate gate(store);

    QCOMPARE(manager.updateConfiguration(configuration.id, values), NTRIPManager::Accepted);
    QTRY_VERIFY(store->writeBlocked());

    QVERIFY(manager.storageBusy());
    QCOMPARE(manager.configuration(configuration.id).value(QStringLiteral("name")).toString(),
             configuration.name);
    QCOMPARE(changed.count(), 0);
    QCOMPARE(finished.count(), 0);

    QCOMPARE(manager.addConfiguration(values), NTRIPManager::Busy);
    QCOMPARE(manager.removeConfiguration(configuration.id), NTRIPManager::Busy);
    QCOMPARE(rejected.count(), 2);

    gate.release();
    QTRY_COMPARE(finished.count(), 1);

    QVERIFY(!manager.storageBusy());
    QVERIFY(finished.at(0).at(2).toBool());
    QCOMPARE(changed.count(), 1);
    QCOMPARE(manager.configuration(configuration.id).value(QStringLiteral("name")).toString(),
             QStringLiteral("Updated"));

    NTRIPConfigurations persisted;
    QString error;
    QVERIFY2(NTRIPConfigurationCodec::decode(store->document(), persisted, error),
             qPrintable(error));
    QCOMPARE(persisted.value(configuration.id).password, configuration.password);
    QCOMPARE(persisted.value(configuration.id).id, configuration.id);
    QVERIFY(!manager.configuration(configuration.id).contains(QStringLiteral("password")));
}

void NTRIPManagerTest::_failedWritePreservesCommittedState()
{
    const auto configuration = profile();
    const QVariantMap original = profileDocument(configuration);
    auto store = std::make_shared<ControlledStore>(original);
    NTRIPManager manager(qgcApp(), qgcApp()->toolbox(), store);
    manager.setToolbox(qgcApp()->toolbox());
    QTRY_VERIFY(manager.storageReady() && !manager.storageBusy());

    QSignalSpy changed(&manager, &NTRIPManager::configurationsChanged);
    QSignalSpy finished(&manager, &NTRIPManager::configurationOperationFinished);
    QVariantMap values = manager.configuration(configuration.id);
    values.insert(QStringLiteral("name"), QStringLiteral("Uncommitted"));

    store->pauseNextWrite(true);
    WriteGate gate(store);
    QCOMPARE(manager.updateConfiguration(configuration.id, values), NTRIPManager::Accepted);
    QTRY_VERIFY(store->writeBlocked());
    gate.release();

    QTRY_COMPARE(finished.count(), 1);
    QVERIFY(!finished.at(0).at(2).toBool());
    QVERIFY(!finished.at(0).at(3).toString().isEmpty());
    QVERIFY(manager.storageReady());
    QVERIFY(!manager.storageBusy());
    QCOMPARE(changed.count(), 0);
    QCOMPARE(manager.configuration(configuration.id).value(QStringLiteral("name")).toString(),
             configuration.name);
    QVERIFY(store->document() == original);
}

void NTRIPManagerTest::_rejectsInvalidAndMissingProfiles()
{
    const auto configuration = profile();
    const QVariantMap original = profileDocument(configuration);
    auto store = std::make_shared<ControlledStore>(original);
    NTRIPManager manager(qgcApp(), qgcApp()->toolbox(), store);
    manager.setToolbox(qgcApp()->toolbox());
    QTRY_VERIFY(manager.storageReady() && !manager.storageBusy());

    QVariantMap values = manager.configuration(configuration.id);
    values.insert(QStringLiteral("name"), QStringLiteral("Invalid"));
    values.insert(QStringLiteral("port"), QStringLiteral("0"));

    QCOMPARE(manager.updateConfiguration(configuration.id, values), NTRIPManager::Invalid);
    QCOMPARE(manager.addConfiguration(values), NTRIPManager::Invalid);
    QCOMPARE(manager.configurations().size(), 1);
    QCOMPARE(manager.configuration(configuration.id).value(QStringLiteral("name")).toString(),
             configuration.name);
    QCOMPARE(store->writeCount(), 0);
    QVERIFY(store->document() == original);
    QVERIFY(!manager.storageBusy());

    QCOMPARE(manager.updateConfiguration(QStringLiteral("missing"), values),
             NTRIPManager::NotFound);
    QCOMPARE(manager.removeConfiguration(QStringLiteral("missing")), NTRIPManager::NotFound);
    QVERIFY(manager.configuration(QStringLiteral("missing")).isEmpty());
}

void NTRIPManagerTest::_addsAndRemovesProfiles()
{
    const auto configuration = profile();
    auto store = std::make_shared<ControlledStore>(profileDocument(configuration));
    NTRIPManager manager(qgcApp(), qgcApp()->toolbox(), store);
    manager.setToolbox(qgcApp()->toolbox());
    QTRY_VERIFY(manager.storageReady() && !manager.storageBusy());

    QSignalSpy finished(&manager, &NTRIPManager::configurationOperationFinished);
    QVariantMap values = NTRIPConfigurationQmlAdapter::toValues(configuration);
    values.insert(QStringLiteral("name"), QStringLiteral("Added"));
    values.insert(QStringLiteral("passwordAction"), QStringLiteral("replace"));
    values.insert(QStringLiteral("password"), QStringLiteral("added-secret"));

    QCOMPARE(manager.addConfiguration(values), NTRIPManager::Accepted);
    QCOMPARE(manager.configurations().size(), 1);
    QTRY_COMPARE(finished.count(), 1);
    QVERIFY(finished.at(0).at(2).toBool());

    const QString addedId = finished.at(0).at(1).toString();
    QVERIFY(!addedId.isEmpty());
    QVERIFY(addedId != configuration.id);
    QCOMPARE(manager.configurations().size(), 2);

    NTRIPConfigurations persisted;
    QString error;
    QVERIFY2(NTRIPConfigurationCodec::decode(store->document(), persisted, error),
             qPrintable(error));
    QCOMPARE(persisted.value(addedId).password, QStringLiteral("added-secret"));

    QCOMPARE(manager.removeConfiguration(addedId), NTRIPManager::Accepted);
    QTRY_COMPARE(finished.count(), 2);
    QCOMPARE(manager.configurations().size(), 1);

    QCOMPARE(manager.removeConfiguration(configuration.id), NTRIPManager::Accepted);
    QTRY_COMPARE(finished.count(), 3);
    QVERIFY(manager.configurations().isEmpty());
    QVERIFY(store->document().value(QStringLiteral("profiles")).toList().isEmpty());
    QCOMPARE(store->removeCount(), 0);

    const int writes = store->writeCount();
    NTRIPManager restored(qgcApp(), qgcApp()->toolbox(), store);
    restored.setToolbox(qgcApp()->toolbox());
    QTRY_VERIFY(restored.storageReady() && !restored.storageBusy());
    QVERIFY(restored.configurations().isEmpty());
    QCOMPARE(store->writeCount(), writes);
}

void NTRIPManagerTest::_passwordActions_data()
{
    QTest::addColumn<QString>("action");
    QTest::addColumn<QString>("replacement");
    QTest::addColumn<QString>("expected");
    QTest::addColumn<bool>("omitAction");

    QTest::newRow("keep")
        << QStringLiteral("keep") << QString{}
        << QStringLiteral("original-secret") << false;
    QTest::newRow("omitted")
        << QStringLiteral("keep") << QString{}
        << QStringLiteral("original-secret") << true;
    QTest::newRow("replace")
        << QStringLiteral("replace") << QStringLiteral(" new password ")
        << QStringLiteral(" new password ") << false;
    QTest::newRow("clear")
        << QStringLiteral("clear") << QString{} << QString{} << false;
}

void NTRIPManagerTest::_passwordActions()
{
    QFETCH(QString, action);
    QFETCH(QString, replacement);
    QFETCH(QString, expected);
    QFETCH(bool, omitAction);

    const auto configuration = profile();
    auto store = std::make_shared<ControlledStore>(profileDocument(configuration));
    NTRIPManager manager(qgcApp(), qgcApp()->toolbox(), store);
    manager.setToolbox(qgcApp()->toolbox());
    QTRY_VERIFY(manager.storageReady() && !manager.storageBusy());

    QVariantMap values = manager.configuration(configuration.id);

    if (omitAction) {
        values.remove(QStringLiteral("passwordAction"));
    } else {
        values.insert(QStringLiteral("passwordAction"), action);
    }

    if (action == QStringLiteral("replace")) {
        values.insert(QStringLiteral("password"), replacement);
    }

    QSignalSpy finished(&manager, &NTRIPManager::configurationOperationFinished);
    QCOMPARE(manager.updateConfiguration(configuration.id, values), NTRIPManager::Accepted);
    QTRY_COMPARE(finished.count(), 1);
    QVERIFY(finished.at(0).at(2).toBool());

    NTRIPConfigurations persisted;
    QString error;
    QVERIFY2(NTRIPConfigurationCodec::decode(store->document(), persisted, error),
             qPrintable(error));
    QCOMPARE(persisted.value(configuration.id).password, expected);
    const QVariantMap output = manager.configuration(configuration.id);
    QVERIFY(!output.contains(QStringLiteral("password")));
    QCOMPARE(output.value(QStringLiteral("hasPassword")).toBool(), !expected.isEmpty());
}

void NTRIPManagerTest::_shutdownWaitsForCommit()
{
    const auto configuration = profile();
    auto store = std::make_shared<ControlledStore>(profileDocument(configuration));
    NTRIPManager manager(qgcApp(), qgcApp()->toolbox(), store);
    manager.setToolbox(qgcApp()->toolbox());
    QTRY_VERIFY(manager.storageReady() && !manager.storageBusy());

    QSignalSpy ready(&manager, &NTRIPManager::shutdownReady);
    QStringList order;
    connect(&manager, &NTRIPManager::configurationOperationFinished,
            &manager, [&order]() { order.append(QStringLiteral("finished")); });
    connect(&manager, &NTRIPManager::shutdownReady,
            &manager, [&order]() { order.append(QStringLiteral("ready")); });

    store->pauseNextWrite();
    WriteGate gate(store);
    QVariantMap values = manager.configuration(configuration.id);
    values.insert(QStringLiteral("name"), QStringLiteral("Saved before exit"));

    QCOMPARE(manager.updateConfiguration(configuration.id, values), NTRIPManager::Accepted);
    QTRY_VERIFY(store->writeBlocked());
    manager.requestShutdown();
    manager.requestShutdown();

    QVERIFY(manager.shutdownRequested());
    QCOMPARE(ready.count(), 0);
    QCOMPARE(manager.removeConfiguration(configuration.id), NTRIPManager::ShuttingDown);

    gate.release();
    QTRY_COMPARE(ready.count(), 1);
    QVERIFY(!manager.storageBusy());
    QCOMPARE(order, QStringList({QStringLiteral("finished"), QStringLiteral("ready")}));
    QCOMPARE(manager.configuration(configuration.id).value(QStringLiteral("name")).toString(),
             QStringLiteral("Saved before exit"));

    manager.requestShutdown();
    QCOMPARE(ready.count(), 1);
}

void NTRIPManagerTest::_shutdownFailureCanRetry()
{
    const auto configuration = profile();
    auto store = std::make_shared<ControlledStore>(profileDocument(configuration));
    NTRIPManager manager(qgcApp(), qgcApp()->toolbox(), store);
    manager.setToolbox(qgcApp()->toolbox());
    QTRY_VERIFY(manager.storageReady() && !manager.storageBusy());

    QSignalSpy ready(&manager, &NTRIPManager::shutdownReady);
    QSignalSpy failed(&manager, &NTRIPManager::shutdownFailed);

    store->pauseNextWrite(true);
    WriteGate gate(store);
    QCOMPARE(manager.removeConfiguration(configuration.id), NTRIPManager::Accepted);
    QTRY_VERIFY(store->writeBlocked());

    manager.requestShutdown();
    QVERIFY(manager.shutdownRequested());
    gate.release();

    QTRY_COMPARE(failed.count(), 1);
    QVERIFY(!manager.shutdownRequested());
    QVERIFY(!manager.storageBusy());
    QCOMPARE(ready.count(), 0);
    QCOMPARE(manager.configurations().size(), 1);

    manager.requestShutdown();
    QCOMPARE(ready.count(), 1);
    manager.requestShutdown();
    QCOMPARE(ready.count(), 1);
}

void NTRIPManagerTest::_connectionDuringPendingDeletion()
{
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));

    const auto configuration = profile(server.serverPort());
    auto store = std::make_shared<ControlledStore>(profileDocument(configuration));
    NTRIPManager manager(qgcApp(), qgcApp()->toolbox(), store);
    manager.setToolbox(qgcApp()->toolbox());
    QTRY_VERIFY(manager.storageReady() && !manager.storageBusy());

    QSignalSpy finished(&manager, &NTRIPManager::configurationOperationFinished);
    store->pauseNextWrite();
    WriteGate gate(store);

    QCOMPARE(manager.removeConfiguration(configuration.id), NTRIPManager::Accepted);
    QTRY_VERIFY(store->writeBlocked());

    QVERIFY(manager.connectConfiguration(configuration.id));
    QTRY_VERIFY(server.hasPendingConnections());
    std::unique_ptr<QTcpSocket> peer(server.nextPendingConnection());
    QVERIFY(peer);
    QTRY_COMPARE(manager.connectionState(), NTRIPConnection::Connected);
    QCOMPARE(manager.connectionName(), configuration.name);

    gate.release();
    QTRY_COMPARE(finished.count(), 1);
    QVERIFY(manager.configurations().isEmpty());
    QCOMPARE(manager.connectionId(), configuration.id);
    QCOMPARE(manager.connectionName(), configuration.name);
    QCOMPARE(manager.connectionState(), NTRIPConnection::Connected);

    manager.disconnectConnection();
    QVERIFY(manager.connectionId().isEmpty());
    QVERIFY(manager.connectionName().isEmpty());
    QCOMPARE(manager.connectionState(), NTRIPConnection::Disconnected);
}

void NTRIPManagerTest::_unsupportedDocumentIsNotOverwritten()
{
    const QVariantMap unsupported {
        {QStringLiteral("fileType"), QStringLiteral("NTRIPProfiles")},
        {QStringLiteral("version"), 99},
        {QStringLiteral("profiles"), QVariantList{}}
    };

    auto store = std::make_shared<ControlledStore>(unsupported);
    NTRIPManager manager(qgcApp(), qgcApp()->toolbox(), store);
    QSignalSpy errors(&manager, &NTRIPManager::errorOccurred);
    manager.setToolbox(qgcApp()->toolbox());
    QTRY_VERIFY(!manager.storageBusy());

    QVERIFY(!manager.storageReady());
    QVERIFY(manager.configurations().isEmpty());
    QCOMPARE(errors.count(), 1);
    QCOMPARE(store->writeCount(), 0);
    QVERIFY(store->document() == unsupported);
}
