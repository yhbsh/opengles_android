# Configuration and paths
ANDROID_ROOT        := $(HOME)/Library/Android/sdk
ANDROID_JAR         := $(ANDROID_ROOT)/platforms/android-34/android.jar
ANDROID_NDK_VERSION := 26.3.11579264
ANDROID_API_LEVEL   := 34
BUILD_TOOLS_VERSION := 34.0.0
JAVA_HOME           := $(shell /usr/libexec/java_home)

# Tool definitions
BUILD_TOOLS         := $(ANDROID_ROOT)/build-tools/$(BUILD_TOOLS_VERSION)
PLATFORM_TOOLS      := $(ANDROID_ROOT)/platform-tools
D8                  := $(BUILD_TOOLS)/d8
AAPT                := $(BUILD_TOOLS)/aapt
ZIP_ALIGN           := $(BUILD_TOOLS)/zipalign
APK_SIGNER          := $(BUILD_TOOLS)/apksigner
ADB                 := $(PLATFORM_TOOLS)/adb
JAVAC               := $(JAVA_HOME)/bin/javac
DEBUG_KEYSTORE      := $(HOME)/.gradle/debug.keystore

# Project-specific settings
SRC_DIR             := src
BUILD_DIR           := build
RES_DIR             := resources
PACKAGE_NAME        := com/example/gles3
PACKAGE_ID          := com.example.gles3
MANIFEST_FILE       := AndroidManifest.xml
ENGINE_SRC          := $(SRC_DIR)/triangle_uv.c
ENGINE_LIB          := $(BUILD_DIR)/lib/arm64-v8a/libengine.so
UNSIGNED_APK        := $(BUILD_DIR)/app.unsigned.apk
UNALIGNED_APK       := $(BUILD_DIR)/app.unaligned.apk
SIGNED_APK          := $(BUILD_DIR)/app.apk

# NDK and compiler settings
TOOLCHAIN           := $(ANDROID_ROOT)/ndk/$(ANDROID_NDK_VERSION)/toolchains/llvm/prebuilt/darwin-x86_64
ARCH                := aarch64-linux-android
CC                  := $(TOOLCHAIN)/bin/$(ARCH)$(ANDROID_API_LEVEL)-clang
STRIP               := $(TOOLCHAIN)/bin/llvm-strip

# Compilation flags
CFLAGS              := -fPIC
LDFLAGS             := -lGLESv3 -llog -lm -shared

.PHONY: all apk engine launch clean

all: launch

apk: clean engine
	$(AAPT) package -f -m -J $(BUILD_DIR) -M $(MANIFEST_FILE) -S $(RES_DIR) -I $(ANDROID_JAR)
	$(JAVAC) -cp $(ANDROID_JAR):$(BUILD_DIR) -d $(BUILD_DIR) $(SRC_DIR)/MainActivity.java
	$(D8) --lib $(ANDROID_JAR) --min-api $(ANDROID_API_LEVEL) --release --output $(BUILD_DIR) $(BUILD_DIR)/com/example/gles3/*.class
	$(AAPT) package -f -M $(MANIFEST_FILE) -S $(RES_DIR) -I $(ANDROID_JAR) -F $(UNALIGNED_APK) $(BUILD_DIR)
	$(ZIP_ALIGN) -v 4 $(UNALIGNED_APK) $(UNSIGNED_APK)
	$(APK_SIGNER) sign --ks $(DEBUG_KEYSTORE) --ks-key-alias androiddebugkey --ks-pass pass:android --key-pass pass:android --out $(SIGNED_APK) $(UNSIGNED_APK)

engine: $(ENGINE_SRC)
	mkdir -p $(BUILD_DIR)/lib/arm64-v8a
	$(CC) $(CFLAGS) $(ENGINE_SRC) -o $(ENGINE_LIB) $(LDFLAGS)
	$(STRIP) $(ENGINE_LIB)

launch: apk
	$(ADB) install $(SIGNED_APK)
	$(ADB) shell am start -n "$(PACKAGE_ID)/.MainActivity"

clean:
	rm -rf $(BUILD_DIR)
