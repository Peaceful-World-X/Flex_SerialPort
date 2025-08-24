# Qt模块配置
QT += core gui widgets serialport

# 第三方库
# 注意：如果系统没有yaml-cpp，可以使用QSettings替代
# CONFIG += yaml-cpp

# C++标准
CONFIG += c++17

# 目标名称和模板
TARGET = FlexSerialPort
TEMPLATE = app

# 源文件
SOURCES += \
    main.cpp \
    mainwindow.cpp \
    configmanager.cpp \
    serialportmanager.cpp \
    logmanager.cpp \
    buttondatabase.cpp

# 头文件
HEADERS += \
    mainwindow.h \
    configmanager.h \
    serialportmanager.h \
    logmanager.h \
    buttondatabase.h

# UI文件
FORMS += \
    mainwindow.ui

# Windows平台特定配置
win32 {
    RC_ICONS = com.ico
    VERSION = 1.0.0.0
    QMAKE_TARGET_COMPANY = "Company"
    QMAKE_TARGET_PRODUCT = "Flex SerialPort"
    QMAKE_TARGET_DESCRIPTION = "Serial Port Communication Tool"
    QMAKE_TARGET_COPYRIGHT = "Copyright (C) 2024"
}


