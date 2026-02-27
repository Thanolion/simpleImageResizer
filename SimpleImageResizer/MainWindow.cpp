// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2024-2026 thanolion

#include "MainWindow.h"
#include "FormatGuideDialog.h"
#include "ImageProcessor.h"
#include "SettingsManager.h"

#include <QApplication>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QClipboard>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QHeaderView>
#include <QDesktopServices>
#include <QUrl>
#include <QCloseEvent>
#include <QtConcurrent>
#include <QDir>
#include <QFileInfo>
#include <QSet>
#include <QDirIterator>
#include <QImageReader>
#include <QSignalBlocker>

static const QStringList IMAGE_FILTERS = {
    "*.png", "*.jpg", "*.jpeg", "*.bmp", "*.gif", "*.tiff", "*.tif", "*.webp", "*.avif",
    "*.cr2", "*.cr3", "*.nef", "*.nrw", "*.arw", "*.dng", "*.raf", "*.orf", "*.rw2", "*.pef", "*.srw"
};

static QString buildDialogFilter() {
    return "Images (" + IMAGE_FILTERS.join(' ') + ");;All Files (*)";
}

static QStringList bareExtensions() {
    QStringList exts;
    for (const QString &f : IMAGE_FILTERS) exts << f.mid(2); // "*.png" -> "png"
    return exts;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Simple Image Resizer");
    resize(1000, 700);
    setAcceptDrops(true);

    setupMenuBar();
    setupUI();
    m_threadPool = new QThreadPool(this);
    loadSettings();
    syncAdvancedToSimple();
}

MainWindow::~MainWindow()
{
    // Safety net — closeEvent should have already handled this
    if (m_watcher && m_watcher->isRunning()) {
        m_cancelled = true;
        m_watcher->disconnect();
        m_watcher->cancel();
        m_watcher->waitForFinished();
    }
}

void MainWindow::setupMenuBar()
{
    auto *fileMenu = menuBar()->addMenu("&File");
    auto *exitAction = fileMenu->addAction("E&xit");
    connect(exitAction, &QAction::triggered, this, &QWidget::close);

    auto *helpMenu = menuBar()->addMenu("&Help");
    auto *formatGuideAction = helpMenu->addAction("Image &Format Guide...");
    connect(formatGuideAction, &QAction::triggered, this, &MainWindow::onFormatGuide);
    helpMenu->addSeparator();
    auto *aboutAction = helpMenu->addAction("&About");
    connect(aboutAction, &QAction::triggered, this, &MainWindow::onAbout);

    auto *donateAction = helpMenu->addAction("Support &Development");
    connect(donateAction, &QAction::triggered, this, &MainWindow::onDonate);
}

void MainWindow::setupUI()
{
    auto *splitter = new QSplitter(Qt::Horizontal, this);
    setCentralWidget(splitter);

    // ── Left panel: Input + Settings ──
    auto *leftWidget = new QWidget;
    auto *leftLayout = new QVBoxLayout(leftWidget);

    // Input file management
    auto *inputGroup = new QGroupBox("Input Files");
    auto *inputLayout = new QVBoxLayout(inputGroup);

    m_inputTable = new QTableWidget(0, 3);
    m_inputTable->setHorizontalHeaderLabels({"File Name", "Size", "Dimensions"});
    m_inputTable->horizontalHeader()->setStretchLastSection(true);
    m_inputTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_inputTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    inputLayout->addWidget(m_inputTable);

    auto *inputBtnLayout = new QHBoxLayout;
    m_addFilesBtn = new QPushButton("Add Files...");
    m_addFolderBtn = new QPushButton("Add Folder...");
    m_removeSelectedBtn = new QPushButton("Remove Selected");
    m_clearAllBtn = new QPushButton("Clear All");
    inputBtnLayout->addWidget(m_addFilesBtn);
    inputBtnLayout->addWidget(m_addFolderBtn);
    inputBtnLayout->addWidget(m_removeSelectedBtn);
    inputBtnLayout->addWidget(m_clearAllBtn);
    inputLayout->addLayout(inputBtnLayout);
    leftLayout->addWidget(inputGroup);

    // Tabbed settings (Simple / Advanced)
    m_tabWidget = new QTabWidget;
    setupSimpleTab(m_tabWidget);
    setupAdvancedTab(m_tabWidget);
    leftLayout->addWidget(m_tabWidget);

    // Process controls
    auto *processLayout = new QHBoxLayout;
    m_processBtn = new QPushButton("Process");
    m_cancelBtn = new QPushButton("Cancel");
    m_cancelBtn->setEnabled(false);
    processLayout->addWidget(m_processBtn);
    processLayout->addWidget(m_cancelBtn);
    leftLayout->addLayout(processLayout);

    m_progressBar = new QProgressBar;
    m_progressBar->setValue(0);
    leftLayout->addWidget(m_progressBar);

    m_statusLabel = new QLabel("Ready");
    leftLayout->addWidget(m_statusLabel);

    // ── Right panel: Results ──
    auto *rightWidget = new QWidget;
    auto *rightLayout = new QVBoxLayout(rightWidget);

    auto *resultsGroup = new QGroupBox("Results");
    auto *resultsLayout = new QVBoxLayout(resultsGroup);

    m_resultsTable = new QTableWidget(0, 5);
    m_resultsTable->setHorizontalHeaderLabels({
        "File Name", "Original Size", "New Size", "Reduction %", "Status"
    });
    m_resultsTable->horizontalHeader()->setStretchLastSection(true);
    m_resultsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_resultsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    resultsLayout->addWidget(m_resultsTable);

    auto *resultsBtnLayout = new QHBoxLayout;
    m_copyResultsBtn = new QPushButton("Copy to Clipboard");
    m_openOutputBtn = new QPushButton("Open Output Folder");
    resultsBtnLayout->addWidget(m_copyResultsBtn);
    resultsBtnLayout->addWidget(m_openOutputBtn);
    resultsBtnLayout->addStretch();
    resultsLayout->addLayout(resultsBtnLayout);

    rightLayout->addWidget(resultsGroup);

    splitter->addWidget(leftWidget);
    splitter->addWidget(rightWidget);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);

    // Connections
    connect(m_addFilesBtn, &QPushButton::clicked, this, &MainWindow::onAddFiles);
    connect(m_addFolderBtn, &QPushButton::clicked, this, &MainWindow::onAddFolder);
    connect(m_removeSelectedBtn, &QPushButton::clicked, this, &MainWindow::onRemoveSelected);
    connect(m_clearAllBtn, &QPushButton::clicked, this, &MainWindow::onClearAll);
    connect(m_processBtn, &QPushButton::clicked, this, &MainWindow::onProcess);
    connect(m_cancelBtn, &QPushButton::clicked, this, &MainWindow::onCancel);
    connect(m_copyResultsBtn, &QPushButton::clicked, this, &MainWindow::onCopyResults);
    connect(m_openOutputBtn, &QPushButton::clicked, this, &MainWindow::onOpenOutputFolder);

    connect(m_tabWidget, &QTabWidget::currentChanged, this, [this](int index) {
        if (index == 0) syncAdvancedToSimple();
        else            syncSimpleToAdvanced();
    });

    updateResizeControls();
}

