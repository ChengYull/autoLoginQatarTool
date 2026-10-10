#include "mainwidget.h"

#include <QGraphicsDropShadowEffect>
#include <QMouseEvent>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPainter>
#include <QPixmap>
#include <QSvgRenderer>
#include <QUrlQuery>

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

/* 重置密码接口：GET ?uid=工号，成功时响应体只有一个字符 1，重置后的密码会发到钉钉 */
const char kResetPasswordUrl[] = "http://10.170.1.137:18081/api/user/forgetpwd";
constexpr int kResetPasswordTimeoutMs = 10000;

/* 把内嵌的 SVG 渲染成多尺寸 QIcon。
 * 直接渲染而不是依赖 Qt 的 SVG 图片插件，可保证任意 DPI 下的清晰度，
 * 也不必在部署时额外分发 iconengines/imageformats 插件。 */
QIcon renderSvgIcon(const QString &path)
{
    QIcon icon;
    QSvgRenderer renderer(path);
    if (!renderer.isValid()) return icon;

    const int sizes[] = {16, 20, 24, 32, 48, 64, 128, 256};
    for (int size : sizes) {
        QPixmap pixmap(size, size);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        renderer.render(&painter, QRectF(0, 0, size, size));
        painter.end();
        icon.addPixmap(pixmap);
    }
    return icon;
}

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

QIcon MainWidget::appIcon()
{
    return renderSvgIcon(QStringLiteral(":/icons/app.svg"));
}

MainWidget::MainWidget(QWidget *parent)
    : QWidget(parent)
{
    loadSettings();

    /* 系统托盘 */
    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setIcon(appIcon());
    tip = appName + "\n程序状态: ";
    trayIcon->setToolTip(tip + "已停止");

    trayMenu = new QMenu(this);
    trayMenu->setStyleSheet(
        "QMenu { background:#ffffff; border:1px solid #e6ebf2; border-radius:8px; padding:6px; }"
        "QMenu::item { padding:6px 22px; border-radius:6px; color:#334155; font-size:12px; }"
        "QMenu::item:selected { background:#eff6ff; color:#1d4ed8; }"
        "QMenu::item:disabled { color:#94a3b8; }"
        "QMenu::separator { height:1px; background:#eef2f7; margin:4px 8px; }");

    // 菜单顶部展示当前状态（仅展示，不可点击）
    statusAction = new QAction(this);
    statusAction->setEnabled(false);
    // 启停动作，文案随当前状态在「启动 / 停止」之间切换
    toggleAction = new QAction(this);
    showAction = new QAction("显示主界面", this);
    exitAction = new QAction("退出", this);

    trayMenu->addAction(statusAction);
    trayMenu->addSeparator();
    trayMenu->addAction(toggleAction);
    trayMenu->addSeparator();
    trayMenu->addAction(showAction);
    trayMenu->addAction(exitAction);
    trayIcon->setContextMenu(trayMenu);

    connect(toggleAction, &QAction::triggered, this, &MainWidget::onTrayToggle);
    connect(showAction, &QAction::triggered, this, &MainWidget::showFromTray);
    connect(exitAction, &QAction::triggered, this, &MainWidget::onExit);
    connect(trayIcon, &QSystemTrayIcon::activated, this, &MainWidget::onTrayIconActivated);
    trayIcon->show();

    buildUi();
    applyStyle();

    // 重置密码接口
    networkManager = new QNetworkAccessManager(this);

    // 检测器初始化
    detector = new ProcessDetector(this);
    // 登录延时定时器：单次触发，可被「停止自动登录」提前取消
    loginTimer = new QTimer(this);
    loginTimer->setSingleShot(true);
    connect(loginTimer, &QTimer::timeout, this, &MainWidget::performLogin);

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
    // 开机自启开关改变时，为开机自启状态赋值
    connect(autoStartSwitch, &ToggleSwitch::toggled, this, [=](bool checked){
        autoStartStatus = checked;
    });

    // 同步初始状态显示
    updateStatusUI();
}

