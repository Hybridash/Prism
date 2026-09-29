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

#pragma once

#include <QFileInfo>
#include <QList>
#include <QString>
#include <QStringList>

/**
 * Zip-based world backups.
 *
 * Backups live in <instance>/backups/<world folder>/<yyyy-MM-dd_HH-mm-ss-zzz>.zip and contain the
 * world folder itself, so restoring is just "extract into saves/".
 */
namespace WorldBackup {

/// Folder that holds every world's backups for an instance.
QString backupRoot(const QString& instanceRoot);

/// Folder that holds the backups of one world.
QString backupDirFor(const QString& instanceRoot, const QString& worldFolderName);

/// Backups of one world, newest first.
QFileInfoList listBackups(const QString& instanceRoot, const QString& worldFolderName);

/// True if the world changed since its newest backup (or has no backup yet).
bool needsBackup(const QString& worldPath, const QString& instanceRoot);

/**
 * Zips a single world folder and removes the oldest backups so that at most @p keep remain.
 * Returns the path of the new zip, or an empty string with @p error set on failure.
 * Safe to call from a worker thread.
 */
QString backupWorld(const QString& worldPath, const QString& instanceRoot, int keep, QString* error = nullptr);

/**
 * Backs up every world in @p savesDir that changed since its last backup.
 * Returns one human-readable line per world, for the launch log.
 */
QStringList backupChangedWorlds(const QString& savesDir, const QString& instanceRoot, int keep);

/**
 * Replaces a world with the contents of a backup zip.
 * The current world folder is first renamed to "<name> (before restore <time>)" so nothing is lost.
 */
bool restoreBackup(const QString& backupZip, const QString& savesDir, const QString& worldFolderName, QString* error = nullptr);

}  // namespace WorldBackup
