HEADERS += \
	../../libs/PhTools/PhTime.h \
    ../../libs/PhTools/PhTimeCode.h \
    ../../libs/PhTools/PhClock.h \
    ../../libs/PhTools/PhTest.h

SOURCES += \
    ../../libs/PhTools/PhTimeCode.cpp \
    ../../libs/PhTools/PhClock.cpp \
    ../../libs/PhTools/PhTest.cpp

# Windows specific
win32 {
	RESOURCES_PATH = .
}

# Ubuntu specific
linux {
	RESOURCES_PATH = .
}


# MacOS specific
mac {
	RESOURCES_PATH = $${TARGET}.app/Contents/MacOS
}