void MainWindow::setupSimpleTab(QTabWidget *tabWidget)
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setSpacing(12);  // More spacious for casual users

    // ── Output ──
    auto *outputGroup = new QGroupBox("Output");
    auto *outputLayout = new QVBoxLayout(outputGroup);

    auto *outputDirLayout = new QHBoxLayout;
    outputDirLayout->addWidget(new QLabel("Output Folder:"));
    m_simpleOutputDirEdit = new QLineEdit;
    m_simpleOutputDirEdit->setPlaceholderText("Leave blank to save next to originals");
    outputDirLayout->addWidget(m_simpleOutputDirEdit);
    m_simpleBrowseOutputBtn = new QPushButton("Browse...");
    outputDirLayout->addWidget(m_simpleBrowseOutputBtn);
    outputLayout->addLayout(outputDirLayout);

    auto *fmtLayout = new QHBoxLayout;
    fmtLayout->addWidget(new QLabel("Format:"));
    m_simpleFormatCombo = new QComboBox;
    m_simpleFormatCombo->addItem("JPG - Best compatibility", 0);
    m_simpleFormatCombo->addItem("PNG - Lossless quality", 1);
    m_simpleFormatCombo->addItem("WebP - Smaller than JPG", 2);
    m_simpleFormatCombo->addItem("AVIF - Smallest files", 3);
    m_simpleFormatCombo->setMinimumWidth(200);
    m_simpleFormatCombo->setToolTip("Choose the output image format");
    fmtLayout->addWidget(m_simpleFormatCombo);
    fmtLayout->addStretch();
    outputLayout->addLayout(fmtLayout);
    layout->addWidget(outputGroup);

    // ── Resize & Quality ──
    auto *rqGroup = new QGroupBox("Resize && Quality");
    auto *rqLayout = new QVBoxLayout(rqGroup);

    // Resize presets
    auto *resizeLayout = new QHBoxLayout;
    resizeLayout->addWidget(new QLabel("Resize:"));
    m_simpleResizeCombo = new QComboBox;
    m_simpleResizeCombo->addItem("Original Size (no resize)", -1);  // -1 = NoResize
    m_simpleResizeCombo->addItem("75% of original", 75);
    m_simpleResizeCombo->addItem("50% of original", 50);
    m_simpleResizeCombo->addItem("25% of original", 25);
    m_simpleResizeCombo->addItem("Custom...", 0);  // 0 = show slider
    m_simpleResizeCombo->setMinimumWidth(200);
    m_simpleResizeCombo->setToolTip("Choose how much to resize images");
    resizeLayout->addWidget(m_simpleResizeCombo);
    resizeLayout->addStretch();
    rqLayout->addLayout(resizeLayout);

    // Custom resize slider (hidden by default)
    auto *customResizeLayout = new QHBoxLayout;
    customResizeLayout->addSpacing(20);  // indent
    m_simpleResizeSlider = new QSlider(Qt::Horizontal);
    m_simpleResizeSlider->setRange(1, 200);
    m_simpleResizeSlider->setValue(100);
    m_simpleResizeLabel = new QLabel("100%");
    m_simpleResizeLabel->setMinimumWidth(45);
    customResizeLayout->addWidget(m_simpleResizeSlider);
    customResizeLayout->addWidget(m_simpleResizeLabel);
    rqLayout->addLayout(customResizeLayout);
    m_simpleResizeSlider->setVisible(false);
    m_simpleResizeLabel->setVisible(false);

    // Quality presets
    auto *qualityLayout = new QHBoxLayout;
    qualityLayout->addWidget(new QLabel("Quality:"));
    m_simpleQualityCombo = new QComboBox;
    m_simpleQualityCombo->addItem("Low (smaller files)", 40);
    m_simpleQualityCombo->addItem("Medium", 65);
    m_simpleQualityCombo->addItem("High (recommended)", 85);
    m_simpleQualityCombo->addItem("Maximum", 100);
    m_simpleQualityCombo->setCurrentIndex(2);  // Default to "High"
    m_simpleQualityCombo->setMinimumWidth(200);
    m_simpleQualityCombo->setToolTip("Balance between image quality and file size");
    qualityLayout->addWidget(m_simpleQualityCombo);
    qualityLayout->addStretch();
    rqLayout->addLayout(qualityLayout);

    // Quality description label
    m_simpleQualityDesc = new QLabel("Recommended for most uses \u2014 minimal quality loss");
    m_simpleQualityDesc->setStyleSheet("QLabel { color: #666; font-style: italic; padding: 2px 0 2px 20px; }");
    m_simpleQualityDesc->setWordWrap(true);
    rqLayout->addWidget(m_simpleQualityDesc);

    layout->addWidget(rqGroup);
    layout->addStretch();

    tabWidget->addTab(page, "Simple");

    // ── Simple tab connections ──
    connect(m_simpleBrowseOutputBtn, &QPushButton::clicked, this, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, "Select Output Folder",
                          m_simpleOutputDirEdit->text());
        if (!dir.isEmpty())
            m_simpleOutputDirEdit->setText(dir);
    });

    connect(m_simpleResizeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
        bool isCustom = (index == 4);  // "Custom..." item
        m_simpleResizeSlider->setVisible(isCustom);
        m_simpleResizeLabel->setVisible(isCustom);
    });

    connect(m_simpleResizeSlider, &QSlider::valueChanged, this, [this](int val) {
        m_simpleResizeLabel->setText(QString::number(val) + "%");
    });

    connect(m_simpleQualityCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
        static const char *descriptions[] = {
            "Smallest file size, some visible quality loss",
            "Good balance of quality and file size",
            "Recommended for most uses \u2014 minimal quality loss",
            "Largest files, highest quality"
        };
        if (index >= 0 && index < 4)
            m_simpleQualityDesc->setText(descriptions[index]);
    });

    connect(m_simpleFormatCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
        bool isPng = (index == 1);
        m_simpleQualityCombo->setEnabled(!isPng);
        m_simpleQualityDesc->setEnabled(!isPng);
        if (isPng)
            m_simpleQualityDesc->setText("PNG uses lossless compression \u2014 quality does not apply");
    });
}

