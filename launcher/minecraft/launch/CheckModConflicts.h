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

#include <QTimer>

#include "launch/LaunchStep.h"
#include "minecraft/MinecraftInstance.h"

/**
 * Hybrid Launcher: looks at the enabled mods right before launch and warns about
 *  - the same mod installed twice,
 *  - mods that are known not to work together,
 *  - required mods that seem to be missing.
 * It only writes warnings to the log; it never blocks the launch.
 */
class CheckModConflicts : public LaunchStep {
    Q_OBJECT

   public:
    explicit CheckModConflicts(LaunchTask* parent, MinecraftInstance* instance);
    ~CheckModConflicts() override = default;

    void executeTask() override;
    bool canAbort() const override { return false; }

   private:
    void runChecks();

    MinecraftInstance* m_instance;
    QTimer m_timeout;
    bool m_done = false;
};
