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
    QString appName = "AutoLogin ";
    QString version = "v1.0.3";
    QString tip = "";
    QString statueTip = "";
    QString exePath = "";

    QVBoxLayout* mainLayout;
    QHBoxLayout* accountLayout;
    QLabel* accountLabel;
    QLineEdit* accountInput;
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

    ProcessDetector* detector;             // 进程检测器
    bool enableStatus = false;
    QString account = "";
    QString password = "";
    bool autoStartStatus = false;

    QSystemTrayIcon *trayIcon; // 系统托盘图标
    QMenu *trayMenu;           // 托盘右键菜单
    QAction *exitAction;       // 退出动作

    void setAutoStart(bool enabled);
    void saveSettings();
    void loadSettings();
    HWND findDialogHwnd();   // 查找密码对话框（自动适配简体/繁体/英文环境）
    HWND findPanelHwnd();    // 查找对话框内的面板控件
    HWND findAccountHwnd();  // 查找账户（Emp ID / 员工ID）下拉框
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