void MainWindow::setupAdvancedTab(QTabWidget *tabWidget)
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);

    // ── Output Settings ──
    auto *outputGroup = new QGroupBox("Output Settings");
    auto *outputLayout = new QVBoxLayout(outputGroup);

    auto *outputDirLayout = new QHBoxLayout;
    outputDirLayout->addWidget(new QLabel("Output Folder:"));
    m_outputDirEdit = new QLineEdit;
    m_outputDirEdit->setPlaceholderText("Leave blank to save next to originals");
    outputDirLayout->addWidget(m_outputDirEdit);
    m_browseOutputBtn = new QPushButton("Browse...");
    outputDirLayout->addWidget(m_browseOutputBtn);
    outputLayout->addLayout(outputDirLayout);

    auto *fmtLayout = new QHBoxLayout;
    fmtLayout->addWidget(new QLabel("Format:"));
    m_fmtJpg = new QRadioButton("JPG");
    m_fmtPng = new QRadioButton("PNG");
    m_fmtWebp = new QRadioButton("WebP");
    m_fmtAvif = new QRadioButton("AVIF");
    m_fmtJpg->setChecked(true);
    m_fmtGroup = new QButtonGroup(this);
    m_fmtGroup->addButton(m_fmtJpg, 0);
    m_fmtGroup->addButton(m_fmtPng, 1);
    m_fmtGroup->addButton(m_fmtWebp, 2);
    m_fmtGroup->addButton(m_fmtAvif, 3);
    fmtLayout->addWidget(m_fmtJpg);
    fmtLayout->addWidget(m_fmtPng);
    fmtLayout->addWidget(m_fmtWebp);
    fmtLayout->addWidget(m_fmtAvif);
    fmtLayout->addStretch();
    outputLayout->addLayout(fmtLayout);
    layout->addWidget(outputGroup);

    // ── Resize Options ──
    auto *resizeGroup = new QGroupBox("Resize Options");
    auto *resizeLayout = new QVBoxLayout(resizeGroup);

    auto *modeLayout = new QHBoxLayout;
    modeLayout->addWidget(new QLabel("Resize Mode:"));
    m_modePercent = new QRadioButton("Percentage");
    m_modeFitWidth = new QRadioButton("Fit Width");
    m_modeFitHeight = new QRadioButton("Fit Height");
    m_modeFitBox = new QRadioButton("Fit Box");
    m_modeNoResize = new QRadioButton("No Resize");
    m_modePercent->setChecked(true);
    m_modeGroup = new QButtonGroup(this);
    m_modeGroup->addButton(m_modePercent, 0);
    m_modeGroup->addButton(m_modeFitWidth, 1);
    m_modeGroup->addButton(m_modeFitHeight, 2);
    m_modeGroup->addButton(m_modeFitBox, 3);
    m_modeGroup->addButton(m_modeNoResize, 4);
    modeLayout->addWidget(m_modePercent);
    modeLayout->addWidget(m_modeFitWidth);
    modeLayout->addWidget(m_modeFitHeight);
    modeLayout->addWidget(m_modeFitBox);
    modeLayout->addWidget(m_modeNoResize);
    resizeLayout->addLayout(modeLayout);

    auto *resizeRow = new QHBoxLayout;
    resizeRow->addWidget(new QLabel("Resize %:"));
    m_resizeSlider = new QSlider(Qt::Horizontal);
    m_resizeSlider->setRange(1, 200);
    m_resizeSlider->setValue(100);
    m_resizeLabel = new QLabel("100%");
    m_resizeLabel->setMinimumWidth(45);
    resizeRow->addWidget(m_resizeSlider);
    resizeRow->addWidget(m_resizeLabel);
    resizeLayout->addLayout(resizeRow);

    auto *dimRow = new QHBoxLayout;
    dimRow->addWidget(new QLabel("Width:"));
    m_widthSpin = new QSpinBox;
    m_widthSpin->setRange(1, 99999);
    m_widthSpin->setValue(1920);
    dimRow->addWidget(m_widthSpin);
    dimRow->addWidget(new QLabel("Height:"));
    m_heightSpin = new QSpinBox;
    m_heightSpin->setRange(1, 99999);
    m_heightSpin->setValue(1080);
    dimRow->addWidget(m_heightSpin);
    resizeLayout->addLayout(dimRow);
    layout->addWidget(resizeGroup);

    // ── Quality & File Size ──
    auto *qualityGroup = new QGroupBox("Quality && File Size");
    auto *qualityLayout = new QVBoxLayout(qualityGroup);

    auto *qualRow = new QHBoxLayout;
    m_qualityTextLabel = new QLabel("Quality:");
    qualRow->addWidget(m_qualityTextLabel);
    m_qualitySlider = new QSlider(Qt::Horizontal);
    m_qualitySlider->setRange(1, 100);
    m_qualitySlider->setValue(85);
    m_qualityLabel = new QLabel("85");
    m_qualityLabel->setMinimumWidth(30);
    qualRow->addWidget(m_qualitySlider);
    qualRow->addWidget(m_qualityLabel);
    qualityLayout->addLayout(qualRow);

    auto *targetRow = new QHBoxLayout;
    m_targetSizeCheck = new QCheckBox("Target file size (KB):");
    m_targetSizeSpin = new QSpinBox;
    m_targetSizeSpin->setRange(1, 999999);
    m_targetSizeSpin->setValue(500);
    m_targetSizeSpin->setEnabled(false);
    targetRow->addWidget(m_targetSizeCheck);
    targetRow->addWidget(m_targetSizeSpin);
    targetRow->addStretch();
    qualityLayout->addLayout(targetRow);

    m_pngInfoLabel = new QLabel("PNG uses lossless compression \u2014 quality and target size settings do not apply.");
    m_pngInfoLabel->setStyleSheet("QLabel { color: #666; font-style: italic; padding: 2px 0; }");
    m_pngInfoLabel->setWordWrap(true);
    m_pngInfoLabel->setVisible(false);
    qualityLayout->addWidget(m_pngInfoLabel);
    layout->addWidget(qualityGroup);

    // ── Performance ──
    auto *perfGroup = new QGroupBox("Performance");
    auto *perfLayout = new QVBoxLayout(perfGroup);

    auto *threadRow = new QHBoxLayout;
    threadRow->addWidget(new QLabel("CPU Threads:"));
    m_threadCountSpin = new QSpinBox;
    int maxThreads = QThread::idealThreadCount();
    m_threadCountSpin->setRange(1, maxThreads);
    m_threadCountSpin->setValue(qMax(1, maxThreads - 1));
    m_threadCountSpin->setToolTip("Controls how many images are processed in parallel. "
                                   "Using all threads may make the system less responsive during processing.");
    threadRow->addWidget(m_threadCountSpin);
    threadRow->addStretch();
    perfLayout->addLayout(threadRow);

    auto *threadDesc = new QLabel(
        QString("Number of CPU threads used for image processing. Lower values leave more "
                "resources for other applications. Default: %1 of %2 available.")
            .arg(qMax(1, maxThreads - 1)).arg(maxThreads));
    threadDesc->setStyleSheet("QLabel { color: #666; font-style: italic; padding: 2px 0; }");
    threadDesc->setWordWrap(true);
    perfLayout->addWidget(threadDesc);
    layout->addWidget(perfGroup);

    tabWidget->addTab(page, "Advanced");

    // Advanced tab connections
    connect(m_browseOutputBtn, &QPushButton::clicked, this, &MainWindow::onBrowseOutput);
    connect(m_resizeSlider, &QSlider::valueChanged, this, [this](int val) {
        m_resizeLabel->setText(QString::number(val) + "%");
    });
    connect(m_qualitySlider, &QSlider::valueChanged, this, [this](int val) {
        m_qualityLabel->setText(QString::number(val));
    });
    connect(m_targetSizeCheck, &QCheckBox::toggled, this, &MainWindow::onTargetSizeToggled);
    connect(m_fmtGroup, &QButtonGroup::idClicked, this, &MainWindow::onFormatChanged);
    connect(m_modeGroup, &QButtonGroup::idClicked, this, [this](int) {
        onResizeModeChanged();
    });
    connect(m_threadCountSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this](int count) {
        if (m_threadPool) m_threadPool->setMaxThreadCount(count);
    });
}

