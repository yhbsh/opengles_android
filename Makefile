ANDROID_ROOT := $(HOME)/Library/Android/sdk
ANDROID_JAR := $(ANDROID_ROOT)/platforms/android-34/android.jar
ANDROID_NDK_VERSION := 26.3.11579264
ANDROID_API_LEVEL := 24

JAVA_VERSION := 17
JAVA_HOME := $(shell /usr/libexec/java_home -v $(JAVA_VERSION))
JAVAC := $(JAVA_HOME)/bin/javac

ANDROID_BUILD_TOOLS_VERSION := 34.0.0
D8 := $(ANDROID_ROOT)/build-tools/$(ANDROID_BUILD_TOOLS_VERSION)/d8
AAPT := $(ANDROID_ROOT)/build-tools/$(ANDROID_BUILD_TOOLS_VERSION)/aapt
ZIP_ALIGN := $(ANDROID_ROOT)/build-tools/$(ANDROID_BUILD_TOOLS_VERSION)/zipalign
APK_SIGNER := $(ANDROID_ROOT)/build-tools/$(ANDROID_BUILD_TOOLS_VERSION)/apksigner
ADB := $(ANDROID_ROOT)/platform-tools/adb
DEBUG_KEYSTORE := $(HOME)/.gradle/debug.keystore


SRC_DIR := ./src
BUILD_DIR := ./build
RESOURCES_DIR := ./resources
ENGINE_LIB := lib/arm64-v8a/libengine.so
PACKAGE_NAME := com/example/gles3
PACKAGE_ID := com.example.gles3

UNSIGNED_APK := $(BUILD_DIR)/app.unsigned.apk
UNALIGNED_APK := $(BUILD_DIR)/app.unaligned.apk
SIGNED_APK := $(BUILD_DIR)/app.apk

TOOLCHAIN := $(ANDROID_ROOT)/ndk/$(ANDROID_NDK_VERSION)/toolchains/llvm/prebuilt/darwin-x86_64
ARCH := aarch64-linux-android
SYSROOT := $(TOOLCHAIN)/sysroot
CC := $(TOOLCHAIN)/bin/$(ARCH)$(ANDROID_API_LEVEL)-clang
CXX := $(TOOLCHAIN)/bin/$(ARCH)$(ANDROID_API_LEVEL)-clang++
CFLAGS := -I$(SYSROOT)/usr/include
CFLAGS += -I$(SYSROOT)/usr/include/$(ARCH)
CFLAGS += -O3 -fPIC -flto
LDFLAGS := -L$(SYSROOT)/usr/lib/$(ARCH)/$(ANDROID_API_LEVEL)
LDFLAGS += -lGLESv3 -landroid -llog -lm -flto -shared
STRIP := $(TOOLCHAIN)/bin/llvm-strip
CLANG_TIDY=$(TOOLCHAIN)/bin/clang-tidy

all: launch

launch: clean
	mkdir -p $(BUILD_DIR)/lib/arm64-v8a
	$(CC) $(CFLAGS) $(SRC_DIR)/engine.c -o $(BUILD_DIR)/$(ENGINE_LIB) $(LDFLAGS)
	$(STRIP) $(BUILD_DIR)/$(ENGINE_LIB)
	$(AAPT) package -f -m -J $(BUILD_DIR) -M AndroidManifest.xml -S $(RESOURCES_DIR) -I $(ANDROID_JAR)
	$(JAVAC) -cp $(ANDROID_JAR):$(BUILD_DIR) -d $(BUILD_DIR) $(SRC_DIR)/*.java
	$(D8) --lib $(ANDROID_JAR) --min-api $(ANDROID_API_LEVEL) --release --output $(BUILD_DIR) $(BUILD_DIR)/com/example/gles3/*.class
	rm -f $(BUILD_DIR)/com/example/gles3/*.class $(BUILD_DIR)/com/example/gles3/R*.java
	$(AAPT) package -f -M AndroidManifest.xml -S $(RESOURCES_DIR) -I $(ANDROID_JAR) -F $(UNALIGNED_APK) $(BUILD_DIR)
	$(ZIP_ALIGN) -v 4 $(UNALIGNED_APK) $(UNSIGNED_APK)
	$(APK_SIGNER) sign --ks $(DEBUG_KEYSTORE) --ks-key-alias androiddebugkey --ks-pass pass:android --key-pass pass:android --out $(SIGNED_APK) $(UNSIGNED_APK)
	rm -f $(UNSIGNED_APK) $(UNALIGNED_APK) $(BUILD_DIR)/*.idsig $(BUILD_DIR)/classes.dex
	$(ADB) install $(SIGNED_APK)
	$(ADB) shell am start -n "$(PACKAGE_ID)/.MainActivity"

check:
	$(CLANG_TIDY) $(SRC_DIR)/*.c -- $(CFLAGS)

clean:
	rm -rf $(BUILD_DIR)
