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

#include <QString>
#include <QStringList>

/**
 * Hybrid Launcher: reads a game log after a crash and explains the most likely cause in plain English.
 *
 * It only knows common, recognizable failures (missing mods, wrong Java, out of memory, broken mixins, ...).
 * Every rule is a simple text match, so it can be unit tested with pasted logs.
 */
namespace CrashExplainer {

struct Finding {
    QString cause;  // what went wrong, e.g. "Sodium needs Fabric API, which is missing."
    QString fix;    // what to do about it
};

/// All causes found in the log, most specific first. Empty if nothing recognizable was found.
QList<Finding> analyze(const QString& log);

/// Ready-to-print lines for the launcher log (header, findings, fallback advice).
QStringList explain(const QString& log);

}  // namespace CrashExplainer
