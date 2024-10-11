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


all: launch_apk

ARCH := arm64-v8a

generate_bytecode: src/MainActivity.java
	$(JAVAC) -cp $(ANDROID_JAR):build -d build src/MainActivity.java

generate_compiled_resources: generate_bytecode
	$(AAPT2) compile -o build/compiled_resources.zip --dir resources

generate_engine_lib: generate_compiled_resources src/engine.c
	mkdir -p build/lib/$(ARCH)
	$(CC) -I./include ./src/video.c -o ./build/lib/$(ARCH)/libengine.so -L./lib -shared -fPIC \
		-lavformat \
		-lavcodec \
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
	$(AAPT2) link -o build/app.unsigned.apk --manifest AndroidManifest.xml -I $(ANDROID_JAR) -R build/compiled_resources.zip --auto-add-overlay --java build --min-sdk-version $(ANDROID_API_LEVEL) --target-sdk-version $(ANDROID_API_LEVEL)

generate_dex_file: generate_unsigned_apk
	$(D8) --lib $(ANDROID_JAR) --min-api $(ANDROID_API_LEVEL) --release --output build build/com/example/gles3/*.class
	zip -quj build/app.unsigned.apk build/classes.dex
	cd build && zip -qur app.unsigned.apk lib/

generate_signed_apk: generate_dex_file
	$(APK_SIGNER) sign --ks $(DEBUG_KEYSTORE) --ks-key-alias androiddebugkey --ks-pass pass:android --key-pass pass:android --out build/app.apk build/app.unsigned.apk

apk: generate_signed_apk

install_apk: apk
	$(ADB) install build/app.apk

launch_apk: install_apk
	$(ADB) shell am start -n "com.example.gles3/.MainActivity"

clean:
	rm -rf build