void MainWindow::syncSimpleToAdvanced()
{
    QSignalBlocker b1(m_fmtGroup);
    QSignalBlocker b2(m_modeGroup);
    QSignalBlocker b3(m_resizeSlider);
    QSignalBlocker b4(m_qualitySlider);
    QSignalBlocker b5(m_outputDirEdit);
    QSignalBlocker b6(m_targetSizeCheck);

    // Format
    int fmtIndex = m_simpleFormatCombo->currentIndex();
    if (auto *btn = m_fmtGroup->button(fmtIndex))
        btn->setChecked(true);

    // Resize
    int resizeData = m_simpleResizeCombo->currentData().toInt();
    if (resizeData == -1) {
        // "Original Size" -> NoResize
        m_modeNoResize->setChecked(true);
    } else {
        // Percentage mode
        m_modePercent->setChecked(true);
        if (resizeData > 0) {
            m_resizeSlider->setValue(resizeData);  // 75, 50, or 25
        } else {
            // Custom - use slider value
            m_resizeSlider->setValue(m_simpleResizeSlider->value());
        }
    }

    // Quality
    int qualityValue = m_simpleQualityCombo->currentData().toInt();
    m_qualitySlider->setValue(qualityValue);

    // Output dir
    m_outputDirEdit->setText(m_simpleOutputDirEdit->text());

    // Target size: not exposed in Simple, ensure it's off
    m_targetSizeCheck->setChecked(false);

    // Update dependent control states
    updateResizeControls();
    onFormatChanged(fmtIndex);
}

