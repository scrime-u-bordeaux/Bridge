VERSION = 0.6.1

RACK_DIR ?= ../..
include $(RACK_DIR)/arch.mk

VST2_SDK ?= VST2_SDK
TARGET := VCV-Bridge

ifdef FX
	TARGET := $(TARGET)-fx
	FLAGS += -DFX
endif

FLAGS += -fPIC
FLAGS += -I$(VST2_SDK) -I$(RACK_DIR)/include

ifdef ARCH_MAC
	FLAGS += -arch i386 -arch x86_64
	LDFLAGS += -arch i386 -arch x86_64
	LDFLAGS += -bundle
	BUNDLE := $(TARGET).vst
endif
ifdef ARCH_WIN
	LDFLAGS += -shared -static -lws2_32
ifdef ARCH_WIN_64
	TARGET := $(TARGET)-64.dll
endif
ifdef ARCH_WIN_32
	TARGET := $(TARGET)-32.dll
endif
endif
ifdef ARCH_LIN
	FLAGS += -D__cdecl=""
	LDFLAGS += -lpthread -shared
	TARGET := $(TARGET).so
endif


SOURCES += BridgeVST.cpp
SOURCES += $(VST2_SDK)/public.sdk/source/vst2.x/audioeffect.cpp
SOURCES += $(VST2_SDK)/public.sdk/source/vst2.x/audioeffectx.cpp
SOURCES += $(VST2_SDK)/public.sdk/source/vst2.x/vstplugmain.cpp


all: $(TARGET)

include $(RACK_DIR)/compile.mk

clean:
	rm -rfv build $(TARGET)

dist: all
	mkdir -p dist
ifdef ARCH_MAC
	mkdir -p dist/$(BUNDLE)
	mkdir -p dist/$(BUNDLE)/Contents
ifdef FX
	cp Info-fx.plist dist/$(BUNDLE)/Contents/Info.plist
else
	cp Info.plist dist/$(BUNDLE)/Contents/Info.plist
endif
	cp PkgInfo dist/$(BUNDLE)/Contents/
	mkdir -p dist/$(BUNDLE)/Contents/MacOS
	cp $(TARGET) dist/$(BUNDLE)/Contents/MacOS/
	strip -S dist/$(BUNDLE)/Contents/MacOS/$(TARGET)
endif
ifdef ARCH_WIN
	cp $(TARGET) dist/
	strip -s dist/$(TARGET)
endif
ifdef ARCH_LIN
	cp $(TARGET) dist/
	strip -s dist/$(TARGET)
endif

install: dist
ifdef ARCH_MAC
	cp -R dist/$(BUNDLE) /Library/Audio/Plug-Ins/VST/
endif
ifdef ARCH_WIN
	mkdir -p /c/Program\ Files/Steinberg/VstPlugins
	cp dist/$(TARGET) /c/Program\ Files/Steinberg/VstPlugins/
endif
ifdef ARCH_LIN
	mkdir -p /usr/lib/vst
	cp dist/$(TARGET) /usr/lib/vst/
endif

uninstall:
ifdef ARCH_MAC
	rm -rfv /Library/Audio/Plug-Ins/VST/$(BUNDLE)
endif
ifdef ARCH_WIN
	rm /c/Program\ Files/Steinberg/VstPlugins/$(TARGET)
endif
ifdef ARCH_LIN
	rm /usr/lib/vst/$(TARGET)
endif
