#include "processdetector.h"

ProcessDetector::ProcessDetector(QObject *parent)
    : QObject{parent}
{
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &ProcessDetector::checkProcess);
    timer->start(1000); // 每1秒检查一次
}

void ProcessDetector::checkProcess() {
    QProcess process;
    process.start("tasklist", QStringList() << "/FI" << "IMAGENAME eq QaTar.exe");
    process.waitForFinished();
    QString output = process.readAllStandardOutput();
    if (output.contains("QaTar.exe")) {
        qDebug() << "QaTar.exe is running!";
        // 在这里调用自动登录的函数
    }
}