MainWidget::~MainWidget() {}

/* 构建现代化界面 */
void MainWidget::buildUi()
{
    // 无边框 + 半透明，自行绘制圆角卡片与投影
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setWindowTitle(appName); // 标题不再携带版本号
    setWindowIcon(appIcon());

    QVBoxLayout *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(14, 14, 14, 14); // 预留投影空间

    container = new QWidget(this);
    container->setObjectName("container");
    rootLayout->addWidget(container);

    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(container);
    shadow->setBlurRadius(20);
    shadow->setOffset(0, 3);
    shadow->setColor(QColor(15, 23, 42, 70));
    container->setGraphicsEffect(shadow);

    QVBoxLayout *containerLayout = new QVBoxLayout(container);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->setSpacing(0);

    /* ---------- 自定义标题栏 ---------- */
    titleBar = new QWidget(container);
    titleBar->setObjectName("titleBar");
    titleBar->setFixedHeight(42);
    titleBar->setAttribute(Qt::WA_StyledBackground, true);

    QHBoxLayout *titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(16, 0, 10, 0);
    titleLayout->setSpacing(8);

    QLabel *titleIcon = new QLabel(titleBar);
    titleIcon->setFixedSize(18, 18);
    titleIcon->setPixmap(appIcon().pixmap(18, 18));
    titleIcon->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    titleLayout->addWidget(titleIcon);

    QLabel *titleLabel = new QLabel("自动登录工具", titleBar);
    titleLabel->setObjectName("titleLabel");
    titleLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();

    QPushButton *minimizeBtn = new QPushButton("—", titleBar);
    minimizeBtn->setObjectName("winBtnMin");
    minimizeBtn->setFixedSize(30, 28);
    minimizeBtn->setCursor(Qt::PointingHandCursor);
    minimizeBtn->setToolTip("最小化到托盘");
    connect(minimizeBtn, &QPushButton::clicked, this, [this]{
        this->hide();
        trayIcon->show();
    });

    QPushButton *closeBtn = new QPushButton("✕", titleBar);
    closeBtn->setObjectName("winBtnClose");
    closeBtn->setFixedSize(30, 28);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setToolTip("关闭到托盘");
    connect(closeBtn, &QPushButton::clicked, this, &MainWidget::close);

    titleLayout->addWidget(minimizeBtn);
    titleLayout->addWidget(closeBtn);

    /* ---------- 内容区 ---------- */
    QWidget *content = new QWidget(container);
    content->setAttribute(Qt::WA_StyledBackground, true);
    QVBoxLayout *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(16, 2, 16, 16);
    contentLayout->setSpacing(10);

    // 运行状态卡片
    QFrame *statusCard = new QFrame(content);
    statusCard->setObjectName("statusCard");
    statusCard->setAttribute(Qt::WA_StyledBackground, true);
    QHBoxLayout *statusLayout = new QHBoxLayout(statusCard);
    statusLayout->setContentsMargins(12, 9, 12, 9);
    statusLayout->setSpacing(8);

    statusDot = new QLabel(statusCard);
    statusDot->setFixedSize(10, 10);
    statusLayout->addWidget(statusDot);

    QLabel *statusTitle = new QLabel("运行状态", statusCard);
    statusTitle->setObjectName("statusTitle");
    statusLayout->addWidget(statusTitle);
    statusLayout->addStretch();

    // 徽标显式给定尺寸：QLabel 的 sizeHint 不会把 QSS 的上下 padding 计入，
    // 交给布局自适应会出现文字被裁掉一截的情况
    statusBadge = new QLabel(statusCard);
    statusBadge->setAlignment(Qt::AlignCenter);
    statusBadge->setFixedHeight(22);
    statusBadge->setMinimumWidth(64);
    statusLayout->addWidget(statusBadge, 0, Qt::AlignVCenter);

    contentLayout->addWidget(statusCard);

    // 账户输入（对应对话框中的 Emp ID / 员工ID），标签与输入框同一行
    QHBoxLayout *accountLayout = new QHBoxLayout();
    accountLayout->setSpacing(8);
    QLabel *accountLabel = new QLabel("账户", content);
    accountLabel->setObjectName("fieldLabel");
    accountLabel->setFixedWidth(36);
    accountInput = new QLineEdit(content);
    accountInput->setObjectName("input");
    accountInput->setPlaceholderText("Emp ID / 员工ID");
    accountInput->setText(account);
    accountInput->setClearButtonEnabled(true);
    accountLayout->addWidget(accountLabel);
    accountLayout->addWidget(accountInput, 1);
    contentLayout->addLayout(accountLayout);

    // 密码输入，同样与标签同一行；明文切换改为输入框内右侧的小眼睛图标
    QHBoxLayout *passwordLayout = new QHBoxLayout();
    passwordLayout->setSpacing(8);
    QLabel *passwordLabel = new QLabel("密码", content);
    passwordLabel->setObjectName("fieldLabel");
    passwordLabel->setFixedWidth(36);
    passwordInput = new QLineEdit(content);
    passwordInput->setObjectName("input");
    passwordInput->setEchoMode(QLineEdit::Password);
    passwordInput->setPlaceholderText("请输入客户端登录密码");
    passwordInput->setText(password);
    passwordLayout->addWidget(passwordLabel);
    passwordLayout->addWidget(passwordInput, 1);

    passwordEyeAction = passwordInput->addAction(renderSvgIcon(QStringLiteral(":/icons/eye.svg")),
                                                 QLineEdit::TrailingPosition);
    passwordEyeAction->setToolTip("显示密码");
    connect(passwordEyeAction, &QAction::triggered, this, [this]{
        const bool hidden = passwordInput->echoMode() == QLineEdit::Password;
        passwordInput->setEchoMode(hidden ? QLineEdit::Normal : QLineEdit::Password);
        passwordEyeAction->setIcon(renderSvgIcon(hidden ? QStringLiteral(":/icons/eye-off.svg")
                                                        : QStringLiteral(":/icons/eye.svg")));
        passwordEyeAction->setToolTip(hidden ? "隐藏密码" : "显示密码");
    });

    // 重置密码：占用的就是原来「显示」按钮的位置
    resetPasswordBtn = new QPushButton("重置", content);
    resetPasswordBtn->setObjectName("resetBtn");
    resetPasswordBtn->setCursor(Qt::PointingHandCursor);
    resetPasswordBtn->setFixedWidth(46);
    resetPasswordBtn->setToolTip("调用重置接口，新密码将通过钉钉发送");
    connect(resetPasswordBtn, &QPushButton::clicked, this, &MainWidget::onResetPassword);
    passwordLayout->addWidget(resetPasswordBtn);
    contentLayout->addLayout(passwordLayout);

    // 开机自启
    QWidget *switchRow = new QWidget(content);
    QHBoxLayout *switchLayout = new QHBoxLayout(switchRow);
    switchLayout->setContentsMargins(0, 2, 0, 2);
    switchLayout->setSpacing(10);
    QVBoxLayout *switchTextLayout = new QVBoxLayout();
    switchTextLayout->setContentsMargins(0, 0, 0, 0);
    switchTextLayout->setSpacing(2);
    QLabel *switchTitle = new QLabel("开机自启动", switchRow);
    switchTitle->setObjectName("switchTitle");
    QLabel *switchDesc = new QLabel("随 Windows 启动后常驻系统托盘", switchRow);
    switchDesc->setObjectName("switchDesc");
    switchTextLayout->addWidget(switchTitle);
    switchTextLayout->addWidget(switchDesc);
    switchLayout->addLayout(switchTextLayout);
    switchLayout->addStretch();

    autoStartSwitch = new ToggleSwitch(switchRow);
    autoStartSwitch->setChecked(autoStartStatus);
    switchLayout->addWidget(autoStartSwitch, 0, Qt::AlignVCenter);
    contentLayout->addWidget(switchRow);

    contentLayout->addStretch();

    // 操作按钮
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(10);
    startAutoLogin = new QPushButton("启动", content);
    startAutoLogin->setObjectName("primaryBtn");
    startAutoLogin->setCursor(Qt::PointingHandCursor);
    startAutoLogin->setMinimumHeight(32);
    stopAutoLogin = new QPushButton("停止", content);
    stopAutoLogin->setObjectName("secondaryBtn");
    stopAutoLogin->setCursor(Qt::PointingHandCursor);
    stopAutoLogin->setMinimumHeight(32);
    btnLayout->addWidget(startAutoLogin, 1);
    btnLayout->addWidget(stopAutoLogin, 1);
    contentLayout->addLayout(btnLayout);

    containerLayout->addWidget(titleBar);
    containerLayout->addWidget(content, 1);

    // 按内容自适应高度，宽度保持 340
    const QSize hint = sizeHint();
    setFixedSize(qMax(340, hint.width()), qMax(280, hint.height()));
}

