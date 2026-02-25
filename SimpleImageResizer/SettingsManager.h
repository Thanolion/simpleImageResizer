#pragma once

#include <QString>
#include "ProcessingJob.h"

class SettingsManager {
public:
    static SettingsManager &instance();

    QString outputDir() const;
    void setOutputDir(const QString &dir);

    OutputFormat outputFormat() const;
    void setOutputFormat(OutputFormat fmt);

    ResizeMode resizeMode() const;
    void setResizeMode(ResizeMode mode);

    int resizePercent() const;
    void setResizePercent(int pct);

    int resizeWidth() const;
    void setResizeWidth(int w);

    int resizeHeight() const;
    void setResizeHeight(int h);

    int quality() const;
    void setQuality(int q);

    bool useTargetSize() const;
    void setUseTargetSize(bool use);

    qint64 targetSizeKB() const;
    void setTargetSizeKB(qint64 kb);

private:
    SettingsManager() = default;
};
