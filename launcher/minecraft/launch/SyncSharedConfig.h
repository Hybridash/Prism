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

#include "launch/LaunchStep.h"
#include "minecraft/MinecraftInstance.h"

/**
 * Hybrid Launcher: keeps keybinds and the multiplayer server list the same in every instance.
 *
 * The shared copy lives in <launcher data>/shared-config/.
 *  - Before launch (Direction::Pull) the shared keybinds and servers.dat are copied into the instance.
 *  - After the game closes normally (Direction::Push) the instance's keybinds and servers.dat are saved
 *    back, so a change made in one instance shows up in all of them next time.
 */
class SyncSharedConfig : public LaunchStep {
    Q_OBJECT

   public:
    enum class Direction { Pull, Push };

    explicit SyncSharedConfig(LaunchTask* parent, MinecraftInstance* instance, Direction direction);
    ~SyncSharedConfig() override = default;

    void executeTask() override;
    bool canAbort() const override { return false; }

    static QString sharedDir();

   private:
    MinecraftInstance* m_instance;
    Direction m_direction;
};
