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

#include "CheckModConflicts.h"

#include <QMap>
#include <QSet>

#include "Application.h"
#include "minecraft/mod/Mod.h"
#include "minecraft/mod/ModFolderModel.h"
#include "settings/SettingsObject.h"

namespace {

struct KnownConflict {
    QStringList a;  // mod ids or file name fragments for the first mod
    QStringList b;  // ... and for the second
    const char* reason;
};

// Pairs that are well known to crash or break each other. Keep this list short and certain:
// a false alarm here teaches players to ignore the warning.
const QList<KnownConflict>& knownConflicts()
{
    static const QList<KnownConflict> list = {
        { { "optifine", "optifabric" }, { "sodium" }, QT_TRANSLATE_NOOP("CheckModConflicts", "both replace the rendering engine") },
        { { "optifine", "optifabric" }, { "embeddium" }, QT_TRANSLATE_NOOP("CheckModConflicts", "both replace the rendering engine") },
        { { "optifine", "optifabric" }, { "rubidium" }, QT_TRANSLATE_NOOP("CheckModConflicts", "both replace the rendering engine") },
        { { "optifine", "optifabric" }, { "iris" }, QT_TRANSLATE_NOOP("CheckModConflicts", "Iris is a replacement for OptiFine shaders") },
        { { "optifine", "optifabric" }, { "oculus" }, QT_TRANSLATE_NOOP("CheckModConflicts", "Oculus is a replacement for OptiFine shaders") },
        { { "sodium" }, { "embeddium" }, QT_TRANSLATE_NOOP("CheckModConflicts", "Embeddium is a fork of Sodium; use only one") },
        { { "sodium" }, { "rubidium" }, QT_TRANSLATE_NOOP("CheckModConflicts", "Rubidium is a fork of Sodium; use only one") },
        { { "embeddium" }, { "rubidium" }, QT_TRANSLATE_NOOP("CheckModConflicts", "Embeddium replaces Rubidium; use only one") },
        { { "canvas" }, { "sodium" }, QT_TRANSLATE_NOOP("CheckModConflicts", "both replace the rendering engine") },
    };
    return list;
}

// Dependencies that are provided by the loader, the game or Java itself
const QSet<QString>& builtInIds()
{
    static const QSet<QString> ids = { "minecraft", "java", "forge", "neoforge", "fabricloader", "fabric", "quilt_loader", "quilt_base",
                                       "mixinextras" };
    return ids;
}

QString displayName(const Mod* mod)
{
    const auto name = mod->name();
    return name.isEmpty() ? mod->fileinfo().fileName() : name;
}

bool matches(const Mod* mod, const QStringList& needles)
{
    const auto id = mod->modId().toLower();
    const auto file = mod->fileinfo().fileName().toLower();
    for (const auto& needle : needles) {
        if (id == needle || (id.isEmpty() && file.contains(needle)) || (needle == "optifine" && file.contains("optifine"))) {
            return true;
        }
    }
    return false;
}

}  // namespace

CheckModConflicts::CheckModConflicts(LaunchTask* parent, MinecraftInstance* instance) : LaunchStep(parent), m_instance(instance)
{
    m_timeout.setSingleShot(true);
    m_timeout.setInterval(10000);
    connect(&m_timeout, &QTimer::timeout, this, &CheckModConflicts::runChecks);
}

void CheckModConflicts::executeTask()
{
    if (!APPLICATION->settings()->get("HybridModChecker").toBool()) {
        emitSucceeded();
        return;
    }

    auto* mods = m_instance->loaderModList();
    if (mods && mods->hasPendingParseTasks()) {
        // mod details (ids, dependencies) are still being read from the jars; wait a little
        connect(mods, &ModFolderModel::parseFinished, this, &CheckModConflicts::runChecks, Qt::SingleShotConnection);
        m_timeout.start();
        return;
    }
    runChecks();
}

void CheckModConflicts::runChecks()
{
    if (m_done) {
        return;
    }
    m_done = true;
    m_timeout.stop();

    auto* model = m_instance->loaderModList();
    if (!model) {
        emitSucceeded();
        return;
    }

    QList<Mod*> mods;
    for (auto* mod : model->allMods()) {
        if (mod->enabled()) {
            mods << mod;
        }
    }
    if (mods.isEmpty()) {
        emitSucceeded();
        return;
    }

    QStringList problems;

    // 1. the same mod twice
    QMap<QString, QList<Mod*>> byId;
    for (auto* mod : mods) {
        if (!mod->modId().isEmpty()) {
            byId[mod->modId().toLower()] << mod;
        }
    }
    for (auto it = byId.cbegin(); it != byId.cend(); ++it) {
        if (it.value().size() > 1) {
            QStringList files;
            for (auto* mod : it.value()) {
                files << mod->fileinfo().fileName();
            }
            problems << tr("%1 is installed %2 times (%3). Remove all but one.")
                            .arg(displayName(it.value().first()))
                            .arg(it.value().size())
                            .arg(files.join(", "));
        }
    }

    // 2. known conflicts
    for (const auto& conflict : knownConflicts()) {
        const Mod* first = nullptr;
        const Mod* second = nullptr;
        for (auto* mod : mods) {
            if (!first && matches(mod, conflict.a)) {
                first = mod;
            } else if (!second && matches(mod, conflict.b)) {
                second = mod;
            }
        }
        if (first && second) {
            problems << tr("%1 and %2 don't work together: %3.")
                            .arg(displayName(first), displayName(second), tr(conflict.reason));
        }
    }

    // 3. missing dependencies
    QSet<QString> installed;
    for (auto* mod : mods) {
        installed << mod->modId().toLower();
    }
    for (auto* mod : mods) {
        QStringList missing;
        for (const auto& dep : mod->dependencies()) {
            const auto id = dep.toLower();
            if (!installed.contains(id) && !builtInIds().contains(id)) {
                missing << dep;
            }
        }
        if (!missing.isEmpty()) {
            problems << tr("%1 needs %2, which doesn't seem to be installed (or is disabled).")
                            .arg(displayName(mod), missing.join(", "));
        }
    }

    if (problems.isEmpty()) {
        emit logLine(tr("Mod check: no problems found in %1 mods.").arg(mods.size()), MessageLevel::Launcher);
    } else {
        emit logLine(tr("Mod check found %1 possible problem(s):").arg(problems.size()), MessageLevel::Warning);
        for (const auto& problem : problems) {
            emit logLine("  - " + problem, MessageLevel::Warning);
        }
    }
    emitSucceeded();
}
