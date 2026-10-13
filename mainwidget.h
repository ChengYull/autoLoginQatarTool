#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QFrame>
#include <QSettings>
#include <QCoreApplication>
#include <QIcon>
#include <QPoint>
#include "processdetector.h"
#include "toggleswitch.h"
#include <QMessageBox>
#include "windows.h"
#include <QDebug>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QCloseEvent>
#include <QApplication>
#include <QTimer>

class QNetworkAccessManager;

class MainWidget : public QWidget
{
    Q_OBJECT

public:
    MainWidget(QWidget *parent = nullptr);
    ~MainWidget();

    static QIcon appIcon();  // 应用图标：窗口 / 任务栏 / 托盘共用，由内嵌 SVG 渲染成多尺寸位图

private:
    const QString appName = "AutoLoginQatarTool";
    QString tip = "";
    QString statueTip = "";

    /* 界面控件 */
    QWidget* container;             // 圆角卡片容器
    QWidget* titleBar;              // 自定义标题栏（可拖动）
    QLabel* statusDot;              // 状态指示圆点
    QLabel* statusBadge;            // 状态徽标
    QLineEdit* accountInput;
    QLineEdit* passwordInput;
    QAction* passwordEyeAction;     // 密码框右侧的眼睛图标（明文 / 掩码切换）
    QPushButton* resetPasswordBtn;  // 重置密码（新密码发到钉钉）
    ToggleSwitch* autoStartSwitch;  // 开机自启开关
    QPushButton* startAutoLogin;
    QPushButton* stopAutoLogin;

    /* 自动登录状态机 */
    enum class LoginState {
        Stopped,   // 未启用
        Running,   // 监测中
        Handling   // 已检测到客户端，正在执行登录动作
    };

    /* 本次登录动作对应的客户端 */
    enum class LoginTarget {
        Qatar,     // QaTar.exe
        QtPacker   // QtPacker.exe
    };

    ProcessDetector* detector;             // 进程检测器
    QNetworkAccessManager* networkManager; // 重置密码接口的网络请求
    QTimer* loginTimer;                    // 登录动作延时定时器（可被停止操作取消）
    QTimer* verifyTimer;                   // QtPacker 登录结果校验定时器
    int packerVerifyCount = 0;             // QtPacker 登录结果已校验次数
    LoginTarget activeTarget = LoginTarget::Qatar; // 当前正在处理的客户端
    LoginState loginState = LoginState::Stopped;
    QString account = "";
    QString password = "";
    bool autoStartStatus = false;

    QSystemTrayIcon *trayIcon; // 系统托盘图标
    QMenu *trayMenu;           // 托盘右键菜单
    QAction *statusAction;     // 菜单顶部：当前状态（仅展示）
    QAction *toggleAction;     // 启动 / 停止自动登录
    QAction *showAction;       // 显示主界面
    QAction *exitAction;       // 退出动作

    bool dragging = false;     // 窗口拖动中
    QPoint dragOffset;         // 拖动时鼠标相对窗口左上角的偏移

    void buildUi();            // 构建界面
    void applyStyle();         // 应用现代化样式表
    void setAutoStart(bool enabled);
    void saveSettings();
    void loadSettings();
    void setLoginState(LoginState state);  // 切换状态并同步界面显示
    void updateStatusUI();                 // 刷新状态标签与托盘提示
    void performLogin();                   // 执行登录动作（按当前客户端分发）
    void performQatarLogin();              // 执行 Qatar 客户端登录动作
    void performQtPackerLogin();           // 执行 QtPacker 客户端登录动作
    void verifyQtPackerLogin();            // 校验 QtPacker 登录结果（失败提示框 / 对话框是否关闭）
    void reportPasswordError();            // 密码错误：停止监测并弹出提示

    HWND findDialogHwnd();   // 查找密码对话框（自动适配简体/繁体/英文环境）
    HWND findPanelHwnd();    // 查找对话框内的面板控件
    HWND findAccountHwnd();  // 查找账户（Emp ID / 员工ID）下拉框
    HWND findPasswordHwnd();
    HWND findLoginHwnd();
    HWND findErrorMsg();

    HWND findPackerDialogHwnd();   // 查找 QtPacker 凭据对话框（标题含 Enter your credential）
    HWND findPackerAccountHwnd();  // 查找 QtPacker Uid 输入框
    HWND findPackerPasswordHwnd(); // 查找 QtPacker Pwd 输入框
    HWND findPackerLoginHwnd();    // 查找 QtPacker Login 按钮
    HWND findPackerErrorMsg();     // 查找 QtPacker 登录失败提示框
private slots:
    void enableAutoLogin();
    void disableAutoLogin();
    void login();
    void onExit(); // 退出程序
    void onTrayToggle(); // 托盘菜单的启停动作
    void onResetPassword(); // 调用接口重置密码（新密码通过钉钉发送）
    void showFromTray(); // 从托盘恢复主窗口
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason); // 托盘图标点击事件
protected:
    void closeEvent(QCloseEvent *event) override; // 重写关闭事件
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
};
#endif // MAINWIDGET_H
