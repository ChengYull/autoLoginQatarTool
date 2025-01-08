#include "mainwidget.h"

MainWidget::MainWidget(QWidget *parent)
    : QWidget(parent)
{
    mainLayout = new QVBoxLayout(this);
    passwordLayout = new QHBoxLayout(this);
    passwordLabel = new QLabel("密码:", this);
    passwordInput = new QLineEdit(this);
    settingPasswordBtn = new QPushButton("设置密码", this);
    passwordLayout->addWidget(passwordLabel);
    passwordLayout->addWidget(passwordInput);
    passwordLayout->addWidget(settingPasswordBtn);

    autoStartEnable = new QCheckBox("是否开机自启", this);
    startAutoLogin = new QPushButton("启动自动登录", this);

    mainLayout->addLayout(passwordLayout);
    mainLayout->addWidget(autoStartEnable);
    mainLayout->addWidget(startAutoLogin, 0, Qt::AlignHCenter);
}

MainWidget::~MainWidget() {}

/* 设置开机自启 */
void MainWidget::setAutoStart(bool enabled) {
    QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    if (enabled) {
        settings.setValue("autoLoginQatarApp", QCoreApplication::applicationFilePath().replace('/', '\\'));
    } else {
        settings.remove("autoLoginQatarApp");
    }
}

void MainWidget::saveSettings(const QString &password, bool autoStart) {
    QSettings settings("Qtech", "autoLoginQatarApp");
    settings.setValue("password", password);
    settings.setValue("autoStart", autoStart);
}

void MainWidget::loadSettings(QString &password, bool &autoStart) {
    QSettings settings("Qtech", "autoLoginQatarApp");
    password = settings.value("password").toString();
    autoStart = settings.value("autoStart").toBool();
}
