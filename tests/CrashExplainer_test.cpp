// SPDX-License-Identifier: GPL-3.0-only
#include <QTest>

#include <hybrid/CrashExplainer.h>

class CrashExplainerTest : public QObject {
    Q_OBJECT

    static bool anyCauseContains(const QList<CrashExplainer::Finding>& findings, const QString& text)
    {
        for (const auto& f : findings) {
            if (f.cause.contains(text)) {
                return true;
            }
        }
        return false;
    }

   private slots:
    void test_fabricMissingDependency()
    {
        const QString log =
            "[main/ERROR]: Incompatible mods found!\n"
            "A potential solution has been determined:\n"
            "\t - Install fabric-api, any version.\n"
            "More details:\n"
            "\t - Mod 'Sodium Extra' (sodium-extra) 0.5.1 requires any version of fabric-api, which is missing!\n";
        const auto findings = CrashExplainer::analyze(log);
        QVERIFY(anyCauseContains(findings, "Install fabric-api"));
        QVERIFY(anyCauseContains(findings, "Sodium Extra"));
    }

    void test_forgeMissingDependency()
    {
        const QString log =
            "Missing or unsupported mandatory dependencies:\n"
            "\tMod ID: 'geckolib', Requested by: 'mowziesmobs', Expected range: '[4.4,)', Actual version: '[MISSING]'\n";
        const auto findings = CrashExplainer::analyze(log);
        QCOMPARE(findings.size(), 1);
        QVERIFY(findings.first().cause.contains("mowziesmobs"));
        QVERIFY(findings.first().cause.contains("geckolib"));
    }

    void test_wrongJava()
    {
        const QString log =
            "java.lang.UnsupportedClassVersionError: Main has been compiled by a more recent version of the Java Runtime "
            "(class file version 65.0), this version of the Java Runtime only recognizes class file versions up to 52.0\n";
        const auto findings = CrashExplainer::analyze(log);
        QVERIFY(anyCauseContains(findings, "Java 21"));
        QVERIFY(anyCauseContains(findings, "Java 8"));
    }

    void test_outOfMemory()
    {
        QVERIFY(anyCauseContains(CrashExplainer::analyze("java.lang.OutOfMemoryError: Java heap space\n"), "memory"));
    }

    void test_mixin()
    {
        QVERIFY(anyCauseContains(CrashExplainer::analyze("Mixin apply for mod create failed create.mixins.json:X\n"), "\"create\""));
    }

    void test_missingFabricApiClass()
    {
        const auto findings = CrashExplainer::analyze("java.lang.NoClassDefFoundError: net/fabricmc/fabric/api/event/Event\n");
        QCOMPARE(findings.size(), 1);
        QVERIFY(findings.first().fix.contains("Fabric API"));
    }

    void test_crashReportMod()
    {
        const QString log =
            "Suspected Mods: \n\tAlex's Mobs (alexsmobs), Version: 1.22\n"
            "Mod File: /home/u/.minecraft/mods/alexsmobs-1.22.jar\n";
        const auto findings = CrashExplainer::analyze(log);
        QVERIFY(anyCauseContains(findings, "Alex's Mobs"));
        QVERIFY(anyCauseContains(findings, "alexsmobs-1.22.jar"));
    }

    void test_nothingFound()
    {
        QVERIFY(CrashExplainer::analyze("Everything is fine\n").isEmpty());
        // launcher-side failures (no game output) should not get a crash explanation
        QVERIFY(CrashExplainer::explain("Checking Java\nJava not found\n").isEmpty());
    }
};

QTEST_GUILESS_MAIN(CrashExplainerTest)

#include "CrashExplainer_test.moc"
