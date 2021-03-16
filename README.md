## StatusNotifier plugin for the DeaDBeeF music player

### About

This plugin aims to implement the StatusNotifierItem for DeaDBeeF.

It tends to replace default tray icon on DE that supports StatusNotifierIitem protocol.
It also aims to provide tray icon for deadbeef on DE where old xmbedded icons aren't supported anymore, such as KDE Plasma 5, Cinnamon, GNOME3.

__WARNING!__ GNOME3 supported using the [Appindictor Extension](https://github.com/ubuntu/gnome-shell-extension-appindicator) or similar.

__WARNING!__ Cinnamon supported using the XAppStatusIcons extension

#### Known DE comptibilty bugs
If you know how to fix certain bugs, patches are welcome.
* The window is not restored in Wayland-based environments because Wayland (and GTK3 Wayland backend) does not provide access to top-level window management. Provided the ability to block minimization using the option `Enable minimize on icon click`
* HTML notification tooltip style and icons is only supported by KDE Plasma
* GNOME3 don't support notifiction tooltips (at least for the Ubuntu Appindicator extension)
* GNOME3 SNI overlay icons work incorrect (update after refresh icon area)
* Cinnamon don't support SNI overlay icons
* In the new version of KDE Plasma (> = 5.19, Ubuntu 20.10), tooltip icons are not displayed. This is tested on KDE Plasma 5.5 (Ubuntu 16.04) and works correctly.

### Required packages
* `cmake`(optional)
* `gtk 2.x` - for gtk 2 version
* `gtk 3.x` - for gtk 3 version
* `libdbusmenu-glib-0.4` - for dbusmenu
* deadbeef development files.

#### openSUSE
You will need to add [packman](http://packman.links2linux.org/) repository for deadbeef itself (probably you already done this :) ).

`sudo zypper in cmake deadbeef-devel libdbusmenu-glib-devel gtk3-devel`

#### Fedora
You will need to find any [repository](https://copr.fedorainfracloud.org/coprs/fulltext/?fulltext=deadbeef) with for deadbeef itself and add it to your system (probably you already done this :) ).

`sudo yum install cmake deadbeef-devel libdbusmenu-devel gtk3-devel`

#### Debian/Ubuntu
You will need to find any [repository](https://launchpad.net/+search?field.text=deadbeef) with for deadbeef itself and add it to your system (probably you already done this :) ).
Package name with development files for deadbeef will depend on selected ppa.

`sudo apt-get install cmake deadbeef-plugins-dev libdbusmenu-glib-dev libgtk-3-dev`

#### Archlinux

`yaourt -S deadbeef-plugin-statusnotifier`

### How to build

#### With CMake (recommended)
``` Shell
	mkdir build && cd build
	cmake -DCMAKE_INSTALL_PREFIX=/usr -DUSE_GTK=OFF ..
	make
	make install
```
#### With Makefile
``` Shell
	make
```
Use target `gtk2` or `gtk3` for separate build. Install target NOT support. Build only gtk3 with `all` target.

### Activation

Once the plugin is installed, restart (or start) deadbeef. By default, this plugin will automatically replace default tray icon. You can change this behaviour in plugins settings (open deadbeef's preference, select plugins tab, select StatusNotifier plugin, select settings).

Note, that by default, this plugin automatically enables "hide tray icon" option in deadbeef's settings (if disabled) on startup and restores it's value on shutdown. But if shutdown was not clean (appliaction crashed, killed or something like this) then value for "hide tray icon" won't be restored.

#### Possible cmake options

* `-DUSE_GTK` - Whether to build gtk2 verion or not. Default: ON
* `-DUSE_GTK3` - Whether to build gtk3 verion or not. Default: ON
* `-DUSE_CUSTOM` - Use custom (not system) glib-mkenums script. Default: ON
* `-DCMAKE_INSTALL_PREFIX` - Where to install. Default: `/usr/local`
* `-DLIB_INSTALL_DIR` - Name of system libdir. Default: `lib`
