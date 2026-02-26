// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2024-2026 thanolion

#include "SimpleImageResizer.h"
#include "MainWindow.h"

#include <QIcon>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Simple Image Resizer");
    app.setOrganizationName("SimpleImageResizer");

    // Ensure Qt finds image format plugins (e.g. qwebp) next to the executable
    QApplication::addLibraryPath(QCoreApplication::applicationDirPath());

    app.setWindowIcon(QIcon(":/icons/app_icon.png"));

    MainWindow window;
    window.show();

    return app.exec();
}
