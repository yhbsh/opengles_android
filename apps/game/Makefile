SDK     = $(HOME)/Library/Android/sdk
ADB     = $(SDK)/platform-tools/adb
AAPT    = $(SDK)/build-tools/34.0.0/aapt
SIGNER  = $(SDK)/build-tools/34.0.0/apksigner
CC      = $(SDK)/ndk/21.4.7075529/toolchains/llvm/prebuilt/darwin-x86_64/bin/aarch64-linux-android21-clang
CFLAGS  = -Wall -Wextra
LDFLAGS = -shared -fPIC -lGLESv3 -legl -llog -landroid

.PHONE: all clean

all:
	@mkdir -p lib/arm64-v8a
	$(CC) $(CFLAGS) game.c -o lib/arm64-v8a/libgame.so $(LDFLAGS)
	$(AAPT) package -f -M AndroidManifest.xml -I $(SDK)/platforms/android-21/android.jar -F game.unsigned.apk
	$(AAPT) add game.unsigned.apk lib/arm64-v8a/libgame.so > /dev/null
	$(AAPT) add game.unsigned.apk assets/vert.glsl > /dev/null
	$(AAPT) add game.unsigned.apk assets/frag.glsl > /dev/null
	$(SIGNER) sign --ks ~/.gradle/debug.keystore --ks-key-alias androiddebugkey --ks-pass pass:android --out game.apk game.unsigned.apk
	@rm -rf game.unsigned.apk game.apk.idsig

launch: all
	@$(ADB) install game.apk > /dev/null
	@$(ADB) shell am start -n "com.example.game/android.app.NativeActivity" > /dev/null

clean:
	rm -rf *.apk *.unsigned.apk
