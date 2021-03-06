## ACPIKeyboard.kext by RehabMan

The purpose of this kext is to allow keystrokes to be generated from ACPI code in response to ACPI events, primarily EC queries.  Many modern laptops use ACPI EC queries when special "media" function keys are pressed, such as the "Brightness Up" and "Brightness Down" keys.  This kext can be used to turn those events into ADB keystroke events that are interpreted by OS X as normal key presses.

In order to use the kext, you must:
- patch DSDT to add a very simple RMKB0000 device to which the kext can attach
- determine which ACPI events are generated when the keys are pressed, and replace those methods with a small amount of code that generates "notifications" to the RMKB device, to which ACPIKeyboard.kext is attached.
- install the ACPIKeyboard.kext

If you're using my version of VoodooPS2Controller.kext (eg. you have a Synaptics trackpad), you do not need this kext as the functionality to handle ACPI notifications is already built-in to that kext.  Unlike this kext, with VoodooPS2Controller.kext, the notifications contain PS2 scan code data instead of ADB codes.  This is advantageous as the PS2 codes sent can be translated with the scan code mapper that is part of VoodooPS2Controller.kext.

The primary users of this kext will be those that are using another PS2 kext because they have a different trackpad not well supported by my version of VooodooPS2Controller.kext (eg. those with ELAN trackpads).

Another good source of information on backlight control (including keyboard control) is here: http://www.tonymacx86.com/yosemite-laptop-support/152659-guide-patching-dsdt-ssdt-laptop-backlight-control.html


### Downloads:

Downloads are available on bitbucket:

https://bitbucket.org/RehabMan/os-x-acpi-keyboard/downloads/


### How to Install:

Install the kext using your favorite kext installer utility, such as Kext Wizard.  The Debug directory is for troubleshooting only, in normal "working" installs, you should install the Release version.


### Usage

In order for the kext to load you must have an "RMKB0000" device in ACPI namespace that the driver can attach to.  You can use the "patch.txt" with MaciASL to patch your DSDT in order to add this ACPI device.

The patch will add a device like:
```
Device (RMKB)
{
    Name (_HID, "RMKB0000")
}
```

Which is enough to cause the driver to load and provide a sink for events (via Notify).

In order to intercept the ACPI events, you will need to determine which methods are called when the keys are pressed.  Usually, media keys generate EC queries.  A simple strategy is to use ACPIDebug.kext to instrument all EC query methods, then press the keys while monitoring system.log.  When you press the keys, the name of the method will be output, which will allow you to patch that method.

Determining EC query methods:
- install ACPIDebug.kext: https://github.com/RehabMan/OS-X-ACPI-Debug
- add the ACPIDebug repo to MaciASL per README
- apply ""Add DSDT Debug Methods"
- apply "Instrument EC Queries"
- reboot
- monitor system.log as you press your brightness keys

After you have determined which methods correspond to the brightness keys, you can patch the methods...

Assuming _Q10 is brightness down, and _Q11 is up:

```
into method label _Q10 replace_content
begin
// Brightness Down\n
Notify(\RMKB, 0x2191)\n
Notify(\RMKB, 0x2291)\n
end;

into method label _Q11 replace_content
begin
// Brightness Up\n
Notify(\RMKB, 0x2190)\n
Notify(\RMKB, 0x2290)\n
end;
```

The format of the data sent via Notify is as follows:
- high-order 16-bits must be 0x11, 0x12, 0x21, or 0x22
- 0x11 indicates delegated keydown, 0x12 indicates delegated keyup
- 0x21 indicates non-delegated keydown, 0x22 indicates non-delegated keyup
- the low order 16-bits contain the ADB code to be sent

ADB codes are defined by the data returned by ACPIKeyboard::defaultKeymapOfLength in ACPIKeyboard.cpp.

In our example, we are sending non-delegated brightness down and up, which are 0x91 and 0x90, respectively.  Note that each keystroke requires both a down and up (make and break) code to be sent.

Delegated ADB codes are sent through the real PS2 driver (if installed).  An attempt is made to find the PS2 IOHIKeyboard device that is associated with a PS2 keyboard.  If one is found, delegated keys are sent through that keyboard object instead of the keyboard object provided by ACPIKeyboard.  This has the advantage of modifier keys (Shift, Ctrl, Option, Command) working as you would expect.  It is possible the PS2 keyboard driver installed uses a different keyboard map (therefore different ADB codes) than this driver.  You can see the keyboard map in ioreg (HIDKeyMapping).

Non-delegated ADB codes are sent directly from the keyboard object implemented by this driver.  In this case, modifier keys have no effect, since the modifiers are being held down on a "different keyboard."  You should use delegated ADB codes if possible.

