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

#include "CrashExplainer.h"

#include <QCoreApplication>
#include <QRegularExpression>
#include <QSet>

namespace CrashExplainer {

static QString tr(const char* text)
{
    return QCoreApplication::translate("CrashExplainer", text);
}

static QString javaForClassVersion(int classVersion)
{
    // class file version 52 = Java 8, 53 = Java 9, ... 65 = Java 21
    if (classVersion >= 52) {
        return QString::number(classVersion - 44);
    }
    return QString();
}

QList<Finding> analyze(const QString& log)
{
    QList<Finding> findings;
    QSet<QString> seen;
    auto add = [&findings, &seen](const QString& cause, const QString& fix) {
        if (!seen.contains(cause)) {
            seen.insert(cause);
            findings.append({ cause, fix });
        }
    };

    // --- Fabric / Quilt: dependency resolution failed. Their messages are already readable, so we quote them.
    if (log.contains("Incompatible mods found!") || log.contains("Mod resolution failed") ||
        log.contains("Some of your mods are incompatible")) {
        static const QRegularExpression fabricLine(R"(^\s*- (?:Mod|Install|Replace|Remove) .+$)", QRegularExpression::MultilineOption);
        auto it = fabricLine.globalMatch(log);
        int count = 0;
        while (it.hasNext() && count < 6) {
            add(it.next().captured(0).trimmed().mid(2), tr("Install, update or remove the mods named above, then launch again."));
            count++;
        }
        if (count == 0) {
            add(tr("The mod loader found mods that need other mods (or other versions) to work."),
                tr("Scroll up to the \"Incompatible mods found\" section to see which mods, then install or update them."));
        }
    }

    // --- Forge / NeoForge: missing mandatory dependencies
    {
        static const QRegularExpression forgeDep(
            R"(Mod ID: '([^']+)', Requested by: '([^']+)', Expected range: '([^']*)', Actual version: '([^']*)')");
        auto it = forgeDep.globalMatch(log);
        while (it.hasNext()) {
            const auto m = it.next();
            const auto actual = m.captured(4);
            if (actual.contains("MISSING", Qt::CaseInsensitive)) {
                add(tr("%1 needs the mod \"%2\", which is not installed.").arg(m.captured(2), m.captured(1)),
                    tr("Download %1 (version %2) for this Minecraft version and loader, and put it in the mods folder.")
                        .arg(m.captured(1), m.captured(3)));
            } else {
                add(tr("%1 needs %2 version %3, but you have %4.").arg(m.captured(2), m.captured(1), m.captured(3), actual),
                    tr("Update or change the version of %1.").arg(m.captured(1)));
            }
        }
    }

    // --- Duplicate mods
    if (log.contains("Found duplicate mods", Qt::CaseInsensitive) || log.contains("Duplicate mods found", Qt::CaseInsensitive) ||
        log.contains("DuplicateModsFoundException")) {
        add(tr("The same mod is installed more than once."), tr("Open the Mods page, find the doubled mod and remove the older copy."));
    }

    // --- Wrong Java version
    {
        static const QRegularExpression classVersion(R"(class file version (\d+)(?:\.\d+)?\), this version of the Java Runtime only recognizes class file versions up to (\d+))");
        const auto m = classVersion.match(log);
        if (m.hasMatch()) {
            const auto needed = javaForClassVersion(m.captured(1).toInt());
            const auto have = javaForClassVersion(m.captured(2).toInt());
            add(tr("Something needs Java %1, but the game was started with Java %2.").arg(needed, have),
                tr("In the instance's Settings, on the Java tab, pick Java %1 or newer (or turn on automatic Java).").arg(needed));
        } else if (log.contains("java.lang.UnsupportedClassVersionError")) {
            add(tr("A mod or the game needs a newer version of Java."), tr("In the instance's Settings, on the Java tab, pick a newer Java."));
        }
    }

    // --- Memory
    if (log.contains("java.lang.OutOfMemoryError")) {
        add(tr("Minecraft ran out of memory (RAM)."),
            tr("Give the instance more memory in its Settings (Java tab). 4096 MiB is a good start for modpacks; 6144+ for big ones."));
    }
    if (log.contains("Could not reserve enough space for") || log.contains("Invalid maximum heap size") ||
        log.contains("Could not create the Java Virtual Machine")) {
        add(tr("Java could not start with the memory settings you picked."),
            tr("Lower the maximum memory in the instance's Settings (Java tab), and make sure you use 64-bit Java."));
    }

    // --- Graphics drivers
    if (log.contains("Pixel format not accelerated") || log.contains("GLFW error 65542") ||
        log.contains("does not appear to support OpenGL") || log.contains("No OpenGL context found")) {
        add(tr("Your graphics driver couldn't start OpenGL."),
            tr("Update your graphics driver from the NVIDIA, AMD or Intel website (not Windows Update)."));
    }

    // --- Mixins: a mod failed to patch the game
    {
        static const QRegularExpression mixinMod(R"(Mixin apply for mod ([\w\-]+) failed)");
        static const QRegularExpression mixinConfig(R"(Mixin \[([\w\-\.]+)\.mixins\.json)");
        auto it = mixinMod.globalMatch(log);
        bool any = false;
        while (it.hasNext()) {
            const auto mod = it.next().captured(1);
            add(tr("The mod \"%1\" failed to patch the game (mixin error).").arg(mod),
                tr("%1 is probably not made for this Minecraft version, or clashes with another mod. Update it or remove it.").arg(mod));
            any = true;
        }
        if (!any && (log.contains("MixinApplyError") || log.contains("InvalidInjectionException") || log.contains("MixinTransformerError"))) {
            const auto m = mixinConfig.match(log);
            const auto who = m.hasMatch() ? m.captured(1) : tr("A mod");
            add(tr("%1 failed to patch the game (mixin error).").arg(who),
                tr("Update the mod, or remove mods one at a time to find the one that clashes."));
        }
    }

    // --- Missing classes: usually a missing library mod, or a mod for the wrong loader/version
    {
        static const QRegularExpression missingClass(R"((?:NoClassDefFoundError|ClassNotFoundException): ([\w\.\$/]+))");
        const auto m = missingClass.match(log);
        if (m.hasMatch()) {
            const auto cls = m.captured(1).replace('/', '.');
            QString hint;
            if (cls.startsWith("net.fabricmc.fabric")) {
                hint = tr("This usually means Fabric API is missing. Install Fabric API.");
            } else if (cls.startsWith("me.shedaniel.clothconfig") || cls.startsWith("me.shedaniel.autoconfig")) {
                hint = tr("This usually means Cloth Config is missing. Install Cloth Config.");
            } else if (cls.startsWith("software.bernie.geckolib")) {
                hint = tr("This usually means GeckoLib is missing. Install GeckoLib.");
            } else if (cls.startsWith("dev.architectury")) {
                hint = tr("This usually means Architectury API is missing. Install Architectury API.");
            } else if (cls.startsWith("net.minecraftforge") || cls.startsWith("net.neoforged")) {
                hint = tr("A Forge/NeoForge mod was loaded by a different loader. Check that every mod is for this instance's loader.");
            } else {
                hint = tr("A required library mod is probably missing, or a mod is for a different Minecraft version or loader.");
            }
            add(tr("The game couldn't find the code \"%1\".").arg(cls), hint);
        }
    }

    // --- Forge crash reports name the mod file
    {
        static const QRegularExpression suspected(R"(Suspected Mods?:\s*\n\s*([^\n]+))");
        const auto m = suspected.match(log);
        if (m.hasMatch() && !m.captured(1).contains("NONE", Qt::CaseInsensitive) && !m.captured(1).contains("Unknown", Qt::CaseInsensitive)) {
            add(tr("The crash report suspects: %1").arg(m.captured(1).trimmed()),
                tr("Try updating or removing that mod first."));
        }
        static const QRegularExpression modFile(R"(Mod File: (?:[^\n]*[/\\])?([^/\\\n]+\.jar))");
        const auto f = modFile.match(log);
        if (f.hasMatch()) {
            add(tr("The crash happened inside %1.").arg(f.captured(1)), tr("Try updating or removing that mod first."));
        }
    }

    return findings;
}

QStringList explain(const QString& log)
{
    QStringList lines;
    const auto findings = analyze(log);

    static const QRegularExpression crashReport(R"(Crash report saved to:?\s*(?:#@!@#\s*)?(\S[^\n]*))");
    const auto report = crashReport.match(log);

    if (findings.isEmpty()) {
        const bool gameRan = log.contains("Exception") || log.contains("Crash") || log.contains("Error");
        if (!gameRan) {
            return lines;
        }
        lines << tr("==== What went wrong? (Hybrid Launcher) ====");
        lines << tr("No known cause was found automatically.");
        lines << tr("Tip: look for the first line starting with \"Caused by:\" above; the mod name is often in it.");
        lines << tr("Tip: use the Upload button to share this log when asking for help.");
    } else {
        lines << tr("==== What went wrong? (Hybrid Launcher) ====");
        for (int i = 0; i < findings.size(); i++) {
            lines << QString("%1. %2").arg(i + 1).arg(findings[i].cause);
            // several findings often share one fix (e.g. a list of missing mods); say it once, after the last of them
            const bool sameAsNext = i + 1 < findings.size() && findings[i + 1].fix == findings[i].fix;
            if (!sameAsNext) {
                lines << tr("   Fix: %1").arg(findings[i].fix);
            }
        }
    }
    if (report.hasMatch()) {
        lines << tr("Full crash report: %1").arg(report.captured(1).trimmed());
    }
    lines << QStringLiteral("============================================");
    return lines;
}

}  // namespace CrashExplainer
