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

#include "HybridPage.h"

#include <QCheckBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

#include "Application.h"
#include "DesktopServices.h"
#include "FileSystem.h"
#include "minecraft/launch/SyncSharedConfig.h"
#include "settings/SettingsObject.h"

HybridPage::HybridPage(QWidget* parent) : QWidget(parent)
{
    setObjectName(QStringLiteral("hybridPage"));
    auto* layout = new QVBoxLayout(this);

    // World backups
    m_backupGroup = new QGroupBox(this);
    auto* backupLayout = new QVBoxLayout(m_backupGroup);
    m_backups = new QCheckBox(m_backupGroup);
    backupLayout->addWidget(m_backups);
    auto* keepRow = new QHBoxLayout();
    m_keepLabel = new QLabel(m_backupGroup);
    m_keep = new QSpinBox(m_backupGroup);
    m_keep->setRange(1, 100);
    keepRow->addWidget(m_keepLabel);
    keepRow->addWidget(m_keep);
    keepRow->addStretch();
    backupLayout->addLayout(keepRow);
    connect(m_backups, &QCheckBox::toggled, m_keep, &QWidget::setEnabled);
    layout->addWidget(m_backupGroup);

    // Launch helpers
    m_launchGroup = new QGroupBox(this);
    auto* launchLayout = new QVBoxLayout(m_launchGroup);
    m_crashExplainer = new QCheckBox(m_launchGroup);
    m_modChecker = new QCheckBox(m_launchGroup);
    m_memoryAdvice = new QCheckBox(m_launchGroup);
    launchLayout->addWidget(m_crashExplainer);
    launchLayout->addWidget(m_modChecker);
    launchLayout->addWidget(m_memoryAdvice);
    layout->addWidget(m_launchGroup);

    // Shared keybinds and servers
    m_syncGroup = new QGroupBox(this);
    auto* syncLayout = new QVBoxLayout(m_syncGroup);
    m_sync = new QCheckBox(m_syncGroup);
    m_syncInfo = new QLabel(m_syncGroup);
    m_syncInfo->setWordWrap(true);
    m_openShared = new QPushButton(m_syncGroup);
    connect(m_openShared, &QPushButton::clicked, this, [] {
        const auto dir = SyncSharedConfig::sharedDir();
        FS::ensureFolderPathExists(dir);
        DesktopServices::openPath(dir, true);
    });
    syncLayout->addWidget(m_sync);
    syncLayout->addWidget(m_syncInfo);
    syncLayout->addWidget(m_openShared, 0, Qt::AlignLeft);
    layout->addWidget(m_syncGroup);

    layout->addStretch();

    retranslate();
    loadSettings();
}

void HybridPage::loadSettings()
{
    auto s = APPLICATION->settings();
    m_backups->setChecked(s->get("HybridWorldBackups").toBool());
    m_keep->setValue(s->get("HybridWorldBackupsKeep").toInt());
    m_keep->setEnabled(m_backups->isChecked());
    m_crashExplainer->setChecked(s->get("HybridCrashExplainer").toBool());
    m_modChecker->setChecked(s->get("HybridModChecker").toBool());
    m_memoryAdvice->setChecked(s->get("HybridMemoryAdvice").toBool());
    m_sync->setChecked(s->get("HybridSyncSharedConfig").toBool());
}

bool HybridPage::apply()
{
    auto s = APPLICATION->settings();
    s->set("HybridWorldBackups", m_backups->isChecked());
    s->set("HybridWorldBackupsKeep", m_keep->value());
    s->set("HybridCrashExplainer", m_crashExplainer->isChecked());
    s->set("HybridModChecker", m_modChecker->isChecked());
    s->set("HybridMemoryAdvice", m_memoryAdvice->isChecked());
    s->set("HybridSyncSharedConfig", m_sync->isChecked());
    return true;
}

void HybridPage::retranslate()
{
    m_backupGroup->setTitle(tr("World backups"));
    m_backups->setText(tr("Back up changed worlds every time you launch"));
    m_keepLabel->setText(tr("Backups to keep per world:"));

    m_launchGroup->setTitle(tr("Launch helpers"));
    m_crashExplainer->setText(tr("Explain crashes in plain English at the end of the log"));
    m_modChecker->setText(tr("Check for duplicate, clashing or missing mods before launch"));
    m_memoryAdvice->setText(tr("Suggest a better RAM setting based on how many mods you have"));

    m_syncGroup->setTitle(tr("Shared keybinds and servers"));
    m_sync->setText(tr("Use the same keybinds and server list in every instance"));
    m_syncInfo->setText(tr("When a game closes, its keybinds and server list are saved. The next instance you launch gets them. "
                           "Video and sound settings stay separate for each instance."));
    m_openShared->setText(tr("Open shared settings folder"));
}