void MainWindow::syncAdvancedToSimple()
{
    QSignalBlocker b1(m_simpleFormatCombo);
    QSignalBlocker b2(m_simpleResizeCombo);
    QSignalBlocker b3(m_simpleResizeSlider);
    QSignalBlocker b4(m_simpleQualityCombo);
    QSignalBlocker b5(m_simpleOutputDirEdit);

    // Format
    m_simpleFormatCombo->setCurrentIndex(m_fmtGroup->checkedId());

    // Resize
    int modeId = m_modeGroup->checkedId();
    if (modeId == 4) {  // NoResize
        m_simpleResizeCombo->setCurrentIndex(0);  // "Original Size"
    } else if (modeId == 0) {  // Percentage
        int pct = m_resizeSlider->value();
        if (pct == 75)      m_simpleResizeCombo->setCurrentIndex(1);
        else if (pct == 50) m_simpleResizeCombo->setCurrentIndex(2);
        else if (pct == 25) m_simpleResizeCombo->setCurrentIndex(3);
        else {
            m_simpleResizeCombo->setCurrentIndex(4);  // Custom
            m_simpleResizeSlider->setValue(pct);
        }
    } else {
        // FitWidth/FitHeight/FitBox: no Simple equivalent
        m_simpleResizeCombo->setCurrentIndex(0);  // Fall back to "Original Size"
    }

    // Quality: find nearest preset
    int q = m_qualitySlider->value();
    if (q <= 52)       m_simpleQualityCombo->setCurrentIndex(0);  // Low (40)
    else if (q <= 74)  m_simpleQualityCombo->setCurrentIndex(1);  // Medium (65)
    else if (q <= 92)  m_simpleQualityCombo->setCurrentIndex(2);  // High (85)
    else               m_simpleQualityCombo->setCurrentIndex(3);  // Maximum (100)

    // Output dir
    m_simpleOutputDirEdit->setText(m_outputDirEdit->text());

    // Update visibility of custom slider
    bool isCustom = (m_simpleResizeCombo->currentIndex() == 4);
    m_simpleResizeSlider->setVisible(isCustom);
    m_simpleResizeLabel->setVisible(isCustom);

    // Update quality description
    static const char *descriptions[] = {
        "Smallest file size, some visible quality loss",
        "Good balance of quality and file size",
        "Recommended for most uses \u2014 minimal quality loss",
        "Largest files, highest quality"
    };
    int qi = m_simpleQualityCombo->currentIndex();
    if (qi >= 0 && qi < 4)
        m_simpleQualityDesc->setText(descriptions[qi]);

    // Handle PNG format state
    bool isPng = (m_simpleFormatCombo->currentIndex() == 1);
    m_simpleQualityCombo->setEnabled(!isPng);
    m_simpleQualityDesc->setEnabled(!isPng);
    if (isPng)
        m_simpleQualityDesc->setText("PNG uses lossless compression \u2014 quality does not apply");
}

void MainWindow::onAddFiles()
{
    QStringList files = QFileDialog::getOpenFileNames(
        this, "Select Images", QString(), buildDialogFilter());
    addImageFiles(files);
}

void MainWindow::onAddFolder()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Select Folder");
    if (dir.isEmpty()) return;

    QStringList files;
    QDirIterator it(dir, IMAGE_FILTERS, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        files << it.next();
    }
    addImageFiles(files);
}

void MainWindow::addImageFiles(const QStringList &paths)
{
    for (const QString &path : paths) {
        // Avoid duplicates
        bool exists = false;
        for (int r = 0; r < m_inputTable->rowCount(); ++r) {
            if (m_inputTable->item(r, 0)->data(Qt::UserRole).toString() == path) {
                exists = true;
                break;
            }
        }
        if (exists) continue;

        QFileInfo info(path);
        QImageReader reader(path);
        QSize imgSize = reader.size();

        int row = m_inputTable->rowCount();
        m_inputTable->insertRow(row);

        auto *nameItem = new QTableWidgetItem(info.fileName());
        nameItem->setData(Qt::UserRole, path);
        m_inputTable->setItem(row, 0, nameItem);

        QString sizeStr;
        qint64 sz = info.size();
        if (sz >= 1024 * 1024)
            sizeStr = QString::number(sz / (1024.0 * 1024.0), 'f', 2) + " MB";
        else
            sizeStr = QString::number(sz / 1024.0, 'f', 1) + " KB";
        m_inputTable->setItem(row, 1, new QTableWidgetItem(sizeStr));

        QString dimStr = imgSize.isValid() ? QString("%1 x %2").arg(imgSize.width()).arg(imgSize.height()) : "?";
        m_inputTable->setItem(row, 2, new QTableWidgetItem(dimStr));
    }

    m_statusLabel->setText(QString("%1 file(s) loaded").arg(m_inputTable->rowCount()));
}

void MainWindow::onRemoveSelected()
{
    auto ranges = m_inputTable->selectedRanges();
    QList<int> rows;
    for (const auto &range : ranges) {
        for (int r = range.topRow(); r <= range.bottomRow(); ++r)
            rows << r;
    }
    std::sort(rows.begin(), rows.end(), std::greater<int>());
    rows.erase(std::unique(rows.begin(), rows.end()), rows.end());
    for (int r : rows)
        m_inputTable->removeRow(r);
}

void MainWindow::onClearAll()
{
    m_inputTable->setRowCount(0);
    m_statusLabel->setText("Ready");
}

void MainWindow::onBrowseOutput()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Select Output Folder");
    if (!dir.isEmpty())
        m_outputDirEdit->setText(dir);
}

