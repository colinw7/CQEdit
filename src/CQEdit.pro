TEMPLATE = app
TARGET = 
DEPENDPATH += .

CONFIG += debug

# Input
SOURCES += \
main.cpp \
CQEditTest.cpp \
CQEditBg.cpp \
CQEdit.cpp \
CQEditCanvas.cpp \
CQEditFile.cpp \
CQHistoryLineEdit.cpp \
CQApp.cpp \
CQWindow.cpp \
CQMainWindow.cpp \
CQFontChooser.cpp \
CQColorChooser.cpp \
CQAlphaButton.cpp \
CQTabWidget.cpp \
CQDialog.cpp \
CQImageButton.cpp \
CQFloatEdit.cpp \
CQWinWidget.cpp \
CQUtil.cpp \
CQImage.cpp \
CQFont.cpp \
CVEditChar.cpp \
CVEditCursor.cpp \
CVEditFile.cpp \
CVEditGen.cpp \
CVEditLine.cpp \
CVEditMgr.cpp \
CVEditVi.cpp \
CVLineEdit.cpp \
CEditChar.cpp \
CEditCmd.cpp \
CEditCursor.cpp \
CEditEd.cpp \
CEditFile.cpp \
CEditFileUtil.cpp \
CEditLine.cpp \
CEditMgr.cpp \
CTextFile.cpp \
CLineEdit.cpp \
CEd.cpp \
CSyntax.cpp \
CSyntaxCPP.cpp \
CSyntaxC.cpp \
CCommand.cpp \
CCommandDest.cpp \
CCommandFileDest.cpp \
CCommandFileSrc.cpp \
CCommandPipe.cpp \
CCommandPipeDest.cpp \
CCommandPipeSrc.cpp \
CCommandSrc.cpp \
CCommandStringDest.cpp \
CCommandStringSrc.cpp \
CCommandUtil.cpp \
CWindow.cpp \
CEvent.cpp \

HEADERS += \
CQEditTest.h \
CQEditBg.h \
CQEdit.h \
CQEditCanvas.h \
CQEditFile.h \
CQEditMgr.h \
CQHistoryLineEdit.h \
CQApp.h \
CQWindow.h \
CQMainWindow.h \
CQFontChooser.h \
CQColorChooser.h \
CQAlphaButton.h \
CQTabWidget.h \
CQDialog.h \
CQImageButton.h \
CQFloatEdit.h \
CQWinWidget.h \
CQUtil.h \
CQImage.h \
CQFont.h \
CVEditChar.h \
CVEditCursor.h \
CVEditFile.h \
CVEditGen.h \
CVEdit.h \
CVEditLine.h \
CVEditMgr.h \
CVEditVi.h \
CVLineEdit.h \
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
CTextFile.h \
CLineEdit.h \
CEd.h \
CSyntax.h \
CSyntaxCPP.h \
CSyntaxC.h \
CCommand.h \
CCommandI.h \
CWindow.h \
CEvent.h \

DESTDIR     = ../bin
OBJECTS_DIR = ../obj
LIB_DIR     = ../lib

INCLUDEPATH += \
../include \
. \
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
-L../../CImageLib/lib \
-L../../CConfig/lib \
-L../../CUndo/lib \
-L../../CFont/lib \
-L../../CReadLine/lib \
-L../../CFile/lib \
-L../../CStrUtil/lib \
-L../../COS/lib \
-L../../CRGBName/lib \
-L../../CRegExp/lib \
-lCImageLib -lCConfig -lCUndo -lCFont -lCReadLine -lCFile -lCStrUtil \
-lCRGBName -lCOS -lCRegExp \
-ljpeg -lpng -lcurses -ltre
