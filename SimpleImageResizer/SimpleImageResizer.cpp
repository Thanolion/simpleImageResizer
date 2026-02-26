// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2024-2026 thanolion

#include "SimpleImageResizer.h"
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Simple Image Resizer");
    app.setOrganizationName("SimpleImageResizer");

    MainWindow window;
    window.show();

    return app.exec();
}
