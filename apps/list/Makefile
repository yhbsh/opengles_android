SDK     = $(HOME)/Library/Android/sdk
ADB     = $(SDK)/platform-tools/adb
AAPT    = $(SDK)/build-tools/34.0.0/aapt
SIGNER  = $(SDK)/build-tools/34.0.0/apksigner
CC      = $(SDK)/ndk/21.1.6352462/toolchains/llvm/prebuilt/darwin-x86_64/bin/aarch64-linux-android21-clang
CFLAGS  = -Wall -Wextra
LDFLAGS = -shared -fPIC -lGLESv3 -legl -llog -landroid

.PHONE: all clean

all:
	@mkdir -p lib/arm64-v8a
	$(CC) $(CFLAGS) list.c -o lib/arm64-v8a/liblist.so $(LDFLAGS)
	$(AAPT) package -f -M AndroidManifest.xml -I $(SDK)/platforms/android-21/android.jar -F list.unsigned.apk
	$(AAPT) add list.unsigned.apk lib/arm64-v8a/liblist.so > /dev/null
	$(AAPT) add list.unsigned.apk assets/vert.glsl > /dev/null
	$(AAPT) add list.unsigned.apk assets/frag.glsl > /dev/null
	$(SIGNER) sign --ks ~/.gradle/debug.keystore --ks-key-alias androiddebugkey --ks-pass pass:android --out list.apk list.unsigned.apk
	@rm -rf list.unsigned.apk list.apk.idsig

install: all
	@$(ADB) install list.apk > /dev/null

launch: install
	@$(ADB) shell am start -n "com.example.list/android.app.NativeActivity" > /dev/null

clean:
	rm -rf *.apk *.unsigned.apk
