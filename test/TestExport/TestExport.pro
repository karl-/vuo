TEMPLATE = app
CONFIG -= app_bundle
CONFIG += console testcase qtTest VuoFramework
TARGET = TestExport

include(../../vuo.pri)

QMAKE_RPATHDIR = $$ROOT/framework
QMAKE_LFLAGS_RPATH = -rpath$$LITERAL_WHITESPACE

SOURCES += \
	TestExport.cc
