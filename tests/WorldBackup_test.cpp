// SPDX-License-Identifier: GPL-3.0-only
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>
#include <QThread>

#include <FileSystem.h>
#include <hybrid/WorldBackup.h>

class WorldBackupTest : public QObject {
    Q_OBJECT

    static void writeFile(const QString& path, const QByteArray& data)
    {
        QFile f(path);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(data);
    }

   private slots:
    void test_backupPruneRestore()
    {
        QTemporaryDir instance;
        QVERIFY(instance.isValid());
        const QString saves = FS::PathCombine(instance.path(), "saves");
        const QString world = FS::PathCombine(saves, "My World");
        QVERIFY(QDir().mkpath(FS::PathCombine(world, "region")));
        writeFile(FS::PathCombine(world, "level.dat"), "original");
        writeFile(FS::PathCombine(world, "region", "r.0.0.mca"), "blocks");
        writeFile(FS::PathCombine(world, "session.lock"), "lock");

        // first launch: backed up
        QVERIFY(WorldBackup::needsBackup(world, instance.path()));
        const auto log = WorldBackup::backupChangedWorlds(saves, instance.path(), 2);
        QCOMPARE(log.size(), 1);
        QCOMPARE(WorldBackup::listBackups(instance.path(), "My World").size(), 1);

        // unchanged world: skipped
        QVERIFY(!WorldBackup::needsBackup(world, instance.path()));

        // keep only the newest 2
        for (int i = 0; i < 3; i++) {
            QString error;
            QVERIFY2(!WorldBackup::backupWorld(world, instance.path(), 2, &error).isEmpty(), qPrintable(error));
        }
        QCOMPARE(WorldBackup::listBackups(instance.path(), "My World").size(), 2);

        // break the world, then restore
        const auto backup = WorldBackup::listBackups(instance.path(), "My World").first().absoluteFilePath();
        writeFile(FS::PathCombine(world, "level.dat"), "griefed");
        QString error;
        QVERIFY2(WorldBackup::restoreBackup(backup, saves, "My World", &error), qPrintable(error));

        QFile restored(FS::PathCombine(world, "level.dat"));
        QVERIFY(restored.open(QIODevice::ReadOnly));
        QCOMPARE(restored.readAll(), QByteArray("original"));
        QVERIFY(QFile::exists(FS::PathCombine(world, "region", "r.0.0.mca")));
        QVERIFY(!QFile::exists(FS::PathCombine(world, "session.lock")));

        // the pre-restore world was kept aside, not deleted
        const auto entries = QDir(saves).entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        QCOMPARE(entries.size(), 2);
    }
};

QTEST_GUILESS_MAIN(WorldBackupTest)

#include "WorldBackup_test.moc"
