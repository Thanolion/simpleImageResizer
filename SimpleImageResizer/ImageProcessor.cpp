// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2024-2026 thanolion

#include "ImageProcessor.h"
#include <QImage>
#include <QFileInfo>
#include <QDir>
#include <QBuffer>
#include <QImageWriter>
#include <libraw/libraw.h>

static QImage loadRawImage(const QString &path)
{
    LibRaw raw;
#ifdef _WIN32
    if (raw.open_file(path.toStdWString().c_str()) != LIBRAW_SUCCESS) return {};
#else
    if (raw.open_file(path.toUtf8().constData()) != LIBRAW_SUCCESS) return {};
#endif
    if (raw.unpack() != LIBRAW_SUCCESS) return {};
    raw.imgdata.params.output_bps = 8;
    raw.imgdata.params.use_auto_wb = 1;
    if (raw.dcraw_process() != LIBRAW_SUCCESS) return {};
    libraw_processed_image_t *img = raw.dcraw_make_mem_image();
    if (!img || img->type != LIBRAW_IMAGE_BITMAP) {
        raw.dcraw_clear_mem(img);
        return {};
    }
    if (img->colors != 3 || img->bits != 8) {
        raw.dcraw_clear_mem(img);
        return {};
    }
    QImage result(img->data, img->width, img->height,
                  img->width * 3, QImage::Format_RGB888);
    QImage copy = result.copy(); // deep copy before freeing LibRaw memory
    raw.dcraw_clear_mem(img);
    return copy;
}

QImage ImageProcessor::loadImage(const QString &path)
{
    QImage img(path);
    if (!img.isNull()) return img;
    return loadRawImage(path);
}

ProcessingResult ImageProcessor::process(const ProcessingJob &job)
{
    ProcessingResult result;
    result.inputPath = job.inputPath;

    QFileInfo inputInfo(job.inputPath);
    result.originalSize = inputInfo.size();

    QImage img = loadImage(job.inputPath);
    if (img.isNull()) {
        result.status = ResultStatus::FailedToLoad;
        result.errorMessage = "Failed to load image: " + job.inputPath;
        return result;
    }

    result.originalWidth = img.width();
    result.originalHeight = img.height();

    // Resize
    QImage resized;
    switch (job.resizeMode) {
    case ResizeMode::Percentage: {
        int newW = static_cast<int>(static_cast<qint64>(img.width()) * job.resizePercent / 100);
        int newH = static_cast<int>(static_cast<qint64>(img.height()) * job.resizePercent / 100);
        if (newW < 1) newW = 1;
        if (newH < 1) newH = 1;
        resized = img.scaled(newW, newH, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        break;
    }
    case ResizeMode::FitWidth:
        if (job.resizeWidth > 0) {
            resized = img.scaledToWidth(job.resizeWidth, Qt::SmoothTransformation);
        } else {
            resized = img;
        }
        break;
    case ResizeMode::FitHeight:
        if (job.resizeHeight > 0) {
            resized = img.scaledToHeight(job.resizeHeight, Qt::SmoothTransformation);
        } else {
            resized = img;
        }
        break;
    case ResizeMode::FitBoundingBox:
        if (job.resizeWidth > 0 && job.resizeHeight > 0) {
            resized = img.scaled(job.resizeWidth, job.resizeHeight,
                                 Qt::KeepAspectRatio, Qt::SmoothTransformation);
        } else {
            resized = img;
        }
        break;
    case ResizeMode::NoResize:
        resized = img;
        break;
    default:
        result.status = ResultStatus::FailedToSave;
        result.errorMessage = "Unknown resize mode";
        return result;
    }

    result.newWidth = resized.width();
    result.newHeight = resized.height();

    QByteArray fmtName = formatName(job.format);
    QString outputPath = job.outputPath;
    result.outputPath = outputPath;

    if (job.useTargetSize && job.format == OutputFormat::PNG) {
        result.errorMessage = "Target size not supported for PNG format";
    }

    if (job.useTargetSize && job.format != OutputFormat::PNG) {
        // Binary search for quality to hit target file size
        qint64 targetBytes = job.targetSizeKB * 1024;
        int lo = 1, hi = 95;
        int bestQuality = lo;
        QByteArray bestData;

        for (int iter = 0; iter < 10 && lo <= hi; ++iter) {
            int mid = (lo + hi) / 2;
            QByteArray data;
            QBuffer buffer(&data);
            buffer.open(QIODevice::WriteOnly);
            QImageWriter writer(&buffer, fmtName);
            writer.setQuality(mid);
            if (!writer.write(resized)) {
                result.status = ResultStatus::FailedToSave;
                result.errorMessage = "Failed to encode image at quality " + QString::number(mid);
                return result;
            }
            buffer.close();

            if (data.size() <= targetBytes) {
                bestQuality = mid;
                bestData = data;
                lo = mid + 1;
            } else {
                hi = mid - 1;
            }
        }

        // If we never got under target, use lowest quality result
        if (bestData.isEmpty()) {
            QBuffer buffer(&bestData);
            buffer.open(QIODevice::WriteOnly);
            QImageWriter writer(&buffer, fmtName);
            writer.setQuality(1);
            if (!writer.write(resized)) {
                result.status = ResultStatus::FailedToSave;
                result.errorMessage = "Failed to encode image at minimum quality";
                return result;
            }
            buffer.close();
        }

        QFile outFile(outputPath);
        if (!outFile.open(QIODevice::WriteOnly)) {
            result.status = ResultStatus::FailedToSave;
            result.errorMessage = "Cannot open output file: " + outputPath;
            return result;
        }
        outFile.write(bestData);
        outFile.close();
        result.newSize = bestData.size();
    } else {
        QImageWriter writer(outputPath, fmtName);
        if (job.format != OutputFormat::PNG) {
            writer.setQuality(job.quality);
        }
        if (!writer.write(resized)) {
            result.status = ResultStatus::FailedToSave;
            result.errorMessage = "Failed to save: " + writer.errorString();
            return result;
        }
        QFileInfo outInfo(outputPath);
        result.newSize = outInfo.size();
    }

    result.status = ResultStatus::Success;
    return result;
}

QString ImageProcessor::formatExtension(OutputFormat fmt)
{
    switch (fmt) {
    case OutputFormat::JPEG: return ".jpg";
    case OutputFormat::PNG:  return ".png";
    case OutputFormat::WebP: return ".webp";
    }
    return ".jpg";
}

QByteArray ImageProcessor::formatName(OutputFormat fmt)
{
    switch (fmt) {
    case OutputFormat::JPEG: return "jpeg";
    case OutputFormat::PNG:  return "png";
    case OutputFormat::WebP: return "webp";
    }
    return "jpeg";
}

QString ImageProcessor::buildOutputPath(const QString &inputPath, const QString &outputDir, const QString &ext)
{
    QFileInfo info(inputPath);
    QString baseName = info.completeBaseName();
    QString outPath = QDir(outputDir).filePath(baseName + ext);

    // If output would overwrite input, append _resized
    if (QFileInfo(outPath) == QFileInfo(inputPath)) {
        outPath = QDir(outputDir).filePath(baseName + "_resized" + ext);
    }

    // Avoid overwriting existing output files
    if (QFile::exists(outPath)) {
        int counter = 1;
        QString candidate;
        do {
            candidate = QDir(outputDir).filePath(baseName + QString("_%1").arg(counter) + ext);
            ++counter;
            if (counter > 10000) return outPath; // Safety limit
        } while (QFile::exists(candidate));
        outPath = candidate;
    }

    return outPath;
}