/* 现代化样式表 */
void MainWidget::applyStyle()
{
    QFont font;
    font.setFamilies({"Microsoft YaHei UI", "Segoe UI", "Microsoft YaHei"});
    font.setPointSize(9);
    setFont(font);

    setStyleSheet(R"(
        #container        { background:#ffffff; border:1px solid #e6ebf2; border-radius:14px; }
        #titleBar         { background:transparent; }
        #titleLabel       { color:#0f172a; font-size:13px; font-weight:600; }

        #winBtnMin, #winBtnClose { border:none; background:transparent; color:#94a3b8;
                                   border-radius:7px; font-size:14px; }
        #winBtnMin:hover  { background:#f1f5f9; color:#0f172a; }
        #winBtnClose:hover{ background:#ef4444; color:#ffffff; }

        #statusCard       { background:#f8fafc; border:1px solid #eef2f7; border-radius:10px; }
        #statusTitle      { color:#334155; font-size:12px; font-weight:600; }

        #fieldLabel       { color:#334155; font-size:12px; font-weight:600; }
        #input            { background:#ffffff; border:1px solid #dbe2ea; border-radius:8px;
                            padding:6px 10px; font-size:12px; color:#0f172a; }
        #input:hover      { border-color:#c3ccd8; }
        #input:focus      { border-color:#2563eb; }
        #input:disabled   { background:#f8fafc; color:#94a3b8; border-color:#eef2f7; }

        #resetBtn         { background:#ffffff; color:#2563eb; border:1px solid #dbe2ea;
                            border-radius:8px; font-size:12px; font-weight:600; padding:6px 0; }
        #resetBtn:hover   { background:#eff6ff; border-color:#93c5fd; }
        #resetBtn:pressed { background:#dbeafe; }
        #resetBtn:disabled { background:#f8fafc; color:#cbd5e1; border-color:#eef2f7; }

        #switchTitle      { color:#334155; font-size:12px; font-weight:600; }
        #switchDesc       { color:#94a3b8; font-size:11px; }

        #primaryBtn       { background:#2563eb; color:#ffffff; border:none; border-radius:8px;
                            font-size:12px; font-weight:600; padding:7px 0; }
        #primaryBtn:hover { background:#1d4ed8; }
        #primaryBtn:pressed { background:#1e40af; }
        #primaryBtn:disabled { background:#e2e8f0; color:#94a3b8; }

        #secondaryBtn     { background:#ffffff; color:#475569; border:1px solid #dbe2ea;
                            border-radius:8px; font-size:12px; font-weight:600; padding:7px 0; }
        #secondaryBtn:hover { background:#f8fafc; border-color:#94a3b8; color:#0f172a; }
        #secondaryBtn:pressed { background:#f1f5f9; }
        #secondaryBtn:disabled { background:#f8fafc; color:#cbd5e1; border-color:#eef2f7; }
    )");

    // 占位符颜色（QSS 不支持，用调色板设置）
    QPalette palette = accountInput->palette();
    palette.setColor(QPalette::PlaceholderText, QColor("#94a3b8"));
    accountInput->setPalette(palette);
    passwordInput->setPalette(palette);
}

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

