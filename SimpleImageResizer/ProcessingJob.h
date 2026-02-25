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
    OutputFormat format = OutputFormat::JPEG;
    ResizeMode resizeMode = ResizeMode::Percentage;
    int resizePercent = 100;
    int resizeWidth = 0;
    int resizeHeight = 0;
    int quality = 85;
    bool useTargetSize = false;
    qint64 targetSizeKB = 500;
};
