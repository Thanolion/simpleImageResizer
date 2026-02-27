// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2024-2026 thanolion

#pragma once

#include <atomic>

#include <QMainWindow>
#include <QTableWidget>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QSlider>
#include <QSpinBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QFutureWatcher>
#include <QSplitter>
#include <QButtonGroup>
#include <QPointer>
#include <QTabWidget>
#include <QComboBox>
#include <QThreadPool>
#include <QThread>

#include "ProcessingJob.h"
#include "ProcessingResult.h"

class FormatGuideDialog;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    void onAddFiles();
    void onAddFolder();
    void onRemoveSelected();
    void onClearAll();
    void onBrowseOutput();
    void onProcess();
    void onCancel();
    void onProcessingFinished();
    void onCopyResults();
    void onOpenOutputFolder();
    void onAbout();
    void onDonate();
    void onResizeModeChanged();
    void onTargetSizeToggled(bool checked);
    void onFormatChanged(int formatId);
    void onFormatGuide();

private:
    void setupMenuBar();
    void setupUI();
    void setupSimpleTab(QTabWidget *tabWidget);
    void setupAdvancedTab(QTabWidget *tabWidget);
    void syncSimpleToAdvanced();
    void syncAdvancedToSimple();
    void addImageFiles(const QStringList &paths);
    void loadSettings();
    void saveSettings();
    void updateResizeControls();

    // Tab widget
    QTabWidget *m_tabWidget = nullptr;

    // Simple tab widgets
    QComboBox   *m_simpleFormatCombo = nullptr;
    QComboBox   *m_simpleResizeCombo = nullptr;
    QSlider     *m_simpleResizeSlider = nullptr;
    QLabel      *m_simpleResizeLabel = nullptr;
    QComboBox   *m_simpleQualityCombo = nullptr;
    QLabel      *m_simpleQualityDesc = nullptr;
    QLineEdit   *m_simpleOutputDirEdit = nullptr;
    QPushButton *m_simpleBrowseOutputBtn = nullptr;

    // Input panel
    QTableWidget *m_inputTable = nullptr;
    QPushButton *m_addFilesBtn = nullptr;
    QPushButton *m_addFolderBtn = nullptr;
    QPushButton *m_removeSelectedBtn = nullptr;
    QPushButton *m_clearAllBtn = nullptr;

    // Output settings
    QLineEdit *m_outputDirEdit = nullptr;
    QPushButton *m_browseOutputBtn = nullptr;
    QRadioButton *m_fmtJpg = nullptr;
    QRadioButton *m_fmtPng = nullptr;
    QRadioButton *m_fmtWebp = nullptr;
    QRadioButton *m_fmtAvif = nullptr;
    QButtonGroup *m_fmtGroup = nullptr;

    // Processing options
    QRadioButton *m_modePercent = nullptr;
    QRadioButton *m_modeFitWidth = nullptr;
    QRadioButton *m_modeFitHeight = nullptr;
    QRadioButton *m_modeFitBox = nullptr;
    QRadioButton *m_modeNoResize = nullptr;
    QButtonGroup *m_modeGroup = nullptr;
    QSlider *m_resizeSlider = nullptr;
    QLabel *m_resizeLabel = nullptr;
    QSpinBox *m_widthSpin = nullptr;
    QSpinBox *m_heightSpin = nullptr;
    QSlider *m_qualitySlider = nullptr;
    QLabel *m_qualityLabel = nullptr;
    QLabel *m_qualityTextLabel = nullptr;
    QLabel *m_pngInfoLabel = nullptr;
    QCheckBox *m_targetSizeCheck = nullptr;
    QSpinBox *m_targetSizeSpin = nullptr;

    // Advanced tab - Performance
    QSpinBox    *m_threadCountSpin = nullptr;

    // Dedicated thread pool
    QThreadPool *m_threadPool = nullptr;

    // Process controls
    QPushButton *m_processBtn = nullptr;
    QPushButton *m_cancelBtn = nullptr;
    QProgressBar *m_progressBar = nullptr;
    QLabel *m_statusLabel = nullptr;

    // Results panel
    QTableWidget *m_resultsTable = nullptr;
    QPushButton *m_copyResultsBtn = nullptr;
    QPushButton *m_openOutputBtn = nullptr;

    // Format Guide
    QPointer<FormatGuideDialog> m_formatGuideDialog;

    // Processing state
    QFutureWatcher<ProcessingResult> *m_watcher = nullptr;
    std::atomic<bool> m_cancelled{false};
    bool m_usePerFileOutput = false;
};
