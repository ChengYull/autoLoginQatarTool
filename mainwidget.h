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
#include <QTimer>
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

    /* 自动登录状态机 */
    enum class LoginState {
        Stopped,   // 未启用
        Running,   // 监测中
        Handling   // 已检测到客户端，正在执行登录动作
    };

    ProcessDetector* detector;             // 进程检测器
    QTimer* loginTimer;                    // 登录动作延时定时器（可被停止操作取消）
    LoginState loginState = LoginState::Stopped;
    QString account = "";
    QString password = "";
    bool autoStartStatus = false;

    QSystemTrayIcon *trayIcon; // 系统托盘图标
    QMenu *trayMenu;           // 托盘右键菜单
    QAction *exitAction;       // 退出动作

    void setAutoStart(bool enabled);
    void saveSettings();
    void loadSettings();
    void setLoginState(LoginState state);  // 切换状态并同步界面显示
    void updateStatusUI();                 // 刷新状态标签与托盘提示
    void performLogin();                   // 执行登录动作
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
