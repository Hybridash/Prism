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

#include "SyncSharedConfig.h"

#include <QFile>
#include <QMap>
#include <QTextStream>

#include "Application.h"
#include "FileSystem.h"
#include "settings/SettingsObject.h"

namespace {

const QString optionsFile = "options.txt";
const QString serversFile = "servers.dat";

bool isKeybindLine(const QString& line)
{
    return line.startsWith("key_");
}

QStringList readLines(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }
    QStringList lines;
    QTextStream in(&file);
    while (!in.atEnd()) {
        lines << in.readLine();
    }
    return lines;
}

bool writeLines(const QString& path, const QStringList& lines)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        return false;
    }
    QTextStream out(&file);
    for (const auto& line : lines) {
        out << line << '\n';
    }
    return true;
}

QString keyOf(const QString& line)
{
    return line.section(':', 0, 0);
}

/// Replaces the keybind lines of @p target with those from @p source, keeping every other option.
bool mergeKeybinds(const QString& source, const QString& target, int* changed)
{
    QMap<QString, QString> keybinds;
    for (const auto& line : readLines(source)) {
        if (isKeybindLine(line)) {
            keybinds[keyOf(line)] = line;
        }
    }
    if (keybinds.isEmpty()) {
        return true;
    }

    QStringList result;
    for (const auto& line : readLines(target)) {
        if (isKeybindLine(line) && keybinds.contains(keyOf(line))) {
            const auto replacement = keybinds.take(keyOf(line));
            if (replacement != line) {
                (*changed)++;
            }
            result << replacement;
        } else {
            result << line;
        }
    }
    // keybinds the target didn't have yet (new instance, or a mod added new keys)
    for (const auto& line : keybinds) {
        result << line;
        (*changed)++;
    }
    return writeLines(target, result);
}

bool copyOver(const QString& from, const QString& to)
{
    if (!QFile::exists(from)) {
        return true;
    }
    QFile::remove(to);
    return QFile::copy(from, to);
}

}  // namespace

SyncSharedConfig::SyncSharedConfig(LaunchTask* parent, MinecraftInstance* instance, Direction direction)
    : LaunchStep(parent), m_instance(instance), m_direction(direction)
{}

QString SyncSharedConfig::sharedDir()
{
    return FS::PathCombine(APPLICATION->dataRoot(), "shared-config");
}

void SyncSharedConfig::executeTask()
{
    if (!APPLICATION->settings()->get("HybridSyncSharedConfig").toBool()) {
        emitSucceeded();
        return;
    }

    const QString shared = sharedDir();
    const QString game = m_instance->gameRoot();
    if (!FS::ensureFolderPathExists(shared) || !FS::ensureFolderPathExists(game)) {
        emit logLine(tr("Shared settings: could not create %1, skipping.").arg(shared), MessageLevel::Warning);
        emitSucceeded();
        return;
    }

    const QString sharedOptions = FS::PathCombine(shared, optionsFile);
    const QString gameOptions = FS::PathCombine(game, optionsFile);
    const QString sharedServers = FS::PathCombine(shared, serversFile);
    const QString gameServers = FS::PathCombine(game, serversFile);

    int changed = 0;
    bool ok = true;
    if (m_direction == Direction::Pull) {
        ok &= mergeKeybinds(sharedOptions, gameOptions, &changed);
        ok &= copyOver(sharedServers, gameServers);
        if (QFile::exists(sharedOptions) || QFile::exists(sharedServers)) {
            emit logLine(tr("Shared settings: applied %1 keybind change(s) and the shared server list.").arg(changed), MessageLevel::Launcher);
        } else {
            emit logLine(tr("Shared settings: nothing saved yet. Your keybinds and servers will be shared after this game closes."),
                         MessageLevel::Launcher);
        }
    } else {
        // Save only keybinds, so video settings etc. stay per instance. Merging (instead of overwriting)
        // keeps keybinds of mods that only other instances have.
        if (QFile::exists(gameOptions)) {
            ok &= mergeKeybinds(gameOptions, sharedOptions, &changed);
        }
        ok &= copyOver(gameServers, sharedServers);
        emit logLine(tr("Shared settings: saved your keybinds and server list for other instances."), MessageLevel::Launcher);
    }

    if (!ok) {
        emit logLine(tr("Shared settings: some files could not be copied."), MessageLevel::Warning);
    }
    emitSucceeded();
}
