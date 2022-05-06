Jacob Haydel

This is spectropi. It originally started out as an audio visualization project however due to faulty hardware it because a audio based locking mechanism. The password can be set by changing the passcode array in main.c

To actually enter the password you need a DTMF (dual tone multi frequency) generator. These can be found readily online or on the app store. Once you have this make sure the device is sufficiently close to your audio source and simply type # and then your passcode. By default the passcode is 314159.

To relock the mechanism just type # on your tone generator.

When unlocked pin 25 will be held high when locked it will be held low.

Building the project:
	export PICO_SDK_PATH="path/to/pico_sdk"
	cd build
	camke ..
	make
