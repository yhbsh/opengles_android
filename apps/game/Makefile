SDK     = $(HOME)/Library/Android/sdk
JAR     = $(SDK)/platforms/android-34/android.jar
CC      = $(SDK)/ndk/21.4.7075529/toolchains/llvm/prebuilt/darwin-x86_64/bin/aarch64-linux-android21-clang

CFLAGS  = -Wall -Wextra
LDFLAGS = -shared -fPIC -lGLESv3 -legl -llog -landroid -lm

.PHONE: all clean

all: package

java: *.java
	javac -cp $(JAR) --release 11 *.java
	d8 --lib $(JAR) *.class --output .

c: *.c
	@mkdir -p lib/arm64-v8a
	$(CC) $(CFLAGS) game.c -o lib/arm64-v8a/libgame.so $(LDFLAGS)

package: c java
	aapt package -f -M AndroidManifest.xml -I $(JAR) -F game.unsigned.apk
	aapt add game.unsigned.apk lib/arm64-v8a/libgame.so > /dev/null
	aapt add game.unsigned.apk classes.dex > /dev/null
	aapt add game.unsigned.apk assets/vert.glsl > /dev/null
	aapt add game.unsigned.apk assets/frag.glsl > /dev/null
	apksigner sign --ks ~/.gradle/debug.keystore --ks-key-alias androiddebugkey --ks-pass pass:android --out game.apk game.unsigned.apk
	@rm -rf **.unsigned.apk **.idsig **.dex **.class

launch: package
	@adb install game.apk > /dev/null
	@adb shell am start -n "com.example.game/.MainActivity" > /dev/null

clean:
	rm -rf **.apk **.unsigned.apk
