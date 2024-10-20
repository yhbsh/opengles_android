ANDROID_ROOT = $(HOME)/Library/Android/sdk
BUILD_TOOLS  = $(ANDROID_ROOT)/build-tools/34.0.0
ADB          = $(ANDROID_ROOT)/platform-tools/adb
AAPT         = $(BUILD_TOOLS)/aapt
ZIP_ALIGN    = $(BUILD_TOOLS)/zipalign
APK_SIGNER   = $(BUILD_TOOLS)/apksigner

TOOLCHAINS   = $(ANDROID_ROOT)/ndk/21.4.7075529/toolchains/llvm/prebuilt/darwin-x86_64
CC           = $(TOOLCHAINS)/bin/aarch64-linux-android21-clang
STRIP        = $(TOOLCHAINS)/bin/llvm-strip

FLAGS        = -I.deps/include -Wall -Wextra
PROG         = triangle
LIBS         = -L.deps/lib -lGLESv3 -legl -lc -lm -llog -landroid -lavformat -lavcodec -lswscale -lswresample -lavutil

all: launch
apk: package

engine:
	@mkdir -p lib/arm64-v8a

	$(CC) $(FLAGS) src/$(PROG).c -o ./lib/arm64-v8a/libengine.so -shared -fPIC $(LIBS)
	$(STRIP) ./lib/arm64-v8a/libengine.so

package: engine
	$(AAPT) package -f -M AndroidManifest.xml -I $(ANDROID_ROOT)/platforms/android-21/android.jar -F app.unsigned.apk
	$(AAPT) add app.unsigned.apk lib/arm64-v8a/libengine.so > /dev/null
	$(APK_SIGNER) sign --ks ~/.gradle/debug.keystore --ks-key-alias androiddebugkey --ks-pass pass:android --out app.apk app.unsigned.apk
	@rm -rf app.unsigned.apk app.apk.idsig lib

install: apk
	@$(ADB) install app.apk > /dev/null 2>&1

launch: install
	@$(ADB) shell am start -n "com.example.gles3/android.app.NativeActivity" > /dev/null 2>&1

clean:
	rm -rf app.apk
