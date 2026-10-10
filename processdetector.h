#ifndef PROCESSDETECTOR_H
#define PROCESSDETECTOR_H

#include <QObject>
#include <QTimer>
#include <QDebug>

class ProcessDetector : public QObject
{
    Q_OBJECT
public:
    explicit ProcessDetector(QObject *parent = nullptr);
    void startDetection();
    void stopDetection();

signals:
    void loginSignal();

private slots:
    void checkProcess();

private:
    // 轮询间隔（毫秒）。改用系统进程快照接口后不再创建子进程，
    // 单次检测约 7~9 毫秒（随系统进程数变化），可按需要取较小间隔。
    static constexpr int kPollIntervalMs = 500;

    QTimer *timer;

};

#endif // PROCESSDETECTOR_H
