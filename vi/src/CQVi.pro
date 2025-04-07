TEMPLATE = lib

TARGET = CQVi

DEPENDPATH += .

QT += widgets

CONFIG += staticlib

QMAKE_CXXFLAGS += \
-std=c++17 \

CONFIG += c++17

MOC_DIR = .moc

SOURCES += \
CQVi.cpp \
CVi.cpp \
CEd.cpp \
\
CSyntaxC.cpp \
CSyntax.cpp \
CSyntaxCPP.cpp \
CSyntaxPython.cpp \
CSyntaxVHDL.cpp \

HEADERS += \
../include/CQVi.h \
../include/CVi.h \
../include/CEd.h \

OBJECTS_DIR = ../obj

DESTDIR = ../lib

INCLUDEPATH += \
. \
../include \
../../../CQUtil/include \
../../../CCommand/include \
../../../CUndo/include \
../../../CUtil/include \
../../../CFile/include \
../../../CStrUtil/include \
../../../CRegExp/include \
