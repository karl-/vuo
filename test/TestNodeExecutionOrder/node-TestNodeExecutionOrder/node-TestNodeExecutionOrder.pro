TEMPLATE = aux

include(../../../vuo.pri)

NODE_SOURCES += \
	vuo.test.conductor.c \
	vuo.test.firePeriodically.c \
	vuo.test.semiconductor.c

OTHER_FILES += $$NODE_SOURCES

include(../../../module.pri)
