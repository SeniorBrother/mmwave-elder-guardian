// QT5 上位机看板入口
#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Elder Guardian Host");
    app.setApplicationVersion("1.0");

    MainWindow w;
    w.resize(1024, 680);
    w.show();
    return app.exec();
}
