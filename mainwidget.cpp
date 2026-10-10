#include "mainwidget.h"

#include <string>
#include <vector>

namespace {

/* 支持同时兼容的三种客户端环境 */
struct DialogEnv {
    const wchar_t *title;     // 密码对话框标题
    const wchar_t *loginBtn;  // 登录按钮文字
};

const DialogEnv kDialogEnvs[] = {
    { L"密码对话框",      L"登录"  }, // 简体环境
    { L"密碼對話框",      L"Login" }, // 繁体环境
    { L"Password Dialog", L"Login" }, // 英文环境
};

/* 获取父窗口的直接子控件 */
QList<HWND> directChildren(HWND parent, const wchar_t *className)
{
    QList<HWND> children;
    if (!parent) return children;
    HWND child = NULL;
    while ((child = FindWindowExW(parent, child, className, NULL)) != NULL) {
        children.append(child);
    }
    return children;
}

/* 获取控件在屏幕上的纵坐标 */
int windowTop(HWND hwnd)
{
    RECT rect{};
    return GetWindowRect(hwnd, &rect) ? rect.top : -1;
}

/* 在可见控件中取出最下方（纵坐标最大）的一个 */
HWND bottomMost(const QList<HWND> &handles)
{
    HWND result = 0x0;
    int maxTop = -1;
    for (HWND handle : handles) {
        if (!IsWindowVisible(handle)) continue;
        const int top = windowTop(handle);
        if (top > maxTop) {
            maxTop = top;
            result = handle;
        }
    }
    return result;
}

/* 写入下拉框内容：优先选中列表中已有的项，否则直接写入文本 */
bool setComboBoxText(HWND hwnd, const QString &text)
{
    if (!hwnd || text.isEmpty()) return false;
    const QString target = text.trimmed();
    const int count = static_cast<int>(SendMessageW(hwnd, CB_GETCOUNT, 0, 0));
    for (int i = 0; i < count; ++i) {
        const int len = static_cast<int>(SendMessageW(hwnd, CB_GETLBTEXTLEN, i, 0));
        if (len <= 0) continue;
        std::vector<wchar_t> buffer(static_cast<size_t>(len) + 1, L'\0');
        SendMessageW(hwnd, CB_GETLBTEXT, i, reinterpret_cast<LPARAM>(buffer.data()));
        if (QString::fromWCharArray(buffer.data()).trimmed() == target) {
            SendMessageW(hwnd, CB_SETCURSEL, i, 0);
            return true;
        }
    }
    const std::wstring value = target.toStdWString();
    SendMessageW(hwnd, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(value.c_str()));
    return true;
}

} // namespace

MainWidget::MainWidget(QWidget *parent)
    : QWidget(parent)
{
    this->setWindowTitle(appName + version);
    // 获取exe目录
    exePath = QCoreApplication::applicationDirPath().append("\\");
    // 最小化
    // 创建系统托盘图标
    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setIcon(QIcon(exePath + "data\\img\\appico.ico")); // 设置托盘图标
    // 创建右键菜单
    trayMenu = new QMenu(this);
    exitAction = new QAction("退出", this);
    trayMenu->addAction(exitAction);
    // 将菜单设置到托盘图标
    trayIcon->setContextMenu(trayMenu);
    tip = "AutoLoginQatarTool " + version + "\n程序状态: ";
    statueTip = enableStatus ? "运行中" : "已停止";
    trayIcon->setToolTip(tip + statueTip);
    // 连接退出动作的槽函数
    connect(exitAction, &QAction::triggered, this, &MainWidget::onExit);
    // 连接托盘图标的点击事件
    connect(trayIcon, &QSystemTrayIcon::activated, this, &MainWidget::onTrayIconActivated);
    // 显示托盘图标
    trayIcon->show();

    this->resize(300, 175);
    loadSettings();
    // 主布局
    mainLayout = new QVBoxLayout(this);
    // 程序运行状态
    statueLabel1 = new QLabel("程序状态：", this);
    statueLabel2 = new QLabel("已停止", this);
    statueLabel2->setStyleSheet("color: red;");
    statueLayout = new QHBoxLayout(this);
    statueLayout->addWidget(statueLabel1);
    statueLayout->addWidget(statueLabel2);
    statueLayout->addStretch();
    mainLayout->addLayout(statueLayout);
    // 账户输入（对应对话框中的 Emp ID / 员工ID）
    accountLayout = new QHBoxLayout(this);
    accountLabel = new QLabel("账户:", this);
    accountInput = new QLineEdit(account, this);
    accountInput->setPlaceholderText("Emp ID / 员工ID");
    accountLayout->addWidget(accountLabel);
    accountLayout->addWidget(accountInput);
    mainLayout->addLayout(accountLayout);
    // 密码输入
    passwordLayout = new QHBoxLayout(this);
    passwordLabel = new QLabel("密码:", this);
    passwordInput = new QLineEdit(password, this);
    passwordLayout->addWidget(passwordLabel);
    passwordLayout->addWidget(passwordInput);
    mainLayout->addLayout(passwordLayout);
    // 开机自启
    autoStartEnable = new QCheckBox("是否开机自启", this);
    autoStartEnable->setChecked(autoStartStatus);
    mainLayout->addWidget(autoStartEnable);

    // 按钮
    startAutoLogin = new QPushButton("启动自动登录", this);
    stopAutoLogin = new QPushButton("停止自动登录", this);
    btnLayout = new QHBoxLayout(this);
    btnLayout->addWidget(startAutoLogin);
    btnLayout->addWidget(stopAutoLogin);
    mainLayout->addLayout(btnLayout);

    // 检测器初始化
    detector = new ProcessDetector(this);

    // 启用自动登录
    connect(startAutoLogin, &QPushButton::clicked, this, &MainWidget::enableAutoLogin);

    // 停用自动登录
    connect(stopAutoLogin, &QPushButton::clicked, this, &MainWidget::disableAutoLogin);

    // 绑定定时检测信号与登录函数
    connect(detector, &ProcessDetector::loginSignal, this, &MainWidget::login);

    // 账户输入框改变时，为账户赋值
    connect(accountInput, &QLineEdit::textChanged, this, [=]{
        account = accountInput->text();
    });
    // 密码输入框改变时，为密码赋值
    connect(passwordInput, &QLineEdit::textChanged, this, [=]{
        password = passwordInput->text();
    });
    // 开机自启选择框改变时，为开机自启状态赋值
    connect(autoStartEnable, &QCheckBox::stateChanged, this, [=]{
        autoStartStatus = autoStartEnable->isChecked();
    });
}

