# Prologue by Fruit-Bat
This is a wonderful game and I wanted to play it on my Pi 5.
I forked it to try and fix some rendering artefacts, in particular gaps in the landscape and a bar down the right-hand side of the screen.

<table>
	<tr>
		<td><img src="./screen_shots/hurrican_poke.png" /></td>
		<td><img src="./screen_shots/hurrican_menu_1.png" /></td>
	</tr>
	<tr>
		<td><img src="./screen_shots/hurrican_intro_1.png" /></td>
		<td><img src="./screen_shots/hurrican_intro_2.png" /></td>
	</tr>
	<tr>
		<td><img src="./screen_shots/hurrican_gameplay_5.png" /></td>
		<td><img src="./screen_shots/hurrican_gameplay_3.png" /></td>
	</tr>	
</table>

Note that. there is also a version [here](https://github.com/fruit-bat/HurricanGame), which may be more up-to-date. 

## Build instructions
```
sudo apt install libsdl1.2-dev libsdl-image1.2-dev libsdl-mixer1.2-dev libmodplug-dev libgles-dev
git clone https://github.com/fruit-bat/Hurrican
cd Hurrican/Hurrican/src
make TARGET=linux_GL2
cd ..
./hurricanlinux -NV
```

Have fun,

--- Fruit-Bat ---

# Readme by Poke53280
```
Hi guys,

I decided to release the sourcecode for Hurrican and its level editor to the public today, along with all the assets that the game contains.
You can take everything contained within this archive and do with it whatever you find appropriate (as long as you don't charge any money for it):
- modify it
- learn from it
- laugh about it
- use it to port the game to other platforms
- fix bugs :D
- release your own game

I have not touched these files for years, so don't expect that everything works out of the box! I trust in your skills to make it all run :)
Of course, I cannot be made responsible for any damage that this code might cause, physical or mental!

If you plan to use assets from the game in a COMMERCIAL release, please contact me at
info@eiswuxe.de

It would be nice if you could give some credit to Poke53280 if you choose to use anything contained within this archive in your own releases. But it's not necessary. Since I would like to know how far this stuff travels, I would love to hear about your releases, just to see what you make of this old stuff. So feel free to tell me about your projects :)

cheers and have fun,
Eiswuxe


SDL/OpenGL-ES Port Information

Thanks to the release by Eiswuxe we now have the privilege of enhancing Hurrican. I have extended the source code to be compatible with SDL systems that are OpenGL-ES compatible.
All changes by myself within exisiting files are under the same copyright as the original source. All original files are under the MIT license.

Build Instructions
In general these instructions should work for most linux versions:
cd src && make

Definitions:
  Platform Type
	PLATFORM_DIRECTX : Use the original directx code
	PLATFORM_SDL     : Use the new SDL/OpenGL code
	__WIN32__	 : Use on windows builds
  OpenGL Options:
  	EGL		 : see SDLPort/eglport.h
	USE_GL1          : Use the OpenGL 1.X code (fixed pipline)
	USE_GLES1        : Use the OpenGL 1.X code with ES compatible (requires USE_GL1)
	USE_GL2          : Use the OpenGL 2.0 code (programable pipline)
	USE_GLES2        : Use the OpenGL 2.0 code with ES compatible (requires USE_GL2)
	USE_PVRTC	 : Use ImgTec's PVRTC texture compression (only for PVR gpu's)
  Sound:
  	USE_MODPLUG      : Use the stable modplug code for music. (otherwise mikmod is used, which is known to have problems)
  	
  Other:
  	ENABLE_CONSOLE_COMMANDS : turns a console where commands can be entered
  	_DEBUG			: enables some debug output
  	
  	
 Typical desktop build would use: -DPLATFORM_SDL -DUSE_GL2 -DUSE_MODPLUG -DENABLE_CONSOLE_COMMANDS
 An mobile device may use : -DPLATFORM_SDL -DUSE_GL2 -DUSE_GLES2 -DUSE_EGL_SDL -DUSE_MODPLUG -DENABLE_CONSOLE_COMMANDS
 Check the makefile for other examples.
 
 Pickle (pickle136@gmail.com)

Note: For cross-platform file/directory handling, the SDL port makes use of the cross-platform 'tinydir' library. Much thanks to Cong Xu, the author. https://github.com/cxong/tinydir Copyright (c) 2013, Cong Xu  - Simplified BSD License.
```
