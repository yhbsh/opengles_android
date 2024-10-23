ANDROID_SDK = $(HOME)/Library/Android/sdk
ANDROID_JAR = $(ANDROID_SDK)/platforms/android-21/android.jar
CC          = $(HOME)/Library/Android/sdk/ndk/21.4.7075529/toolchains/llvm/prebuilt/darwin-x86_64/bin/aarch64-linux-android21-clang

CFLAGS  = -Wall -Wextra
LDFLAGS = -shared -fPIC -lGLESv3 -legl -llog -landroid -lm

.PHONE: all clean

all: launch

package:
	@mkdir -p lib/arm64-v8a
	$(CC) $(CFLAGS) game.c -o lib/arm64-v8a/libgame.so $(LDFLAGS)
	aapt package -f -M AndroidManifest.xml -I $(ANDROID_JAR) -F game.unsigned.apk
	aapt add game.unsigned.apk lib/arm64-v8a/libgame.so > /dev/null
	aapt add game.unsigned.apk assets/vert.glsl > /dev/null
	aapt add game.unsigned.apk assets/frag.glsl > /dev/null
	apksigner sign --ks ~/.gradle/debug.keystore --ks-key-alias androiddebugkey --ks-pass pass:android --out game.apk game.unsigned.apk
	@rm -rf **.idsig **.unsigned.apk

launch: package
	@adb install game.apk > /dev/null
	@adb shell am start -n "com.example.game/android.app.NativeActivity" > /dev/null

clean:
	rm -rf **.apk **.unsigned.apk build/
