#include "mainwidget.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("AutoLoginQatarTool");
    a.setApplicationDisplayName("AutoLoginQatarTool");
    a.setWindowIcon(MainWidget::appIcon());

    MainWidget w;
    w.show();
    return a.exec();
}
