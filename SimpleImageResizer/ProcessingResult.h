// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2024-2026 thanolion

#pragma once

#include <QString>

enum class ResultStatus {
    Success,
    FailedToLoad,
    FailedToSave,
    Cancelled
};

struct ProcessingResult {
    QString inputPath;
    QString outputPath;
    qint64 originalSize = 0;
    qint64 newSize = 0;
    int originalWidth = 0;
    int originalHeight = 0;
    int newWidth = 0;
    int newHeight = 0;
    ResultStatus status = ResultStatus::Success;
    QString errorMessage;

    double reductionPercent() const {
        if (originalSize <= 0) return 0.0;
        return (1.0 - static_cast<double>(newSize) / static_cast<double>(originalSize)) * 100.0;
    }
};
