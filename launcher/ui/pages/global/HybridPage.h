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

#include <QWidget>

#include "ui/pages/BasePage.h"

class QCheckBox;
class QSpinBox;
class QLabel;
class QPushButton;
class QGroupBox;

/// Hybrid Launcher: one settings page for all the features this fork adds.
class HybridPage : public QWidget, public BasePage {
    Q_OBJECT

   public:
    explicit HybridPage(QWidget* parent = nullptr);
    ~HybridPage() override = default;

    QString displayName() const override { return tr("Hybrid Extras"); }
    QIcon icon() const override { return QIcon::fromTheme("launch"); }
    QString id() const override { return "hybrid-settings"; }
    QString helpPage() const override { return {}; }
    bool apply() override;
    void retranslate() override;

   private:
    void loadSettings();

    QGroupBox* m_backupGroup;
    QCheckBox* m_backups;
    QLabel* m_keepLabel;
    QSpinBox* m_keep;

    QGroupBox* m_launchGroup;
    QCheckBox* m_crashExplainer;
    QCheckBox* m_modChecker;
    QCheckBox* m_memoryAdvice;

    QGroupBox* m_syncGroup;
    QCheckBox* m_sync;
    QLabel* m_syncInfo;
    QPushButton* m_openShared;
};
