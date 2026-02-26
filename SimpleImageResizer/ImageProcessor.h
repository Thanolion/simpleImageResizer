// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2024-2026 thanolion

#pragma once

#include "ProcessingJob.h"
#include "ProcessingResult.h"

class QImage;

class ImageProcessor {
public:
    static ProcessingResult process(const ProcessingJob &job);
    static QString buildOutputPath(const QString &inputPath, const QString &outputDir, const QString &ext);
    static QString formatExtension(OutputFormat fmt);

private:
    static QImage loadImage(const QString &path);
    static QByteArray formatName(OutputFormat fmt);
    static QImage loadAvifImage(const QString &path);
    static bool saveAvifImage(const QImage &img, const QString &path, int quality);
};
