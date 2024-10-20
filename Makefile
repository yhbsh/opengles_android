ANDROID_ROOT = $(HOME)/Library/Android/sdk
BUILD_TOOLS  = $(ANDROID_ROOT)/build-tools/34.0.0
ADB          = $(ANDROID_ROOT)/platform-tools/adb
AAPT2        = $(BUILD_TOOLS)/aapt2
ZIP_ALIGN    = $(BUILD_TOOLS)/zipalign
APK_SIGNER   = $(BUILD_TOOLS)/apksigner

TOOLCHAINS   = $(ANDROID_ROOT)/ndk/21.4.7075529/toolchains/llvm/prebuilt/darwin-x86_64
CC           = $(TOOLCHAINS)/bin/aarch64-linux-android21-clang
STRIP        = $(TOOLCHAINS)/bin/llvm-strip

PROG        ?= triangle

all: launch

engine:
	@mkdir -p build/lib/arm64-v8a

	$(CC) \
		-I./.deps/include \
		-Wall \
		-Wextra \
		./src/$(PROG).c \
		-o \
		./build/lib/arm64-v8a/libengine.so \
		-L./.deps/lib \
		-shared \
		-fPIC \
		-lGLESv3 \
		-legl \
		-lc \
		-lm \
		-llog \
		-landroid \
		-lavformat \
		-lavcodec \
		-lswscale \
		-lswresample \
		-lavutil

	@$(STRIP) ./build/lib/arm64-v8a/libengine.so

unsigned: engine
	$(AAPT2) \
		link \
		-o \
		build/app.unsigned.apk \
		--manifest \
		AndroidManifest.xml \
		-I \
		$(ANDROID_ROOT)/platforms/android-21/android.jar \
		--auto-add-overlay \
		--min-sdk-version 21 \
		--target-sdk-version 34

	@cd build && zip -qur app.unsigned.apk lib

signed: unsigned
	$(APK_SIGNER) \
		sign \
		--ks \
		~/.gradle/debug.keystore\
		--ks-key-alias \
		androiddebugkey \
		--ks-pass \
		pass:android \
		--key-pass \
		pass:android \
		--out \
		build/app.apk \
		build/app.unsigned.apk

apk: signed

install: apk
	@$(ADB) install build/app.apk > /dev/null 2>&1

launch: install
	@$(ADB) shell am start -n "com.example.gles3/android.app.NativeActivity" > /dev/null 2>&1

clean:
	rm -rf build
