SDK     = $(HOME)/Library/Android/sdk
JAR     = $(SDK)/platforms/android-34/android.jar
ADB     = $(SDK)/platform-tools/adb
AAPT    = $(SDK)/build-tools/34.0.0/aapt
SIGNER  = $(SDK)/build-tools/34.0.0/apksigner
D8      = $(SDK)/build-tools/34.0.0/d8
CC      = $(SDK)/ndk/21.4.7075529/toolchains/llvm/prebuilt/darwin-x86_64/bin/aarch64-linux-android21-clang
CFLAGS  = -Wall -Wextra
LDFLAGS = -shared -fPIC -lGLESv3 -legl -llog -landroid -lm

.PHONE: all clean

all:
	@mkdir -p lib/arm64-v8a
	javac -cp $(JAR) --release 11 MainActivity.java
	$(D8) --lib $(JAR) MainActivity.class --output .
	$(CC) $(CFLAGS) game.c -o lib/arm64-v8a/libgame.so $(LDFLAGS)
	$(AAPT) package -f -M AndroidManifest.xml -I $(JAR) -F game.unsigned.apk
	$(AAPT) add game.unsigned.apk lib/arm64-v8a/libgame.so > /dev/null
	$(AAPT) add game.unsigned.apk classes.dex > /dev/null
	$(AAPT) add game.unsigned.apk assets/vert.glsl > /dev/null
	$(AAPT) add game.unsigned.apk assets/frag.glsl > /dev/null
	$(SIGNER) sign --ks ~/.gradle/debug.keystore --ks-key-alias androiddebugkey --ks-pass pass:android --out game.apk game.unsigned.apk
	@rm -rf **.unsigned.apk **.idsig **.dex **.class

launch: all
	@$(ADB) install game.apk > /dev/null
	@$(ADB) shell am start -n "com.example.game/.MainActivity" > /dev/null

clean:
	rm -rf **.apk **.unsigned.apk
