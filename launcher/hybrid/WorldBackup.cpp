// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Hybrid Launcher - a fork of Prism Launcher
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "WorldBackup.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>

#include "FileSystem.h"
#include "MMCZip.h"
#include "archive/ArchiveWriter.h"

namespace WorldBackup {

static QString tr(const char* text)
{
    return QCoreApplication::translate("WorldBackup", text);
}

QString backupRoot(const QString& instanceRoot)
{
    return FS::PathCombine(instanceRoot, "backups");
}

QString backupDirFor(const QString& instanceRoot, const QString& worldFolderName)
{
    return FS::PathCombine(backupRoot(instanceRoot), worldFolderName);
}

QFileInfoList listBackups(const QString& instanceRoot, const QString& worldFolderName)
{
    QDir dir(backupDirFor(instanceRoot, worldFolderName));
    if (!dir.exists()) {
        return {};
    }
    // names are timestamps, so sorting by name (descending) gives newest first
    return dir.entryInfoList({ "*.zip" }, QDir::Files, QDir::Name | QDir::Reversed);
}

static QDateTime newestChange(const QString& worldPath)
{
    QDateTime newest;
    QDirIterator it(worldPath, QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        const auto info = it.fileInfo();
        if (info.fileName() == "session.lock") {
            continue;
        }
        const auto modified = info.lastModified();
        if (!newest.isValid() || modified > newest) {
            newest = modified;
        }
    }
    return newest;
}

bool needsBackup(const QString& worldPath, const QString& instanceRoot)
{
    const auto backups = listBackups(instanceRoot, QFileInfo(worldPath).fileName());
    if (backups.isEmpty()) {
        return true;
    }
    const auto changed = newestChange(worldPath);
    return !changed.isValid() || changed > backups.first().lastModified();
}

static void pruneBackups(const QString& instanceRoot, const QString& worldFolderName, int keep)
{
    auto backups = listBackups(instanceRoot, worldFolderName);
    for (int i = std::max(keep, 1); i < backups.size(); i++) {
        QFile::remove(backups.at(i).absoluteFilePath());
    }
}

QString backupWorld(const QString& worldPath, const QString& instanceRoot, int keep, QString* error)
{
    const QFileInfo worldInfo(worldPath);
    const QString worldFolderName = worldInfo.fileName();
    const QString targetDir = backupDirFor(instanceRoot, worldFolderName);

    if (!FS::ensureFolderPathExists(targetDir)) {
        if (error) {
            *error = tr("Could not create the backup folder %1").arg(targetDir);
        }
        return {};
    }

    QString target = FS::PathCombine(targetDir, QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss-zzz") + ".zip");
    // two backups within the same second: add a suffix instead of overwriting
    for (int n = 2; QFile::exists(target); n++) {
        target = FS::PathCombine(targetDir, QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss-zzz") + QString("_%1.zip").arg(n));
    }

    // Paths inside the zip start with the world folder name, e.g. "My World/level.dat"
    const QDir savesDir = worldInfo.absoluteDir();

    MMCZip::ArchiveWriter zip(target);
    if (!zip.open()) {
        if (error) {
            *error = tr("Could not create %1").arg(target);
        }
        return {};
    }

    bool ok = true;
    QDirIterator it(worldInfo.absoluteFilePath(), QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        const auto info = it.fileInfo();
        // session.lock is held open by a running game and is useless in a backup
        if (info.fileName() == "session.lock") {
            continue;
        }
        if (!zip.addFile(info.absoluteFilePath(), savesDir.relativeFilePath(info.absoluteFilePath()))) {
            ok = false;
            if (error) {
                *error = tr("Could not add %1 to the backup").arg(info.fileName());
            }
            break;
        }
    }

    if (!zip.close()) {
        ok = false;
        if (error && error->isEmpty()) {
            *error = tr("Could not finish writing %1").arg(target);
        }
    }

    if (!ok) {
        QFile::remove(target);
        return {};
    }

    pruneBackups(instanceRoot, worldFolderName, keep);
    return target;
}

QStringList backupChangedWorlds(const QString& savesDir, const QString& instanceRoot, int keep)
{
    QStringList log;
    QDir saves(savesDir);
    if (!saves.exists()) {
        return log;
    }

    for (const auto& world : saves.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        // only real worlds have a level.dat
        if (!QFile::exists(FS::PathCombine(world.absoluteFilePath(), "level.dat"))) {
            continue;
        }
        if (!needsBackup(world.absoluteFilePath(), instanceRoot)) {
            log << tr("  %1: unchanged since the last backup, skipped").arg(world.fileName());
            continue;
        }
        QString error;
        const auto zip = backupWorld(world.absoluteFilePath(), instanceRoot, keep, &error);
        if (zip.isEmpty()) {
            log << tr("  %1: backup FAILED (%2)").arg(world.fileName(), error);
        } else {
            log << tr("  %1: backed up to %2").arg(world.fileName(), QFileInfo(zip).fileName());
        }
    }
    return log;
}

bool restoreBackup(const QString& backupZip, const QString& savesDir, const QString& worldFolderName, QString* error)
{
    const QString worldPath = FS::PathCombine(savesDir, worldFolderName);

    // Keep the current world around instead of deleting it
    QString aside;
    if (QFileInfo::exists(worldPath)) {
        const QString stamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH-mm-ss");
        aside = FS::PathCombine(savesDir, QString("%1 (before restore %2)").arg(worldFolderName, stamp));
        if (!QDir().rename(worldPath, aside)) {
            if (error) {
                *error = tr("Could not move the current world out of the way. Is the game still running?");
            }
            return false;
        }
    }

    if (!MMCZip::extractDir(backupZip, savesDir)) {
        // put the original world back so the player is no worse off
        if (!aside.isEmpty()) {
            FS::deletePath(worldPath);
            QDir().rename(aside, worldPath);
        }
        if (error) {
            *error = tr("Could not extract %1").arg(QFileInfo(backupZip).fileName());
        }
        return false;
    }
    return true;
}

}  // namespace WorldBackup
