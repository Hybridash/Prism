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

#include <QFutureWatcher>
#include <QStringList>

#include "launch/LaunchStep.h"
#include "minecraft/MinecraftInstance.h"

/// Hybrid Launcher: zips every world that changed since its last backup, before the game starts.
class BackupWorlds : public LaunchStep {
    Q_OBJECT

   public:
    explicit BackupWorlds(LaunchTask* parent, MinecraftInstance* instance);
    ~BackupWorlds() override = default;

    void executeTask() override;
    bool canAbort() const override { return false; }

   private:
    MinecraftInstance* m_instance;
    QFutureWatcher<QStringList> m_watcher;
};
