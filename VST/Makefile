VERSION = 0.6.1

RACK_DIR ?= ../..
include $(RACK_DIR)/arch.mk

VST2_SDK ?= VST2_SDK
TARGET := VCV-Bridge

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
	rm -rfv build $(TARGET) dist

dist: all
ifdef ARCH_MAC
	mkdir -p dist
	mkdir -p dist/$(BUNDLE)/Contents
	mkdir -p dist/$(BUNDLE)/Contents/MacOS
	cp Info.plist dist/$(BUNDLE)/Contents/Info.plist
	cp PkgInfo dist/$(BUNDLE)/Contents/
	cp $(TARGET) dist/$(BUNDLE)/Contents/MacOS/
	strip -S dist/$(BUNDLE)/Contents/MacOS/$(TARGET)
	cd dist && zip -5 -r VCV-Bridge-$(VERSION)-$(ARCH)-VST.zip $(BUNDLE)
endif
ifdef ARCH_WIN
	mkdir -p dist
	cp $(TARGET) dist/
	strip -s dist/$(TARGET)
	cd dist && zip -5 -r VCV-Bridge-$(VERSION)-$(ARCH)-VST.zip $(TARGET)
endif
ifdef ARCH_LIN
	mkdir -p dist
	cp $(TARGET) dist/
	strip -s dist/$(TARGET)
	cd dist && zip -5 -r VCV-Bridge-$(VERSION)-$(ARCH)-VST.zip $(TARGET)
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
