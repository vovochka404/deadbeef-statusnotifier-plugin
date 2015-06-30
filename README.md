## StatusNotifier plugin for the DeaDBeeF music player

### About

This plugin aims to implement the StatusNotifierItem for DeaDBeeF.

It tendeds to replace default tray icon on DE that supports StatusNotifier items.
It also aims to provide tray icon for deadbeef on KDE5/Plasma5, where old xmbedded icons aren't supported anymore.

### How to build

mkdir build && cd build  
cmake ..  
make  
make install  

### Activation

Once the plugin is installed, restart (or start) deadbeef.
Got to preference, GUI/Misc section and disable standart tray icon (hide system tray icon).  
Or you can force StatusNotifierItem to be shown in plugin preference.

#### Possible cmake options

-DUSE_GTK - Whether to build gtk2 verion or not. Default: ON  
-DUSE3_GTK - Whether to build gtk3 verion or not. Default: ON  
-DCMAKE_INSTALL_PREFIX - Where to install. Default: /usr/local  
-DLIB_INSTALL_DIR - Name of system libdir. Default: lib  