void MainWindow::onProcess()
{
    if (m_watcher) return;

    // Ensure canonical (Advanced) state is current before building jobs
    if (m_tabWidget->currentIndex() == 0)
        syncSimpleToAdvanced();

    if (m_inputTable->rowCount() == 0) {
        QMessageBox::warning(this, "No Input", "Please add image files first.");
        return;
    }
    QString outputDir = m_outputDirEdit->text();
    m_usePerFileOutput = outputDir.isEmpty();

    if (!m_usePerFileOutput) {
        if (!QDir().mkpath(outputDir)) {
            QMessageBox::warning(this, "Error", "Could not create output directory: " + outputDir);
            return;
        }
    }

    // Validate UI selections
    int fmtId = m_fmtGroup->checkedId();
    int modeId = m_modeGroup->checkedId();
    if (fmtId < 0 || modeId < 0) {
        QMessageBox::warning(this, "Error", "Please select an output format and resize mode.");
        return;
    }
    OutputFormat fmt = static_cast<OutputFormat>(fmtId);
    ResizeMode mode = static_cast<ResizeMode>(modeId);
    QString ext = ImageProcessor::formatExtension(fmt);

    // Build jobs with pre-computed output paths (avoids race conditions in concurrent processing)
    QSet<QString> assignedPaths;
    QList<ProcessingJob> jobs;
    for (int r = 0; r < m_inputTable->rowCount(); ++r) {
        ProcessingJob job;
        job.inputPath = m_inputTable->item(r, 0)->data(Qt::UserRole).toString();
        job.outputDir = m_usePerFileOutput
            ? QFileInfo(job.inputPath).dir().filePath("resized")
            : outputDir;
        if (m_usePerFileOutput) {
            if (!QDir().mkpath(job.outputDir)) {
                QMessageBox::warning(this, "Error", "Could not create output directory: " + job.outputDir);
                return;
            }
        }
        job.outputPath = ImageProcessor::buildOutputPath(job.inputPath, job.outputDir, ext);
        // Deduplicate against already-assigned paths in this batch (handles same-named files from different dirs)
        if (assignedPaths.contains(job.outputPath)) {
            QFileInfo outInfo(job.outputPath);
            QString baseName = outInfo.completeBaseName();
            int counter = 1;
            QString candidate;
            do {
                candidate = QDir(job.outputDir).filePath(baseName + QString("_%1").arg(counter) + ext);
                ++counter;
            } while (assignedPaths.contains(candidate) || QFile::exists(candidate));
            job.outputPath = candidate;
        }
        assignedPaths.insert(job.outputPath);
        job.format = fmt;
        job.resizeMode = mode;
        job.resizePercent = m_resizeSlider->value();
        job.resizeWidth = m_widthSpin->value();
        job.resizeHeight = m_heightSpin->value();
        job.quality = m_qualitySlider->value();
        job.useTargetSize = m_targetSizeCheck->isChecked();
        job.targetSizeKB = m_targetSizeSpin->value();
        job.cancelFlag = &m_cancelled;
        jobs << job;
    }

    // Pre-populate results table with placeholders matching input order
    m_resultsTable->setRowCount(jobs.size());
    for (int r = 0; r < jobs.size(); ++r) {
        QFileInfo info(jobs[r].inputPath);
        m_resultsTable->setItem(r, 0, new QTableWidgetItem(info.fileName()));
        m_resultsTable->setItem(r, 1, new QTableWidgetItem("..."));
        m_resultsTable->setItem(r, 2, new QTableWidgetItem("..."));
        m_resultsTable->setItem(r, 3, new QTableWidgetItem("..."));
        m_resultsTable->setItem(r, 4, new QTableWidgetItem("Processing..."));
    }
    m_progressBar->setMaximum(jobs.size());
    m_progressBar->setValue(0);
    m_cancelled = false;
    m_processBtn->setEnabled(false);
    m_cancelBtn->setEnabled(true);
    m_statusLabel->setText("Processing...");

    m_watcher = new QFutureWatcher<ProcessingResult>(this);
    connect(m_watcher, &QFutureWatcher<ProcessingResult>::resultReadyAt, this, [this](int index) {
        ProcessingResult result = m_watcher->resultAt(index);
        m_progressBar->setValue(m_progressBar->value() + 1);

        // Update the pre-populated row in-place (preserves input order)
        int row = index;

        auto formatSize = [](qint64 sz) -> QString {
            if (sz >= 1024 * 1024)
                return QString::number(sz / (1024.0 * 1024.0), 'f', 2) + " MB";
            return QString::number(sz / 1024.0, 'f', 1) + " KB";
        };

        m_resultsTable->setItem(row, 1, new QTableWidgetItem(formatSize(result.originalSize)));
        m_resultsTable->setItem(row, 2, new QTableWidgetItem(formatSize(result.newSize)));

        if (result.status == ResultStatus::Success) {
            double pct = result.reductionPercent();
            auto *pctItem = new QTableWidgetItem(QString::number(pct, 'f', 1) + "%");
            if (pct > 50)
                pctItem->setForeground(QColor(0, 150, 0));
            else if (pct > 20)
                pctItem->setForeground(QColor(0, 100, 200));
            else if (pct < 0)
                pctItem->setForeground(QColor(200, 0, 0));
            m_resultsTable->setItem(row, 3, pctItem);
            QString statusText = "OK";
            if (!result.errorMessage.isEmpty())
                statusText += " (" + result.errorMessage + ")";
            m_resultsTable->setItem(row, 4, new QTableWidgetItem(statusText));
        } else if (result.status == ResultStatus::Cancelled) {
            m_resultsTable->setItem(row, 3, new QTableWidgetItem("-"));
            auto *statusItem = new QTableWidgetItem("Cancelled");
            statusItem->setForeground(QColor(150, 150, 150));
            m_resultsTable->setItem(row, 4, statusItem);
        } else {
            m_resultsTable->setItem(row, 3, new QTableWidgetItem("-"));
            auto *statusItem = new QTableWidgetItem(result.errorMessage);
            statusItem->setForeground(Qt::red);
            m_resultsTable->setItem(row, 4, statusItem);
        }
    });
    connect(m_watcher, &QFutureWatcher<ProcessingResult>::finished,
            this, &MainWindow::onProcessingFinished);

    m_threadPool->setMaxThreadCount(m_threadCountSpin->value());
    QFuture<ProcessingResult> future = QtConcurrent::mapped(m_threadPool, jobs, ImageProcessor::process);
    m_watcher->setFuture(future);
}

