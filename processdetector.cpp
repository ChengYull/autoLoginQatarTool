#include "processdetector.h"

ProcessDetector::ProcessDetector(QObject *parent)
    : QObject{parent}
{
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &ProcessDetector::checkProcess);
    //timer->start(1000); // 每1秒检查一次
}

void ProcessDetector::checkProcess() {
    QProcess process;
    process.start("tasklist", QStringList() << "/FI" << "IMAGENAME eq QaTar.exe");
    process.waitForFinished();
    QString output = process.readAllStandardOutput();
    if (output.contains("QaTar.exe")) {
        qDebug() << "QaTar.exe is running!";
        // 在这里发送一个登录的信号
        emit loginSignal();
    }
}

void ProcessDetector::startDetection() {
    if (!timer->isActive()) {
        timer->start(1000); // 重新启动定时器，间隔为 1 秒
        qDebug() << "Detection started.";
    }
}

void ProcessDetector::stopDetection() {
    if (timer->isActive()) {
        timer->stop(); // 停止定时器
        qDebug() << "Detection stopped.";
    }
}
