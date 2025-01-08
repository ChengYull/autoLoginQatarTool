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

    void setAutoStart(bool enabled);
    void saveSettings(const QString &password, bool autoStart);
    void loadSettings(QString &password, bool &autoStart);
};
#endif // MAINWIDGET_H
