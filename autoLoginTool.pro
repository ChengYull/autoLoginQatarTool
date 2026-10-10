QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets svg network

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwidget.cpp \
    processdetector.cpp \
    toggleswitch.cpp

HEADERS += \
    mainwidget.h \
    processdetector.h \
    toggleswitch.h

RESOURCES += \
    resources.qrc

# 由 assets/icons/app.svg 生成的多尺寸图标，内嵌进 exe（资源管理器 / 快捷方式 / 任务栏固定）
RC_ICONS = $$PWD/assets/icons/app.ico

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
