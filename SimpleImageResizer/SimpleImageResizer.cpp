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
