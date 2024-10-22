SDK     = $(HOME)/Library/Android/sdk
ADB     = $(SDK)/platform-tools/adb
AAPT    = $(SDK)/build-tools/34.0.0/aapt
SIGNER  = $(SDK)/build-tools/34.0.0/apksigner
CC      = $(SDK)/ndk/21.4.7075529/toolchains/llvm/prebuilt/darwin-x86_64/bin/aarch64-linux-android21-clang
CFLAGS  = -I../../.deps/include -Wall -Wextra
LDFLAGS = -shared -fPIC -L../../.deps/lib -lGLESv3 -legl -llog -lm -landroid -lavformat -lavcodec -lswscale -lswresample -lavutil

.PHONE: all clean

all:
	@mkdir -p lib/arm64-v8a
	$(CC) $(CFLAGS) video.c -o lib/arm64-v8a/libvideo.so $(LDFLAGS)
	$(AAPT) package -f -M AndroidManifest.xml -I $(SDK)/platforms/android-21/android.jar -F video.unsigned.apk
	$(AAPT) add video.unsigned.apk lib/arm64-v8a/libvideo.so > /dev/null
	$(SIGNER) sign --ks ~/.gradle/debug.keystore --ks-key-alias androiddebugkey --ks-pass pass:android --out video.apk video.unsigned.apk
	@rm -rf video.unsigned.apk video.apk.idsig

launch: all
	@$(ADB) install video.apk > /dev/null
	@$(ADB) shell am start -n "com.example.video/android.app.NativeActivity" > /dev/null

clean:
	rm -rf *.apk *.unsigned.apk
