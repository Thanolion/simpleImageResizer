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

#include "ProcessingJob.h"
#include "ProcessingResult.h"

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
    void addImageFiles(const QStringList &paths);
    void loadSettings();
    void saveSettings();
    void updateResizeControls();

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
    QPointer<QDialog> m_formatGuideDialog;

    // Processing state
    QFutureWatcher<ProcessingResult> *m_watcher = nullptr;
    std::atomic<bool> m_cancelled{false};
    bool m_usePerFileOutput = false;
};
