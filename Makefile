ANDROID_ROOT        := $(HOME)/Library/Android/sdk
ANDROID_JAR         := $(ANDROID_ROOT)/platforms/android-34/android.jar
ANDROID_NDK_VERSION := 21.4.7075529
ANDROID_API_LEVEL   := 21
BUILD_TOOLS_VERSION := 34.0.0
JAVA_HOME           := $(shell /usr/libexec/java_home)

BUILD_TOOLS         := $(ANDROID_ROOT)/build-tools/$(BUILD_TOOLS_VERSION)
PLATFORM_TOOLS      := $(ANDROID_ROOT)/platform-tools
D8                  := $(BUILD_TOOLS)/d8
AAPT2               := $(BUILD_TOOLS)/aapt2
ZIP_ALIGN           := $(BUILD_TOOLS)/zipalign
APK_SIGNER          := $(BUILD_TOOLS)/apksigner
ADB                 := $(PLATFORM_TOOLS)/adb
JAVAC               := $(JAVA_HOME)/bin/javac
DEBUG_KEYSTORE      := $(HOME)/.gradle/debug.keystore

TOOLCHAINS          := $(ANDROID_ROOT)/ndk/21.4.7075529/toolchains/llvm/prebuilt/darwin-x86_64
CC                  := $(TOOLCHAINS)/bin/aarch64-linux-android21-clang
STRIP               := $(TOOLCHAINS)/bin/llvm-strip

BUILD_DIR           := build
RES_DIR             := resources
MANIFEST_FILE       := AndroidManifest.xml
JAVA_FILES          := src/MainActivity.java
ENGINE_FILES        ?= src/video.c
CLASS_FILES         := $(BUILD_DIR)/com/example/gles3/*.class
COMPILED_RESOURCES  := $(BUILD_DIR)/compiled_resources.zip
UNSIGNED_APK        := $(BUILD_DIR)/app.unsigned.apk
SIGNED_APK          := $(BUILD_DIR)/app.apk

all: launch_apk

ARCH := arm64-v8a

generate_bytecode: $(JAVA_FILES)
	$(JAVAC) -cp $(ANDROID_JAR):$(BUILD_DIR) -d $(BUILD_DIR) $(JAVA_FILES)

generate_compiled_resources: generate_bytecode
	$(AAPT2) compile -o $(COMPILED_RESOURCES) --dir resources

generate_engine_lib: generate_compiled_resources src/video.c
	mkdir -p $(BUILD_DIR)/lib/$(ARCH)
	$(CC) -I./include ./src/video.c -o ./build/lib/$(ARCH)/libengine.so -L./lib -shared -fPIC \
		-lavformat \
		-lavcodec \
		-lavdevice \
		-lswscale \
		-lswresample \
		-lavutil \
		-lGLESv3 \
		-lc \
		-lm \
		-llog \
		-lmediandk \
		-landroid
	$(STRIP) ./build/lib/$(ARCH)/libengine.so

generate_unsigned_apk: generate_engine_lib
	$(AAPT2) link -o $(UNSIGNED_APK) --manifest $(MANIFEST_FILE) -I $(ANDROID_JAR) -R $(COMPILED_RESOURCES) --auto-add-overlay --java $(BUILD_DIR) --min-sdk-version $(ANDROID_API_LEVEL) --target-sdk-version $(ANDROID_API_LEVEL)

generate_dex_file: generate_unsigned_apk
	$(D8) --lib $(ANDROID_JAR) --min-api $(ANDROID_API_LEVEL) --release --output $(BUILD_DIR) $(CLASS_FILES)
	zip -quj $(UNSIGNED_APK) $(BUILD_DIR)/classes.dex
	cd $(BUILD_DIR) && zip -qur ../$(UNSIGNED_APK) lib/

generate_signed_apk: generate_dex_file
	$(APK_SIGNER) sign --ks $(DEBUG_KEYSTORE) --ks-key-alias androiddebugkey --ks-pass pass:android --key-pass pass:android --out $(SIGNED_APK) $(UNSIGNED_APK)

apk: generate_signed_apk

install_apk: apk
	$(ADB) install $(SIGNED_APK)

launch_apk: install_apk
	$(ADB) shell am start -n "com.example.gles3/.MainActivity"

clean:
	rm -rf $(BUILD_DIR)
