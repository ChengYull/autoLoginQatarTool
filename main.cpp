#include "mainwidget.h"

#include <QApplication>
#include "processdetector.h"
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWidget w;
    w.show();
    ProcessDetector detector;
    return a.exec();
}
