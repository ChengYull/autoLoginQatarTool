#ifndef PROCESSDETECTOR_H
#define PROCESSDETECTOR_H

#include <QObject>
#include <QProcess>
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
    QTimer *timer;

};

#endif // PROCESSDETECTOR_H
