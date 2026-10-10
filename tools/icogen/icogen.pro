QT += core gui svg
CONFIG += c++17 console
TARGET = icogen
SOURCES += main.cpp

# 由 assets/icons/app.svg 生成 assets/icons/app.ico
DEFINES += ASSETS_DIR=\\\"$$PWD/../../assets/icons\\\"
