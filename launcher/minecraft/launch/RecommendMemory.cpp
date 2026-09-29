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

#include "RecommendMemory.h"

#include <algorithm>

#include "Application.h"
#include "HardwareInfo.h"
#include "minecraft/mod/Mod.h"
#include "minecraft/mod/ModFolderModel.h"
#include "settings/SettingsObject.h"

RecommendMemory::RecommendMemory(LaunchTask* parent, MinecraftInstance* instance) : LaunchStep(parent), m_instance(instance) {}

int RecommendMemory::recommendedMiB(int modCount, uint64_t totalRamMiB)
{
    int recommended = 2048;
    if (modCount > 0) {
        recommended = 4096;
    }
    if (modCount > 60) {
        recommended = 6144;
    }
    if (modCount > 180) {
        recommended = 8192;
    }
    if (totalRamMiB > 0) {
        // leave at least 3 GiB for the operating system and everything else
        const int ceiling = static_cast<int>(std::max<int64_t>(1024, static_cast<int64_t>(totalRamMiB) - 3072));
        recommended = std::min(recommended, ceiling);
    }
    return recommended;
}

void RecommendMemory::executeTask()
{
    if (!APPLICATION->settings()->get("HybridMemoryAdvice").toBool()) {
        emitSucceeded();
        return;
    }

    int modCount = 0;
    if (auto* mods = m_instance->loaderModList()) {
        for (auto* mod : mods->allMods()) {
            if (mod->enabled()) {
                modCount++;
            }
        }
    }

    const uint64_t totalRam = HardwareInfo::totalRamMiB();
    const int recommended = recommendedMiB(modCount, totalRam);
    const int current = m_instance->settings()->get("MaxMemAlloc").toInt();

    if (current < recommended) {
        emit logLine(tr("Memory tip: this instance has %1 mods but may only use %2 MiB of RAM. "
                        "%3 MiB is recommended. Change it in the instance's Settings page, on the Java tab.")
                         .arg(modCount)
                         .arg(current)
                         .arg(recommended),
                     MessageLevel::Warning);
    } else if (current > recommended * 2 && current > 8192) {
        emit logLine(tr("Memory tip: %1 MiB is a lot for %2 mods. Giving Java much more than it needs can cause lag spikes; "
                        "around %3 MiB is usually enough.")
                         .arg(current)
                         .arg(modCount)
                         .arg(recommended),
                     MessageLevel::Launcher);
    } else {
        emit logLine(tr("Memory: %1 MiB for %2 mods looks good.").arg(current).arg(modCount), MessageLevel::Launcher);
    }
    emitSucceeded();
}