/* 拖动无边框窗口（仅标题栏区域响应） */
void MainWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && titleBar
        && titleBar->rect().contains(titleBar->mapFrom(this, event->position().toPoint()))) {
        dragging = true;
        dragOffset = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void MainWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPosition().toPoint() - dragOffset);
        event->accept();
        return;
    }
    QWidget::mouseMoveEvent(event);
}

void MainWidget::mouseReleaseEvent(QMouseEvent *event)
{
    dragging = false;
    QWidget::mouseReleaseEvent(event);
}

/* 右键菜单退出槽函数 */
void MainWidget::onExit() {
    this->show();
    // 确认退出
    if (QMessageBox::question(this, "退出", "确定要退出程序吗？", QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
        trayIcon->hide(); // 隐藏托盘图标
        // 未停止状态下退出，需要先取消监测与待执行的登录动作
        if(loginState != LoginState::Stopped) disableAutoLogin();
        QApplication::quit(); // 退出程序
    }
}
/* 托盘菜单的启停动作 */
void MainWidget::onTrayToggle(){
    if(loginState == LoginState::Stopped){
        // 密码为空时会弹出提示，先恢复主窗口，避免提示框挂在隐藏窗口上
        if(password.isEmpty()) showFromTray();
        enableAutoLogin();
    } else {
        disableAutoLogin();
    }
}
/* 调用接口重置密码：参数为当前账户，新密码会通过钉钉发送 */
void MainWidget::onResetPassword(){
    const QString uid = accountInput->text().trimmed();
    if(uid.isEmpty()){
        QMessageBox::warning(this, "账户为空", "请先填写账户，再重置密码。");
        return;
    }

    QUrl url(QString::fromLatin1(kResetPasswordUrl));
    QUrlQuery query;
    query.addQueryItem("uid", uid);
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setTransferTimeout(kResetPasswordTimeoutMs);

    // 请求期间禁用按钮，避免重复提交
    resetPasswordBtn->setEnabled(false);

    QNetworkReply *reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]{
        reply->deleteLater();
        resetPasswordBtn->setEnabled(true);

        if(reply->error() != QNetworkReply::NoError){
            QMessageBox::critical(this, "重置失败", "请求失败：" + reply->errorString());
            return;
        }

        // 接口成功时的响应体只有一个字符 1
        const QString body = QString::fromUtf8(reply->readAll()).trimmed();
        if(body == "1"){
            QMessageBox::information(this, "重置成功", "密码已重置，新密码将通过钉钉发送，请查收。");
        } else {
            QMessageBox::critical(this, "重置失败",
                                  body.isEmpty() ? QString("接口未返回内容，请检查网络或接口地址。")
                                                 : "接口返回异常：" + body);
        }
    });
}
/* 从托盘恢复主窗口 */
void MainWidget::showFromTray(){
    this->show();
    this->raise();
    this->activateWindow();
}
/* 托盘图标点击事件 */
void MainWidget::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason){
    // 判断交互类型
    if (reason == QSystemTrayIcon::DoubleClick || reason == QSystemTrayIcon::Trigger) {
        showFromTray();
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
/* 切换状态并同步界面显示 */
void MainWidget::setLoginState(LoginState state) {
    loginState = state;
    updateStatusUI();
}
/* 刷新状态标签、按钮可用性与托盘提示 */
void MainWidget::updateStatusUI() {
    QString dotColor;
    QString badgeStyle;
    QString badgeText;

    switch (loginState) {
    case LoginState::Running:
        dotColor = "#22c55e";
        badgeText = "运行中";
        badgeStyle = "background:#dcfce7; color:#15803d;";
        break;
    case LoginState::Handling:
        dotColor = "#f59e0b";
        badgeText = "登录中";
        badgeStyle = "background:#fef3c7; color:#b45309;";
        break;
    case LoginState::Stopped:
    default:
        dotColor = "#94a3b8";
        badgeText = "已停止";
        badgeStyle = "background:#f1f5f9; color:#64748b;";
        break;
    }

    statusDot->setStyleSheet(QString("background:%1; border-radius:5px;").arg(dotColor));
    statusBadge->setText(badgeText);
    statusBadge->setStyleSheet(badgeStyle + "border-radius:11px; padding:0 12px; font-size:11px; font-weight:600;");

    // 未启用时只能「启动」，已启用/登录中时只能「停止」
    const bool running = (loginState != LoginState::Stopped);
    startAutoLogin->setEnabled(!running);
    stopAutoLogin->setEnabled(running);

    // 运行中不允许修改账户与密码，避免监测期间的登录凭据与实际配置不一致
    accountInput->setEnabled(!running);
    passwordInput->setEnabled(!running);
    passwordEyeAction->setEnabled(!running);

    // 同步托盘菜单：顶部状态 + 启停动作文案
    statusAction->setText("当前状态：" + badgeText);
    toggleAction->setText(running ? "停止" : "启动");

    statueTip = badgeText;
    trayIcon->setToolTip(tip + statueTip);
}
/* 开启自动登录 */
void MainWidget::enableAutoLogin(){
    if(loginState == LoginState::Running){
        QMessageBox::critical(this, "自动登录已启用！", "自动登录已启用...");
        return;
    }
    // 卡控密码为空情况
    if(password.isEmpty()){
        QMessageBox::critical(this, "密码为空！", "请先配置密码...");
        return;
    }
    detector->startDetection();
    setLoginState(LoginState::Running);
}
/* 停止自动登录（处于“登录中”时同样可以停止，并会取消待执行的登录动作） */
void MainWidget::disableAutoLogin(){
    if(loginState == LoginState::Stopped){
        QMessageBox::critical(this, "自动登录未启用！", "自动登录未启用...");
        return;
    }
    // 取消仍在等待中的登录动作，避免停止后又被延时回调重新拉起
    if(loginTimer->isActive()) loginTimer->stop();
    detector->stopDetection();
    setLoginState(LoginState::Stopped);
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
/* 检测到客户端启动 */
void MainWidget::login(){
    // 未处于监测状态，或上一次登录动作仍在处理中时，忽略本次信号
    if(loginState != LoginState::Running) return;
    // 密码对话框尚未出现时不做任何处理，避免登录成功后客户端仍驻留
    // 而导致监测被无意义地反复暂停与恢复
    if(!findDialogHwnd()) return;

    qDebug() << "auto Login!";
    detector->stopDetection();
    setLoginState(LoginState::Handling);
    // 延时 2 秒，等待对话框与控件完全就绪
    loginTimer->start(2000);
}
/* 执行登录动作（等待期间可被「停止自动登录」取消） */
void MainWidget::performLogin(){
    // 等待期间若已被停止，则本次登录不再执行，也不再恢复监测
    if(loginState != LoginState::Handling) return;

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
            // 密码错误：停止监测，等用户重新配置密码后手动启动，避免反复重试
            setLoginState(LoginState::Stopped);
            // 窗口置顶并重新显示（切换窗口标志后需要再次 show）
            setWindowFlag(Qt::FramelessWindowHint, true);
            setWindowFlag(Qt::WindowStaysOnTopHint, true);
            this->show();
            QMessageBox::critical(this, "密码错误", "密码与工号不匹配, 请重新配置密码...");
            return;
        }
    }
    // 登录动作执行完毕，恢复监测
    detector->startDetection();
    setLoginState(LoginState::Running);
}
