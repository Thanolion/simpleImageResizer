#pragma once

#include "ProcessingJob.h"
#include "ProcessingResult.h"

class QImage;

class ImageProcessor {
public:
    static ProcessingResult process(const ProcessingJob &job);

private:
    static QImage loadImage(const QString &path);
    static QString formatExtension(OutputFormat fmt);
    static QByteArray formatName(OutputFormat fmt);
    static QString buildOutputPath(const QString &inputPath, const QString &outputDir, const QString &ext);
};
