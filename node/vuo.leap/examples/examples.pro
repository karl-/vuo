TEMPLATE = aux

include(../../../vuo.pri)

!equals(MAC_VERSION, "10.6") {

EXAMPLE_SOURCES += \
	CountLeapObjects.vuo \
	DisplayLeapHand.vuo \
	PlayFingerPuppetsWithLeap.vuo \
	PlayTennisWithLeap.vuo \
	TwirlImageWithLeap.vuo
OTHER_FILES += $$EXAMPLE_SOURCES

}

include(../../../example.pri)
