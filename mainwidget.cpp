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
    statueTip = enableStatue ? "运行中" : "已停止";
    trayIcon->setToolTip(tip + statueTip);
    // 连接退出动作的槽函数
    connect(exitAction, &QAction::triggered, this, &MainWidget::onExit);
    // 连接托盘图标的点击事件
    connect(trayIcon, &QSystemTrayIcon::activated, this, &MainWidget::onTrayIconActivated);
    // 显示托盘图标
    trayIcon->show();

    this->resize(260, 120);
    loadSettings(password, autoStartStatue);
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
    autoStartEnable->setChecked(autoStartStatue);
    mainLayout->addWidget(autoStartEnable);

    // 创建单选框
    radio1 = new QRadioButton("简体环境", this);
    radio1->setChecked(isSimpleEnvironment);
    radio2 = new QRadioButton("繁體環境", this);
    radio1->setChecked(!isSimpleEnvironment);
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

    connect(startAutoLogin, &QPushButton::clicked, this, &MainWidget::enableAutoLogin);

    connect(stopAutoLogin, &QPushButton::clicked, this, &MainWidget::disableAutoLogin);

    connect(detector, &ProcessDetector::loginSignal, this, &MainWidget::login);
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
    // 确认退出
    if (QMessageBox::question(this, "退出", "确定要退出程序吗？", QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
        trayIcon->hide(); // 隐藏托盘图标
        if(enableStatue) disableAutoLogin();
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
void MainWidget::saveSettings(const QString &password, bool autoStart) {
    QSettings settings("Qtech", "autoLoginQatarApp");
    settings.setValue("password", password);
    settings.setValue("autoStart", autoStart);
}
/* 加载配置 */
void MainWidget::loadSettings(QString &password, bool &autoStart) {
    QSettings settings("Qtech", "autoLoginQatarApp");
    password = settings.value("password").toString();
    autoStart = settings.value("autoStart").toBool();
}
/* 开启自动登录 */
void MainWidget::enableAutoLogin(){
    if(!enableStatue){
        statueLabel2->setText("运行中");
        statueLabel2->setStyleSheet("color: green;");
        password = passwordInput->text();
        autoStartStatue = autoStartEnable->isChecked();
        saveSettings(password, autoStartStatue); // 保存设置
        setAutoStart(autoStartStatue);  // 设置是否开机自启
        isSimpleEnvironment = radio1->isChecked();
        detector->startDetection();
        enableStatue = true;
        statueTip = enableStatue ? "运行中" : "已停止";
        trayIcon->setToolTip(tip + statueTip);
    }else{
        QMessageBox::critical(this, "自动登录已启用！", "自动登录已启用...");
    }
}
/* 停止自动登录 */
void MainWidget::disableAutoLogin(){
    if(enableStatue){
        statueLabel2->setText("已停止");
        statueLabel2->setStyleSheet("color: red;");
        detector->stopDetection();
        enableStatue = false;
        statueTip = enableStatue ? "运行中" : "已停止";
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
/* 登录 */
void MainWidget::login(){
    qDebug() << "auto Login!";
    //disableAutoLogin();
// 使用 QTimer 延时 2 秒
    QTimer::singleShot(2000, this, [this]() {
        HWND hwndPassword = findPasswordHwnd();
        if (hwndPassword) {
            // 输入密码
            for (QChar ch : password) {
                SendMessage(hwndPassword, WM_CHAR, ch.unicode(), 0);
            }

            HWND hwndLogin = findLoginHwnd();
            if(hwndLogin){
                // 发送 BM_CLICK 消息，模拟点击按钮
                SendMessage(hwndLogin, BM_CLICK, 0, 0);
            }
        }
    });
}
