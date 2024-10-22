SDK     = $(HOME)/Library/Android/sdk
ADB     = $(SDK)/platform-tools/adb
AAPT    = $(SDK)/build-tools/34.0.0/aapt
SIGNER  = $(SDK)/build-tools/34.0.0/apksigner
NDK     = $(SDK)/ndk/21.4.7075529/toolchains/llvm/prebuilt/darwin-x86_64
CC      = $(NDK)/bin/aarch64-linux-android21-clang
CFLAGS  = -I../../.deps/include -Wall -Wextra
LDFLAGS = -shared -fPIC -L$(NDK)/sysroot/usr/lib/aarch64-linux-android/30 -laaudio -lGLESv3 -legl -llog -landroid -lm

.PHONE: all clean

all:
	@mkdir -p lib/arm64-v8a
	$(CC) $(CFLAGS) native.c -o lib/arm64-v8a/libnative.so $(LDFLAGS)
	$(AAPT) package -f -M AndroidManifest.xml -I $(SDK)/platforms/android-21/android.jar -F native.unsigned.apk
	$(AAPT) add native.unsigned.apk lib/arm64-v8a/libnative.so > /dev/null
	$(AAPT) add native.unsigned.apk assets/file.wav > /dev/null
	$(SIGNER) sign --ks ~/.gradle/debug.keystore --ks-key-alias androiddebugkey --ks-pass pass:android --out native.apk native.unsigned.apk
	@rm -rf native.unsigned.apk native.apk.idsig

launch: all
	@$(ADB) install native.apk > /dev/null
	@$(ADB) shell am start -n "com.example.native/android.app.NativeActivity" > /dev/null

clean:
	rm -rf *.apk *.unsigned.apk
