TEMPLATE = app

QT += widgets

TARGET = CQEdit

DEPENDPATH += .

MOC_DIR = .moc

QMAKE_CXXFLAGS += -std=c++17 -DCQUTIL_IMAGE -DCQUTIL_FONT

CONFIG += debug

# Input
SOURCES += \
main.cpp \
\
CQEditTest.cpp \
CQEditBg.cpp \
CQEdit.cpp \
CQEditCanvas.cpp \
CQEditFile.cpp \
\
CVEditChar.cpp \
CVEditCursor.cpp \
CVEditFile.cpp \
CVEditGen.cpp \
CVEditLine.cpp \
CVEditMgr.cpp \
CVEditVi.cpp \
CVLineEdit.cpp \
\
CEditChar.cpp \
CEditCmd.cpp \
CEditCursor.cpp \
CEditEd.cpp \
CEditFile.cpp \
CEditFileUtil.cpp \
CEditLine.cpp \
CEditMgr.cpp \
\
CTextFile.cpp \
CLineEdit.cpp \
\
CEd.cpp \
\
CSyntax.cpp \
CSyntaxCPP.cpp \
CSyntaxC.cpp \
\
CQHistoryLineEdit.cpp \

HEADERS += \
CQEditTest.h \
CQEditBg.h \
CQEdit.h \
CQEditCanvas.h \
CQEditFile.h \
CQEditMgr.h \
\
CVEditChar.h \
CVEditCursor.h \
CVEditFile.h \
CVEditGen.h \
CVEdit.h \
CVEditLine.h \
CVEditMgr.h \
CVEditVi.h \
CVLineEdit.h \
\
CEditChar.h \
CEditCmd.h \
CEditCursor.h \
CEditEd.h \
CEditFileCharIterator.h \
CEditFile.h \
CEditFileUtil.h \
CEdit.h \
CEditLine.h \
CEditMgr.h \
\
CTextFile.h \
CLineEdit.h \
\
CEd.h \
\
CSyntax.h \
CSyntaxCPP.h \
CSyntaxC.h \
\
CQHistoryLineEdit.h \

DESTDIR     = ../bin
OBJECTS_DIR = ../obj
LIB_DIR     = ../lib

INCLUDEPATH += \
. \
../include \
../../CQUtil/include \
../../CCommand/include \
../../CImageLib/include \
../../CUndo/include \
../../CFont/include \
../../CFile/include \
../../CConfig/include \
../../COS/include \
../../CStrUtil/include \
../../CUtil/include \
../../CMath/include \
../../CReadLine/include \
../../CRegExp/include \
../../CRGBName/include \

unix:LIBS += \
-L$$LIB_DIR \
-L../../CQUtil/lib \
-L../../CCommand/lib \
-L../../CImageLib/lib \
-L../../CConfig/lib \
-L../../CUndo/lib \
-L../../CFont/lib \
-L../../CReadLine/lib \
-L../../CFile/lib \
-L../../CFileUtil/lib \
-L../../CMath/lib \
-L../../CStrUtil/lib \
-L../../CUtil/lib \
-L../../COS/lib \
-L../../CRGBName/lib \
-L../../CRegExp/lib \
-lCQUtil -lCCommand -lCImageLib -lCConfig -lCUndo -lCFont -lCReadLine -lCFile \
-lCFileUtil -lCMath -lCStrUtil -lCRGBName -lCUtil -lCOS -lCRegExp \
-ljpeg -lpng -lcurses -ltre
