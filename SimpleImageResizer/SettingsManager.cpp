// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2024-2026 thanolion

#include "SettingsManager.h"
#include <QSettings>

SettingsManager &SettingsManager::instance()
{
    static SettingsManager mgr;
    return mgr;
}

QString SettingsManager::outputDir() const
{
    QSettings s;
    return s.value("outputDir").toString();
}

void SettingsManager::setOutputDir(const QString &dir)
{
    QSettings s;
    s.setValue("outputDir", dir);
}

OutputFormat SettingsManager::outputFormat() const
{
    QSettings s;
    return static_cast<OutputFormat>(s.value("outputFormat", 0).toInt());
}

void SettingsManager::setOutputFormat(OutputFormat fmt)
{
    QSettings s;
    s.setValue("outputFormat", static_cast<int>(fmt));
}

ResizeMode SettingsManager::resizeMode() const
{
    QSettings s;
    return static_cast<ResizeMode>(s.value("resizeMode", 0).toInt());
}

void SettingsManager::setResizeMode(ResizeMode mode)
{
    QSettings s;
    s.setValue("resizeMode", static_cast<int>(mode));
}

int SettingsManager::resizePercent() const
{
    QSettings s;
    return s.value("resizePercent", 100).toInt();
}

void SettingsManager::setResizePercent(int pct)
{
    QSettings s;
    s.setValue("resizePercent", pct);
}

int SettingsManager::resizeWidth() const
{
    QSettings s;
    return s.value("resizeWidth", 1920).toInt();
}

void SettingsManager::setResizeWidth(int w)
{
    QSettings s;
    s.setValue("resizeWidth", w);
}

int SettingsManager::resizeHeight() const
{
    QSettings s;
    return s.value("resizeHeight", 1080).toInt();
}

void SettingsManager::setResizeHeight(int h)
{
    QSettings s;
    s.setValue("resizeHeight", h);
}

int SettingsManager::quality() const
{
    QSettings s;
    return s.value("quality", 85).toInt();
}

void SettingsManager::setQuality(int q)
{
    QSettings s;
    s.setValue("quality", q);
}

bool SettingsManager::useTargetSize() const
{
    QSettings s;
    return s.value("useTargetSize", false).toBool();
}

void SettingsManager::setUseTargetSize(bool use)
{
    QSettings s;
    s.setValue("useTargetSize", use);
}

qint64 SettingsManager::targetSizeKB() const
{
    QSettings s;
    return s.value("targetSizeKB", 500).toLongLong();
}

void SettingsManager::setTargetSizeKB(qint64 kb)
{
    QSettings s;
    s.setValue("targetSizeKB", kb);
}
