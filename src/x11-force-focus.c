/*
 * deadbeef-statusnotifier-plugin - Copyright (C) 2015 Vladimir Perepechin
 *
 * x11-force-focus.c
 * Copyright (C) 2014 Vladimir Perepechin <vovochka13@gmail.com>
 *
 * This file is part of deadbeef-statusnotifier-plugin.
 *
 * deadbeef-statusnotifier-plugin is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * deadbeef-statusnotifier-plugin is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * deadbeef-statusnotifier-plugin. If not, see http://www.gnu.org/licenses/
 */

#include "x11-force-focus.h"

#if !GTK_CHECK_VERSION(3,0,0) || defined(GDK_WINDOWING_X11)

#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#include <string.h> /* memset */

#define GDK_WINDOW_DISPLAY(win)       gdk_screen_get_display (gdk_window_get_screen (win))

#define GDK_WINDOW_XROOTWIN(win)      GDK_WINDOW_XID(gdk_screen_get_root_window (gdk_window_get_screen (win)))

#if defined(GDK_IS_DISPLAY) && !defined(GDK_IS_X11_DISPLAY)
#define GDK_IS_X11_DISPLAY(display) GDK_IS_DISPLAY(display)
#endif

void
gdk_x11_window_force_focus (GdkWindow *window,
                            guint32    timestamp)
{
    GdkDisplay *display;

    g_return_if_fail (GDK_IS_WINDOW (window));

    display = GDK_WINDOW_DISPLAY (window);

    if (GTK_CHECK_VERSION(3,0,0) && !GDK_IS_X11_DISPLAY (display))
        return;

    if (gdk_x11_screen_supports_net_wm_hint (gdk_window_get_screen (window),
                        gdk_atom_intern_static_string ("_NET_ACTIVE_WINDOW")))
    {
        if (!timestamp)
        {
#if defined(GDK_CURRENT_TIME)
            timestamp = GDK_CURRENT_TIME; /* in fact it doesn't needed */
#endif
        }

        XClientMessageEvent xclient;

        memset (&xclient, 0, sizeof (xclient));
        xclient.type = ClientMessage;
        xclient.window = GDK_WINDOW_XID (window);
        xclient.message_type = gdk_x11_get_xatom_by_name_for_display (display,
                                                                    "_NET_ACTIVE_WINDOW");
        xclient.format = 32;
        xclient.data.l[0] = 2; /* requestor type; we're a tool */
        xclient.data.l[1] = timestamp;
        xclient.data.l[2] = None; /* currently active window */
        xclient.data.l[3] = 0;
        xclient.data.l[4] = 0;

        XSendEvent (GDK_DISPLAY_XDISPLAY (display), GDK_WINDOW_XROOTWIN (window), False,
                    SubstructureRedirectMask | SubstructureNotifyMask,
                    (XEvent *)&xclient);
    }
}
#else

void
gdk_x11_window_force_focus (GdkWindow *window,
                            guint32    timestamp)
{
    
}
#endif