void MainWindow::onCancel()
{
    if (m_watcher) {
        m_cancelled = true;
        m_watcher->cancel();
        m_statusLabel->setText("Cancelling...");
    }
}

void MainWindow::onProcessingFinished()
{
    m_processBtn->setEnabled(true);
    m_cancelBtn->setEnabled(false);
    if (m_cancelled) {
        // Sweep stale "Processing..." rows that never got a result
        int completedCount = 0;
        for (int r = 0; r < m_resultsTable->rowCount(); ++r) {
            auto *statusItem = m_resultsTable->item(r, 4);
            if (statusItem && statusItem->text() == "Processing...") {
                m_resultsTable->setItem(r, 1, new QTableWidgetItem("-"));
                m_resultsTable->setItem(r, 2, new QTableWidgetItem("-"));
                m_resultsTable->setItem(r, 3, new QTableWidgetItem("-"));
                auto *cancelledItem = new QTableWidgetItem("Cancelled");
                cancelledItem->setForeground(QColor(150, 150, 150));
                m_resultsTable->setItem(r, 4, cancelledItem);
            } else if (statusItem && statusItem->text().startsWith("OK")) {
                ++completedCount;
            }
        }
        m_statusLabel->setText(QString("Cancelled (%1 of %2 completed)")
                               .arg(completedCount).arg(m_resultsTable->rowCount()));
    } else if (m_usePerFileOutput) {
        m_statusLabel->setText(QString("Done - %1 file(s) saved to \"resized\" subfolders next to originals")
                              .arg(m_resultsTable->rowCount()));
    } else {
        m_statusLabel->setText(QString("Done - %1 file(s) processed").arg(m_resultsTable->rowCount()));
    }
    m_watcher->deleteLater();
    m_watcher = nullptr;
}

void MainWindow::onCopyResults()
{
    QString tsv;
    // Header
    for (int c = 0; c < m_resultsTable->columnCount(); ++c) {
        if (c > 0) tsv += '\t';
        tsv += m_resultsTable->horizontalHeaderItem(c)->text();
    }
    tsv += '\n';
    // Rows
    for (int r = 0; r < m_resultsTable->rowCount(); ++r) {
        for (int c = 0; c < m_resultsTable->columnCount(); ++c) {
            if (c > 0) tsv += '\t';
            auto *item = m_resultsTable->item(r, c);
            tsv += item ? item->text() : "";
        }
        tsv += '\n';
    }
    QApplication::clipboard()->setText(tsv);
    m_statusLabel->setText("Results copied to clipboard");
}

void MainWindow::onOpenOutputFolder()
{
    QString dir = (m_tabWidget->currentIndex() == 0)
                      ? m_simpleOutputDirEdit->text()
                      : m_outputDirEdit->text();

    if (dir.isEmpty()) {
        // No explicit output dir — open the folder of the first input file
        if (m_inputTable->rowCount() > 0) {
            QString firstInput = m_inputTable->item(0, 0)->data(Qt::UserRole).toString();
            dir = QFileInfo(firstInput).absolutePath();
        } else {
            m_statusLabel->setText("No output folder set and no input files added.");
            return;
        }
    }

    QDir d(dir);
    if (!d.exists()) {
        m_statusLabel->setText("Output folder does not exist: " + QDir::toNativeSeparators(dir));
        return;
    }

    QDesktopServices::openUrl(QUrl::fromLocalFile(d.absolutePath()));
}

void MainWindow::onAbout()
{
    QMessageBox::about(this, "About Simple Image Resizer",
        "<h3>Simple Image Resizer</h3>"
        "<p>Copyright 2024-2026 thanolion</p>"
        "<p>Licensed under the GNU General Public License v3.0</p>"
        "<p>A batch image resizer and compressor.</p>"

        "<hr><h4>Qt Framework</h4>"
        "<p>Built with Qt " QT_VERSION_STR " (dynamic linking).</p>"
        "<p>Qt is used under the <b>LGPL v3</b> license.<br>"
        "See <a href=\"https://www.qt.io/licensing\">qt.io/licensing</a> for details.<br>"
        "Source code available at <a href=\"https://code.qt.io\">code.qt.io</a>.</p>"

        "<hr><h4>LibRaw 0.21.3</h4>"
        "<p>Copyright 2008-2024 LibRaw LLC<br>"
        "Used under <b>LGPL v2.1</b> (also available under CDDL v1.0).<br>"
        "See <a href=\"https://www.libraw.org\">www.libraw.org</a> for details.</p>"

        "<hr><h4>libavif 1.1.1</h4>"
        "<p>Copyright 2019 Joe Drago and libavif contributors<br>"
        "Used under <b>BSD 2-Clause</b> license.<br>"
        "See <a href=\"https://github.com/AOMediaCodec/libavif\">github.com/AOMediaCodec/libavif</a> for details.</p>"

        "<hr><h4>libaom (AV1 codec)</h4>"
        "<p>Copyright 2016 Alliance for Open Media<br>"
        "Used under <b>BSD 2-Clause</b> license.<br>"
        "See <a href=\"https://aomedia.googlesource.com/aom/\">aomedia.googlesource.com/aom</a> for details.</p>"

        "<hr><h4>LibRaw Sub-dependencies</h4>"
        "<ul>"
        "<li><b>dcraw</b> by Dave Coffin (public domain)</li>"
        "<li><b>DCB and FBDD demosaic</b> by Jacek Gozdz (BSD 3-Clause)</li>"
        "<li><b>X3F decoder</b> by Roland Karlsson (BSD)</li>"
        "<li><b>Adobe DNG SDK</b> (MIT License)</li>"
        "</ul>"

        "<hr><h4>Support Development</h4>"
        "<p><a href=\"https://github.com/sponsors/thanolion\">GitHub Sponsors</a>"
        " | <a href=\"https://ko-fi.com/cullen38127\">Ko-fi</a></p>"

        "<hr><p>Full license texts are included in the <b>licenses/</b> folder "
        "distributed with this application.</p>");
}

