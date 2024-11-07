SDK     = $(HOME)/Library/Android/sdk
JAR     = $(SDK)/platforms/android-21/android.jar
 
.PHONY: all clean install launch

all: java.apk

classes.dex: MainActivity.java
	javac -cp $(JAR) MainActivity.java
	d8 --lib $(JAR) *.class --output .
	rm -f *.class

java.apk: AndroidManifest.xml classes.dex
	aapt package -f -M AndroidManifest.xml -I $(JAR) -F java.unsigned.apk
	aapt add java.unsigned.apk classes.dex > /dev/null
	apksigner sign --ks ~/.gradle/debug.keystore --ks-key-alias androiddebugkey --ks-pass pass:android --out java.apk java.unsigned.apk

install: java.apk
	@adb install -r java.apk > /dev/null

launch: install
	@adb shell am start -n "com.example.java/.MainActivity" > /dev/null

clean:
	rm -f *.apk *.idsig
	rm -rf lib