MainWidget::~MainWidget() {}
/* 重写关闭按钮 */
void MainWidget::closeEvent(QCloseEvent *event) {
    saveSettings(); // 保存配置
    setAutoStart(autoStartStatus);  // 设置是否开机自启
    // 隐藏主窗口
    this->hide();

    // 显示系统托盘图标
    trayIcon->show();

    // 阻止默认关闭行为
    event->ignore();
}
/* 右键菜单退出槽函数 */
void MainWidget::onExit() {
    this->show();
    // 确认退出
    if (QMessageBox::question(this, "退出", "确定要退出程序吗？", QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
        trayIcon->hide(); // 隐藏托盘图标
        if(enableStatus) disableAutoLogin();
        QApplication::quit(); // 退出程序
    }
}
/* 托盘图标点击事件 */
void MainWidget::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason){
    // 判断交互类型
    if (reason == QSystemTrayIcon::DoubleClick || reason == QSystemTrayIcon::Trigger) {
        this->show(); // 显示主窗口
        this->activateWindow(); // 激活窗口
    }
}
/* 设置开机自启 */
void MainWidget::setAutoStart(bool enabled) {
    QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    if (enabled) {
        settings.setValue("autoLoginQatarApp", QCoreApplication::applicationFilePath().replace('/', '\\'));
    } else {
        settings.remove("autoLoginQatarApp");
    }
}
/* 保存配置 */
void MainWidget::saveSettings() {
    QSettings settings("Qtech", "autoLoginQatarApp");
    settings.setValue("account", account);
    settings.setValue("password", password);
    settings.setValue("autoStart", autoStartStatus);
}
/* 加载配置 */
void MainWidget::loadSettings() {
    QSettings settings("Qtech", "autoLoginQatarApp");
    account = settings.value("account").toString();
    password = settings.value("password").toString();
    autoStartStatus = settings.value("autoStart").toBool();
}
/* 开启自动登录 */
void MainWidget::enableAutoLogin(){
    if(!enableStatus){
        // 卡控密码为空情况
        if(password.isEmpty()){
            QMessageBox::critical(this, "密码为空！", "请先配置密码...");
            return;
        }
        statueLabel2->setText("运行中");
        statueLabel2->setStyleSheet("color: green;");

        detector->startDetection();
        enableStatus = true;
        statueTip = enableStatus ? "运行中" : "已停止";
        trayIcon->setToolTip(tip + statueTip);
    }else{
        QMessageBox::critical(this, "自动登录已启用！", "自动登录已启用...");
    }
}
/* 停止自动登录 */
void MainWidget::disableAutoLogin(){
    if(enableStatus){
        statueLabel2->setText("已停止");
        statueLabel2->setStyleSheet("color: red;");
        detector->stopDetection();
        enableStatus = false;
        statueTip = enableStatus ? "运行中" : "已停止";
        trayIcon->setToolTip(tip + statueTip);
    }else{
        QMessageBox::critical(this, "自动登录未启用！", "自动登录未启用...");
    }
}
/* 查找密码对话框（自动适配简体/繁体/英文环境，不再依赖环境选择） */
HWND MainWidget::findDialogHwnd(){
    for (const DialogEnv &env : kDialogEnvs) {
        HWND hwnd = FindWindowW(NULL, env.title);
        if (hwnd) {
            //qDebug() << "Found dialog:" << QString::fromWCharArray(env.title);
            return hwnd;
        }
    }
    return 0x0;
}
/* 查找对话框内的面板控件 */
HWND MainWidget::findPanelHwnd(){
    HWND hwnd = findDialogHwnd();
    if(!hwnd) return 0x0;
    return FindWindowEx(hwnd, NULL, L"wxWindowNR", NULL);
}
/* 寻找密码输入框（面板中最下方的输入框） */
HWND MainWidget::findPasswordHwnd(){
    // 密码行位于对话框最底部，取最下方的 Edit 控件最稳妥
    return bottomMost(directChildren(findPanelHwnd(), L"Edit"));
}
/* 寻找账户（Emp ID / 员工ID）下拉框（密码输入框上方最近的下拉框） */
HWND MainWidget::findAccountHwnd(){
    HWND hwndPassword = findPasswordHwnd();
    if(!hwndPassword) return 0x0;
    const int passwordTop = windowTop(hwndPassword);
    HWND result = 0x0;
    int maxTop = -1;
    for (HWND hwndCombo : directChildren(findPanelHwnd(), L"ComboBox")) {
        if (!IsWindowVisible(hwndCombo)) continue;
        const int top = windowTop(hwndCombo);
        // 服务器、端口也在密码上方，取纵坐标最大的一个即为账户行
        if (top < passwordTop && top > maxTop) {
            maxTop = top;
            result = hwndCombo;
        }
    }
    return result;
}
/* 寻找登录按钮 */
HWND MainWidget::findLoginHwnd(){
    HWND hwndPanel = findPanelHwnd();
    if(!hwndPanel) return 0x0;
    // 「加密狗 / Keypro」按钮不会与登录按钮文案重名，逐个环境尝试即可
    for (const DialogEnv &env : kDialogEnvs) {
        HWND hwndLogin = FindWindowEx(hwndPanel, NULL, L"Button", env.loginBtn);
        if (hwndLogin) return hwndLogin;
    }
    return 0x0;
}
/* 寻找登录错误信息 */
HWND MainWidget::findErrorMsg(){
    for (HWND hwndStatic : directChildren(findPanelHwnd(), L"Static")) {
        if (!IsWindowVisible(hwndStatic)) continue;
        wchar_t buffer[512] = {0};
        GetWindowTextW(hwndStatic, buffer, 511);
        const QString text = QString::fromWCharArray(buffer);
        // 三种环境下的提示均为 "Right: 0, Area: 0x99, ec: -3(...)"，
        // 该返回码串不随界面语言变化，故只匹配这一段
        if (text.contains("ec: -3")) return hwndStatic;
    }
    return 0x0;
}
/* 登录 */
void MainWidget::login(){
    qDebug() << "auto Login!";
    disableAutoLogin();
    // 使用 QTimer 延时 2 秒
    QTimer::singleShot(2000, this, [this]() {
        HWND hwndPassword = findPasswordHwnd();
        if (hwndPassword) {

            // 自动填入账户（Emp ID / 员工ID），未配置账户时保持对话框原有内容
            const QString accountText = account.trimmed();
            if (!accountText.isEmpty()) {
                HWND hwndAccount = findAccountHwnd();
                if (hwndAccount) setComboBoxText(hwndAccount, accountText);
            }

            // 先清空原密码内容
            SendMessage(hwndPassword, WM_SETTEXT, 0, (LPARAM)L"");
            // 输入密码
            for (QChar ch : password) {
                SendMessage(hwndPassword, WM_CHAR, ch.unicode(), 0);
            }

            HWND hwndLogin = findLoginHwnd();
            if(hwndLogin){
                // 发送 BM_CLICK 消息，模拟点击按钮
                SendMessage(hwndLogin, BM_CLICK, 0, 0);
            }
            if(findErrorMsg()){
                // 窗口置顶
                this->setWindowFlag(Qt::WindowStaysOnTopHint);
                this->show();
                QMessageBox::critical(this, "密码错误", "密码与工号不匹配, 请重新配置密码...");
                return;
            }
        }
        enableAutoLogin();
    });
}
