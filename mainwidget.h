#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QCheckBox>
#include <QSettings>
#include <QCoreApplication>
#include "processdetector.h"
#include <QMessageBox>
#include "windows.h"
#include <QDebug>
#include <QRadioButton>
#include <QButtonGroup>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QCloseEvent>
#include <QApplication>
class MainWidget : public QWidget
{
    Q_OBJECT

public:
    MainWidget(QWidget *parent = nullptr);
    ~MainWidget();

private:
    QString version = "v1.0.1";
    QString tip = "";
    QString statueTip = "";
    QString exePath = "";

    QVBoxLayout* mainLayout;
    QHBoxLayout* passwordLayout;
    QLabel* passwordLabel;
    QLineEdit* passwordInput;
    QPushButton* settingPasswordBtn;
    QCheckBox* autoStartEnable;
    QPushButton* startAutoLogin;
    QPushButton* stopAutoLogin;
    QHBoxLayout* btnLayout;
    QLabel* statueLabel1;
    QLabel* statueLabel2;
    QHBoxLayout* statueLayout;
    QRadioButton *radio1;
    QRadioButton *radio2;

    ProcessDetector* detector;             // 进程检测器
    bool enableStatus = false;
    QString password = "";
    bool autoStartStatus = false;
    bool isSimpleEnvironment = true;

    QSystemTrayIcon *trayIcon; // 系统托盘图标
    QMenu *trayMenu;           // 托盘右键菜单
    QAction *exitAction;       // 退出动作

    void setAutoStart(bool enabled);
    void saveSettings();
    void loadSettings();
    HWND findPasswordHwnd();
    HWND findLoginHwnd();
    HWND findErrorMsg();
private slots:
    void enableAutoLogin();
    void disableAutoLogin();
    void login();
    void onExit(); // 退出程序
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason); // 托盘图标点击事件
protected:
    void closeEvent(QCloseEvent *event) override; // 重写关闭事件
};
#endif // MAINWIDGET_H
