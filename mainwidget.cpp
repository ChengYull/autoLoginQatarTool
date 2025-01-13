#include "mainwidget.h"

MainWidget::MainWidget(QWidget *parent)
    : QWidget(parent)
{
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

    this->resize(260, 120);
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

    // 创建单选框
    radio1 = new QRadioButton("简体环境", this);
    radio1->setChecked(isSimpleEnvironment);
    radio2 = new QRadioButton("繁體環境", this);
    radio2->setChecked(!isSimpleEnvironment);
    QHBoxLayout *radioLayout = new QHBoxLayout(this);
    // 将单选框添加到布局中
    radioLayout->addWidget(radio1);
    radioLayout->addWidget(radio2);
    // 创建按钮组并将单选框添加到组中
    QButtonGroup *buttonGroup = new QButtonGroup(this);
    buttonGroup->addButton(radio1);
    buttonGroup->addButton(radio2);
    mainLayout->addLayout(radioLayout);

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

    // 密码输入框改变时，为密码赋值
    connect(passwordInput, &QLineEdit::textChanged, this, [=]{
        password = passwordInput->text();
    });
    // 开机自启选择框改变时，为开机自启状态赋值
    connect(autoStartEnable, &QCheckBox::stateChanged, this, [=]{
        autoStartStatus = autoStartEnable->isChecked();
    });
    // 简体繁体选择框改变时，为其重新赋值
    connect(radio1, &QRadioButton::toggled, this,[=]{
        isSimpleEnvironment = radio1->isChecked();
    });
}

MainWidget::~MainWidget() {}
/* 重写关闭按钮 */
void MainWidget::closeEvent(QCloseEvent *event) {
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
        saveSettings(); // 保存配置
        setAutoStart(autoStartStatus);  // 设置是否开机自启
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
    settings.setValue("password", password);
    settings.setValue("autoStart", autoStartStatus);
    settings.setValue("isSimple", isSimpleEnvironment);
}
/* 加载配置 */
void MainWidget::loadSettings() {
    QSettings settings("Qtech", "autoLoginQatarApp");
    password = settings.value("password").toString();
    autoStartStatus = settings.value("autoStart").toBool();
    isSimpleEnvironment = settings.value("isSimple").toBool();
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
/* 寻找密码输入框 */
HWND MainWidget::findPasswordHwnd(){
    // 查找主窗口
    HWND hwnd;
    if(isSimpleEnvironment){
        hwnd = FindWindow(NULL, L"密码对话框");
    }else{
        hwnd = FindWindow(NULL, L"密碼對話框");
    }

    if (hwnd) {
        // 查找密码输入框
        //HWND hwndPassword = FindWindowEx(hwnd, NULL, L"Edit", L"密码");
        // 查找 panel 控件
        HWND hwndPanel = FindWindowEx(hwnd, NULL, L"wxWindowNR", NULL);
        if (hwndPanel) {
            //qDebug() << "Found panel.";
            // 查找密码输入框（位于“密码” Static 控件之后）
            HWND hwndPassword = FindWindowEx(hwndPanel, NULL, L"Edit", NULL);
            if (hwndPassword) {
                //qDebug() << "Found password input!!!!!!!!!!!!!!!!!!!!";
                return hwndPassword;
            }
        }
    }
    return 0x0;
}
/* 寻找登录按钮 */
HWND MainWidget::findLoginHwnd(){
    // 查找主窗口
    HWND hwnd;
    if(isSimpleEnvironment){
        hwnd = FindWindow(NULL, L"密码对话框");
    }else{
        hwnd = FindWindow(NULL, L"密碼對話框");
    }
    if (hwnd) {
        // 查找 panel 控件
        HWND hwndPanel = FindWindowEx(hwnd, NULL, L"wxWindowNR", NULL);
        if (hwndPanel) {
            //qDebug() << "Found panel.";
            // 查找登录按钮
            HWND hwndLogin;
            if(isSimpleEnvironment){
                hwndLogin = FindWindowEx(hwndPanel, NULL, L"Button", L"登录");
            }else{
                hwndLogin = FindWindowEx(hwndPanel, NULL, L"Button", L"Login");
            }
            if (hwndLogin) return hwndLogin;
        }
    }
    return 0x0;
}
/* 寻找登录错误信息 */
HWND MainWidget::findErrorMsg(){
    // 查找主窗口
    HWND hwnd;
    if(isSimpleEnvironment){
        hwnd = FindWindow(NULL, L"密码对话框");
    }else{
        hwnd = FindWindow(NULL, L"密碼對話框");
    }
    if (hwnd) {
        // 查找 panel 控件
        HWND hwndPanel = FindWindowEx(hwnd, NULL, L"wxWindowNR", NULL);
        if (hwndPanel) {
            // 查找登录按钮
            HWND hwndError = FindWindowEx(hwndPanel, NULL, L"Static", L"Right: 0, Area: 0x99, ec: -3(远端响应帐密错误)");
            if (hwndError) return hwndError;
        }
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
