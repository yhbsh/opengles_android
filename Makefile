SDK     = $(HOME)/Library/Android/sdk
ADB     = $(SDK)/platform-tools/adb
AAPT    = $(SDK)/build-tools/34.0.0/aapt
SIGNER  = $(SDK)/build-tools/34.0.0/apksigner
NDK     = $(SDK)/ndk/21.4.7075529/toolchains/llvm/prebuilt/darwin-x86_64
CC      = $(NDK)/bin/aarch64-linux-android21-clang
STRIP   = $(NDK)/bin/llvm-strip
FLAGS   = -I.deps/include -Wall -Wextra
PROG    = list
LIBS    = -L.deps/lib -L$(NDK)/sysroot/usr/lib/aarch64-linux-android/30 -laaudio -lGLESv3 -legl -lc -lm -llog -landroid -lavformat -lavcodec -lswscale -lswresample -lavutil

all: launch
apk: package

engine:
	@mkdir -p lib/arm64-v8a

	$(CC) $(FLAGS) src/$(PROG).c -o ./lib/arm64-v8a/libengine.so -shared -fPIC $(LIBS)
	$(STRIP) ./lib/arm64-v8a/libengine.so

package: engine
	$(AAPT) package -f -M AndroidManifest.xml -I $(SDK)/platforms/android-21/android.jar -F app.unsigned.apk
	$(AAPT) add app.unsigned.apk lib/arm64-v8a/libengine.so > /dev/null
	$(AAPT) add app.unsigned.apk assets/shader.vert > /dev/null
	$(AAPT) add app.unsigned.apk assets/shader.frag > /dev/null
	$(SIGNER) sign --ks ~/.gradle/debug.keystore --ks-key-alias androiddebugkey --ks-pass pass:android --out app.apk app.unsigned.apk
	@rm -rf app.unsigned.apk app.apk.idsig lib

install: apk
	@$(ADB) install app.apk > /dev/null

launch: install
	@$(ADB) shell am start -n "com.example.native/android.app.NativeActivity" > /dev/null

clean:
	rm -rf app.apk lib
