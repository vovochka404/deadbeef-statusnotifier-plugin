## StatusNotifier plugin for the DeaDBeeF music player

### About

This plugin aims to implement the StatusNotifierItem for DeaDBeeF.

It tends to replace default tray icon on DE that supports StatusNotifier items.
It also aims to provide tray icon for deadbeef on KDE5/Plasma5, where old xmbedded icons aren't supported anymore.

### Required packages
cmake  
gtk 2.x - for gtk 2 version  
gtk 3.x - for gtk 3 version  
libdbusmenu-glib-0.4 - for dbusmenu  
deadbeef development files.  

#### openSUSE
You will need to add [packman](http://packman.links2linux.org/) repository for deadbeef itself (probably you already done this :) ).  
`zypper in cmake deadbeef-devel libdbusmenu-glib-devel gtk3-devel`

#### Fedora
You will need to find any [repository](https://copr.fedorainfracloud.org/coprs/fulltext/?fulltext=deadbeef) with for deadbeef itself and add it to your system (probably you already done this :) ).  
`yum install cmake deadbeef-devel libdbusmenu-devel gtk3-devel`

#### Debian/Ubuntu
You will need to find any [repository](https://launchpad.net/+search?field.text=deadbeef) with for deadbeef itself and add it to your system (probably you already done this :) ).  
Package name with development files for deadbeef will depend on selected ppa.  
`apt-get install cmake deadbeef-plugins-dev libdbusmenu-glib-dev libgtk-3-dev`

### How to build

	mkdir build && cd build
	cmake -DCMAKE_INSTALL_PREFIX=/usr -DUSE_GTK=OFF ..
	make
	make install

### Activation

Once the plugin is installed, restart (or start) deadbeef.
Go to preference, GUI/Misc section and disable standart tray icon (hide system tray icon).  
Or you can force StatusNotifierItem to be shown in plugin settings.

#### Possible cmake options

-DUSE_GTK - Whether to build gtk2 verion or not. Default: ON  
-DUSE3_GTK - Whether to build gtk3 verion or not. Default: ON  
-DCMAKE_INSTALL_PREFIX - Where to install. Default: /usr/local  
-DLIB_INSTALL_DIR - Name of system libdir. Default: lib  
