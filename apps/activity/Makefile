ANDROID_JAR = $(HOME)/Library/Android/sdk/platforms/android-21/android.jar

.PHONY: all clean

all: activity.apk

activity.apk: MainActivity.java AndroidManifest.xml
	javac -cp $(ANDROID_JAR) MainActivity.java
	d8 --lib $(ANDROID_JAR) MainActivity.class --output .
	aapt package -f -M AndroidManifest.xml -I $(ANDROID_JAR) -F activity.unsigned.apk
	aapt add activity.unsigned.apk classes.dex > /dev/null
	apksigner sign --ks ~/.gradle/debug.keystore --ks-key-alias androiddebugkey --ks-pass pass:android --out activity.apk activity.unsigned.apk
	@rm -rf activity.apk.idsig activity.unsigned.apk MainActivity.class classes.dex

install: activity.apk
	@adb install -r activity.apk > /dev/null

launch: install
	@adb shell am start -n "com.example.activity/.MainActivity" > /dev/null

clean:
	rm -f *.apk MainActivity.class classes.dex
