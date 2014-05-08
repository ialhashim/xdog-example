QT       += core gui widgets

TARGET = sketcher
TEMPLATE = app


SOURCES += main.cpp mainwindow.cpp
HEADERS  += mainwindow.h
FORMS    += mainwindow.ui

LIBS *= $$PWD/lib/gpu.lib

INCLUDEPATH += $$PWD/include
DEPENDPATH += $$PWD/include

## Cuda stuff:

# Default options for Visual Studio 2012 Windows 8.1 x64
SYSTEM_NAME=x64
CUDA_DIR = C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v6.0

# include paths
INCLUDEPATH += "$$CUDA_DIR/include"

# library directories
QMAKE_LIBDIR += $$CUDA_DIR/lib/$$SYSTEM_NAME

LIBS += -L"$$CUDA_DIR/lib/$$SYSTEM_NAME" -lcuda -lcudart

message($$INCLUDEPATH)

