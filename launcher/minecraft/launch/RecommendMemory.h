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

/// Hybrid Launcher: suggests a better "maximum memory" setting based on how many mods are installed.
class RecommendMemory : public LaunchStep {
    Q_OBJECT

   public:
    explicit RecommendMemory(LaunchTask* parent, MinecraftInstance* instance);
    ~RecommendMemory() override = default;

    void executeTask() override;
    bool canAbort() const override { return false; }

    /// Suggested maximum memory in MiB for a given number of mods and total system RAM (0 = unknown).
    static int recommendedMiB(int modCount, uint64_t totalRamMiB);

   private:
    MinecraftInstance* m_instance;
};
