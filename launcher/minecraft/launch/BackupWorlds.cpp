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

#include "BackupWorlds.h"

#include <QtConcurrent>

#include "Application.h"
#include "hybrid/WorldBackup.h"
#include "settings/SettingsObject.h"

BackupWorlds::BackupWorlds(LaunchTask* parent, MinecraftInstance* instance) : LaunchStep(parent), m_instance(instance) {}

void BackupWorlds::executeTask()
{
    auto settings = APPLICATION->settings();
    if (!settings->get("HybridWorldBackups").toBool()) {
        emitSucceeded();
        return;
    }

    const QString savesDir = m_instance->worldDir();
    const QString instanceRoot = m_instance->instanceRoot();
    const int keep = std::max(1, settings->get("HybridWorldBackupsKeep").toInt());

    if (!QDir(savesDir).exists()) {
        emitSucceeded();
        return;
    }

    emit logLine(tr("Backing up worlds (keeping the newest %1 backups of each)...").arg(keep), MessageLevel::Launcher);

    connect(&m_watcher, &QFutureWatcher<QStringList>::finished, this, [this] {
        const auto lines = m_watcher.result();
        if (lines.isEmpty()) {
            emit logLine(tr("  No worlds to back up."), MessageLevel::Launcher);
        } else {
            emit logLines(lines, MessageLevel::Launcher);
        }
        // a failed backup is worth a warning, but it should never stop the game from starting
        emitSucceeded();
    });
    m_watcher.setFuture(QtConcurrent::run([savesDir, instanceRoot, keep] { return WorldBackup::backupChangedWorlds(savesDir, instanceRoot, keep); }));
}
