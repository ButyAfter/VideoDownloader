#include <QApplication>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    /*
     * 中文字体
     */
    QFont font(
        QStringLiteral("Microsoft YaHei"),
        10);

    app.setFont(font);

    MainWindow window;

    window.show();

    return app.exec();
}
