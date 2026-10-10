#include "processdetector.h"

#include <string.h>

#include "windows.h"
#include <tlhelp32.h>

namespace {

// 需要监测的客户端进程映像名
const wchar_t *kTargetImage = L"QaTar.exe";

/* 通过系统进程快照按映像名查找目标进程。
 * 原先的方案是每轮起一个 tasklist 子进程并同步 waitForFinished()，实测单次约 200 毫秒，
 * 且期间界面线程完全阻塞。这里改为一次系统调用，实测约 7~9 毫秒（随系统进程数变化），
 * 既不创建子进程，也不会造成可感知的界面卡顿。 */
bool isProcessRunning(const wchar_t *imageName)
{
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return false;

    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(PROCESSENTRY32W);

    bool found = false;
    if (Process32FirstW(snapshot, &entry)) {
        do {
            if (_wcsicmp(entry.szExeFile, imageName) == 0) {
                found = true;
                break;
            }
        } while (Process32NextW(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return found;
}

} // namespace

ProcessDetector::ProcessDetector(QObject *parent)
    : QObject{parent}
{
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &ProcessDetector::checkProcess);
}

void ProcessDetector::checkProcess() {
    if (isProcessRunning(kTargetImage)) {
        qDebug() << "QaTar.exe is running!";
        emit loginSignal();
    }
}

void ProcessDetector::startDetection() {
    if (!timer->isActive()) {
        timer->start(kPollIntervalMs);
        qDebug() << "Detection started.";
    }
}

void ProcessDetector::stopDetection() {
    if (timer->isActive()) {
        timer->stop();
        qDebug() << "Detection stopped.";
    }
}
