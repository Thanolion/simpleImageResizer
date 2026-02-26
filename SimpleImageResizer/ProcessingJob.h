// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2024-2026 thanolion

#pragma once

#include <QString>

enum class ResizeMode {
    Percentage,
    FitWidth,
    FitHeight,
    FitBoundingBox,
    NoResize
};

enum class OutputFormat {
    JPEG,
    PNG,
    WebP
};

struct ProcessingJob {
    QString inputPath;
    QString outputDir;
    QString outputPath;  // Pre-computed by main thread to avoid race conditions
    OutputFormat format = OutputFormat::JPEG;
    ResizeMode resizeMode = ResizeMode::Percentage;
    int resizePercent = 100;
    int resizeWidth = 0;
    int resizeHeight = 0;
    int quality = 85;
    bool useTargetSize = false;
    qint64 targetSizeKB = 500;
};
