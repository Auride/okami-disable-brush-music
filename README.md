# okami-disable-brush-music
Disables the music which plays while using the Celestial Brush in the Steam version of Ōkami. Running the executable (available [here](https://github.com/Auride/okami-disable-brush-music/releases/)) toggles the patch on and off.

# How to build
A pre-built .exe file is provided under the [Releases](https://github.com/Auride/okami-disable-brush-music/releases/)) section for your convenience. If you wish to build the executable from source (e.g. because you want to modify the source file), follow these instructions:
1. Have the GCC C compiler.
   1. Open cmd.exe or Windows PowerShell at any directory. Run the command `gcc --version`.
   2. If you get a response like `gcc.exe (MinGW-W64 x86_64-ucrt-posix-seh, built by Brecht Sanders, r5) 13.2.0` you should be good to go to step 2.
   3. If you get an error, go to https://winlibs.com/ and download the version of GCC + MinGW labelled "**(LATEST)**". You most likely want the "Win64: 7-Zip archive". Save it anywhere.
   4. Open the folder you downloaded the .7z file to, then right click on it and click "Extract All..."
   5. In the window which opens, click "Browse" and then navigate to your `C:\` drive (or any other desired location), then click "Select Folder" and click the "Extract" button. Wait for a minute or two while it decompresses.
   6. Press the Windows key, then type "Env" and click "Edit the System Environment Variables". If this doesn't appear, open Windows Settings, click "System" in the top left, scroll down and click "About", click "Advanced System Settings" in the middle of the window, and click "Environment Variables" at the bottom of the smaller window which appears.
   7. In either the "User variables for <username>" or "System variables" section (it doesn't really matter which), find the "Path" variable, click it once, and then click "Edit...".
   9. On the top right of the small window which pops up, click "New", then type `C:\mingw64\bin`. If you extracted  the .7z file somewhere else, you'll need to find where, click "Browse..." after clicking New, then navigate to the bin file inside that location. Note that you must extract the .7z file first.
   10. Press "OK" until all those windows are closed.
2. Download the `toggleBrushMusic.c` file anywhere.
3. Open file explorer to the folder you downloaded the .c file to.
4. Right click (or Shift + right click) anyhere in the folder *not* on a file, then click "Open in Terminal" or "Open PowerShell window here". There's no difference.
5. Type `gcc  toggleBrushMusic.c -o  toggleBrushMusic.exe ` and press enter. This should generate the .exe file. It should be about 70 kilobytes (very small!).
6. Either double click the executable to run it, or in the same terminal window as before, type `./toggleBrushMusic.exe`. The latter will let you see the debug output of the mod.