void MainWindow::onDonate()
{
    QMessageBox box(this);
    box.setWindowTitle("Support Development");
    box.setTextFormat(Qt::RichText);
    box.setText(
        "<p>If you find Simple Image Resizer useful, consider supporting its development:</p>"
        "<ul>"
        "<li><a href=\"https://github.com/sponsors/thanolion\">GitHub Sponsors</a></li>"
        "<li><a href=\"https://ko-fi.com/cullen38127\">Ko-fi</a></li>"
        "</ul>"
        "<p>Thank you for your support!</p>"
    );
    box.exec();
}

void MainWindow::onFormatGuide()
{
    if (!m_formatGuideDialog) {
        m_formatGuideDialog = new FormatGuideDialog(this);
    }
    m_formatGuideDialog->show();
    m_formatGuideDialog->raise();
    m_formatGuideDialog->activateWindow();
}

void MainWindow::onResizeModeChanged()
{
    updateResizeControls();
}

void MainWindow::onTargetSizeToggled(bool checked)
{
    m_targetSizeSpin->setEnabled(checked);
    bool isPng = (m_fmtGroup->checkedId() == 1);
    if (!isPng) {
        m_qualitySlider->setEnabled(!checked);
        m_qualityTextLabel->setEnabled(!checked);
        m_qualityLabel->setEnabled(!checked);
    }
}

void MainWindow::onFormatChanged(int formatId)
{
    bool isPng = (formatId == 1); // OutputFormat::PNG

    // Quality controls
    m_qualityTextLabel->setEnabled(!isPng);
    m_qualitySlider->setEnabled(!isPng && !m_targetSizeCheck->isChecked());
    m_qualityLabel->setEnabled(!isPng);

    // Target size controls
    if (isPng) {
        m_targetSizeCheck->setChecked(false);
        m_targetSizeCheck->setEnabled(false);
        m_targetSizeSpin->setEnabled(false);
    } else {
        m_targetSizeCheck->setEnabled(true);
        m_targetSizeSpin->setEnabled(m_targetSizeCheck->isChecked());
        // Restore quality slider state based on target size toggle
        m_qualitySlider->setEnabled(!m_targetSizeCheck->isChecked());
    }

    // Info label
    m_pngInfoLabel->setVisible(isPng);
}

void MainWindow::updateResizeControls()
{
    int mode = m_modeGroup->checkedId();
    bool pctMode = (mode == 0);
    bool widthMode = (mode == 1 || mode == 3);
    bool heightMode = (mode == 2 || mode == 3);
    bool noResize = (mode == 4);

    m_resizeSlider->setEnabled(pctMode);
    m_widthSpin->setEnabled(widthMode);
    m_heightSpin->setEnabled(heightMode);

    if (noResize) {
        m_resizeSlider->setEnabled(false);
        m_widthSpin->setEnabled(false);
        m_heightSpin->setEnabled(false);
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    static const QStringList BARE_EXTENSIONS = bareExtensions();
    QStringList paths;
    for (const QUrl &url : event->mimeData()->urls()) {
        QString path = url.toLocalFile();
        QFileInfo info(path);
        if (info.isDir()) {
            QDirIterator it(path, IMAGE_FILTERS, QDir::Files, QDirIterator::Subdirectories);
            while (it.hasNext())
                paths << it.next();
        } else if (info.isFile()) {
            QString ext = info.suffix().toLower();
            if (BARE_EXTENSIONS.contains(ext))
                paths << path;
        }
    }
    addImageFiles(paths);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_watcher && m_watcher->isRunning()) {
        m_cancelled = true;
        m_watcher->disconnect();
        m_watcher->cancel();
        m_statusLabel->setText("Cancelling...");
        m_watcher->waitForFinished();
    }
    saveSettings();
    event->accept();
}

void MainWindow::loadSettings()
{
    auto &s = SettingsManager::instance();
    m_outputDirEdit->setText(s.outputDir());

    int fmtId = static_cast<int>(s.outputFormat());
    if (auto *btn = m_fmtGroup->button(fmtId)) btn->setChecked(true);

    int modeId = static_cast<int>(s.resizeMode());
    if (auto *btn = m_modeGroup->button(modeId)) btn->setChecked(true);

    m_resizeSlider->setValue(s.resizePercent());
    m_widthSpin->setValue(s.resizeWidth());
    m_heightSpin->setValue(s.resizeHeight());
    m_qualitySlider->setValue(s.quality());
    m_targetSizeCheck->setChecked(s.useTargetSize());
    m_targetSizeSpin->setValue(static_cast<int>(s.targetSizeKB()));

    m_threadCountSpin->setValue(s.threadCount());
    m_threadPool->setMaxThreadCount(s.threadCount());
    m_tabWidget->setCurrentIndex(s.lastActiveTab());

    updateResizeControls();
    onTargetSizeToggled(m_targetSizeCheck->isChecked());
    onFormatChanged(m_fmtGroup->checkedId());
}

void MainWindow::saveSettings()
{
    // Ensure canonical (Advanced) state is current before saving
    if (m_tabWidget->currentIndex() == 0)
        syncSimpleToAdvanced();

    auto &s = SettingsManager::instance();
    s.setOutputDir(m_outputDirEdit->text());
    s.setOutputFormat(static_cast<OutputFormat>(m_fmtGroup->checkedId()));
    s.setResizeMode(static_cast<ResizeMode>(m_modeGroup->checkedId()));
    s.setResizePercent(m_resizeSlider->value());
    s.setResizeWidth(m_widthSpin->value());
    s.setResizeHeight(m_heightSpin->value());
    s.setQuality(m_qualitySlider->value());
    s.setUseTargetSize(m_targetSizeCheck->isChecked());
    s.setTargetSizeKB(m_targetSizeSpin->value());
    s.setThreadCount(m_threadCountSpin->value());
    s.setLastActiveTab(m_tabWidget->currentIndex());
}
