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
class MainWidget : public QWidget
{
    Q_OBJECT

public:
    MainWidget(QWidget *parent = nullptr);
    ~MainWidget();

private:
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
    bool enableStatue = false;
    QString password = "";
    bool autoStartStatue = false;
    bool isSimpleEnvironment = true;

    void setAutoStart(bool enabled);
    void saveSettings(const QString &password, bool autoStart);
    void loadSettings(QString &password, bool &autoStart);
    HWND findPasswordHwnd();
    HWND findLoginHwnd();
private slots:
    void enableAutoLogin();
    void disableAutoLogin();
    void login();
};
#endif // MAINWIDGET_H