Please note: You must have a working brightness slider before attempting to fix your brightness keys.

** Important Note about the ELAN PS2 keyboard driver **

It may use different codes for brightnesss up/down.  Brightness up=0x4d, Brightness down=0x4f?

Assuming that is the case, the same patch above would be written:

```
into method label _Q10 replace_content
begin
// Brightness Down\n
Notify(\RMKB, 0x114f)\n
Notify(\RMKB, 0x124f)\n
end;

into method label _Q11 replace_content
begin
// Brightness Up\n
Notify(\RMKB, 0x114d)\n
Notify(\RMKB, 0x124d)\n
end;
```

Note: The patch above uses delegated ADB codes (0x11xx/0x12xx) and uses the codes appropriate for the ELAN driver instead of the codes used by most PS2 drivers and this driver (0x4d/0x4f vs. 0x90/0x91).

### More ADB codes for ELAN

With the ELAN PS2 keyboar driver loaded, if you look at HIDKeyMapping under PS2K...ApplePS2Keyboard, you find this data at the very end:
[code]
10 00 48 01 49 02 4d 03 4f 04 39 05 72 06 7f 07 4a 0a 47 0e 70 0f 50 10 42 11 44 12 46 13 40 14 34
[/code]

This is the "special key" area of the keymap (the bits returned from IOHIKeyboard::defaultKeymapOfLength) The first byte 0x10 is the number of codes that follow (I worked backwards from the end to verify this).

The bytes that follow are thus 16 pairs of special codes.  The first byte in the pair is an NX_KEYTYPE (codes defined in the SDK ev_keymap.h). The second byte is the ADB code.

So, you have these pairs:
```
00 48 //NX_KEYTYPE_SOUND_UP
01 49 //NX_KEYTYPE_SOUND_DOWN
02 4d //NX_KEYTYPE_BRIGHTNESS_UP
03 4f //NX_KEYTYPE_BRIGHTNESS_DOWN
04 39 //NX_KEYTYPE_CAPS_LOCK
05 72 //NX_KEYTYPE_HELP
06 7f //NX_POWER_KEY
07 4a //NX_KEYTYPE_MUTE
0a 47 //NX_KEYTYPE_NUM_LOCK
0e 70 //NX_KEYTYPE_EJECT
0f 50 //NX_KEYTYPE_VIDMIRROR
10 42 //NX_KEYTYPE_PLAY
11 44 //NX_KEYTYPE_NEXT
12 46 //NX_KEYTYPE_PREVIOUS
13 40 //NX_KEYTYPE_FAST
14 34 //NX_KEYTYPE_REWIND
```

So now you know what (delegated) ADB codes (second column) to send to implement each of those functions.


### Build Environment

My build environment is currently Xcode 6.1, using SDK 10.8, targeting OS X 10.6.

No other build environment is supported.


### 32-bit Builds

Currently, builds are provided only for 64-bit systems.  32-bit/64-bit FAT binaries are not provided.  But you may be able build your own should you need them.  I do not test 32-bit, and there may be times when the repo is broken with respect to 32-bit builds.

Here's how to build 32-bit (universal):

- xcode 4.6.3
- open ACPIKeyboard.xcodeproj
- click on ACPIKeyboard at the top of the project tree
- select ACPIKeyboard under Project
- change Architectures to 'Standard (32/64-bit Intel)'

probably not necessary, but a good idea to check that the targets don't have overrides:
- multi-select all the Targets
- check/change Architectures to 'Standard (32/64-bit Intel)'
- build (either w/ menu or with make)

Or, if you have the command line tools installed, just run:

- For FAT binary (32-bit and 64-bit in one binary)
make BITS=3264

- For 32-bit only
make BITS=32


### Source Code:

The source code is maintained at the following sites:

https://github.com/RehabMan/OS-X-ACPI-Keyboard

https://bitbucket.org/RehabMan/os-x-acpi-keyboard


### Feedback:

Please use this thread for feedback, questions, and help:

TBD...


### Known issues:

- None yet.


### Change Log:

2014-12-17 v1.0.1

- Allow for both delegated ADB codes and non-delegated ADB codes


2014-12-16 v1.0

- Forward key events to the real PS2 driver (if found) so modifier keys work correctly.

- Set HIDVirtualDevice to keep this keyboard out of SysPrefs->Keyboard

- Fix minor problem with patch.txt ending up at Contents/Resources/patch.txt.  Add patch.txt to the ZIP.


2014-12-10 v0.9

- Created, based on my version of VoodooPS2Controller/VoodooPS2Keyboard

