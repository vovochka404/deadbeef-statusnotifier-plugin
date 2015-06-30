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

#### Possible cmake options

-DUSE_GTK - Whether to build gtk2 verion or not. Default: ON  
-DUSE3_GTK - Whether to build gtk3 verion or not. Default: ON  
-DCMAKE_INSTALL_PREFIX - Where to install. Default: /usr/local  
-DLIB_INSTALL_DIR - Name of system libdir. Default lib  