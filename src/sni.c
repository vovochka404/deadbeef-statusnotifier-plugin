/*
 * deadbeef-statusnotifier-plugin - Copyright (C) 2015 Vladimir Perepechin
 *
 * sni.c
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

#include "sni.h"


#define TOOLTIP_FORMAT_WO_YEAR "%s"\
        "<table><tr><td>Title:</td><td>%s</td></tr>"\
        "<tr><td>Artist:</td><td>%s</td></tr>"\
        "<tr><td>Album:</td><td>%s</td></tr></table>"
#define TOOLTIP_FORMAT "%s"\
        "<table><tr><td>Title:</td><td>%s</td></tr>"\
        "<tr><td>Artist:</td><td>%s</td></tr>"\
        "<tr><td>Album:</td><td>%s [%s]</td></tr></table>"

StatusNotifier *icon = NULL;

DB_plugin_action_t *toggle_mainwindow_action;
DB_plugin_action_t *quit_action;

void
on_activate_requested (void) {
    if (toggle_mainwindow_action) {
        toggle_mainwindow_action->callback2 (toggle_mainwindow_action, -1);
    }
    else {
        GtkWidget *mainwin = gtkui_plugin->get_mainwin ();

        int iconified = gdk_window_get_state (gtk_widget_get_window(mainwin)) & GDK_WINDOW_STATE_ICONIFIED;
        if (gtk_widget_get_visible (mainwin) && !iconified) {
            gtk_widget_hide (mainwin);
        }
        else {
            if (iconified) {
                gtk_window_deiconify (GTK_WINDOW(mainwin));
            }
            else {
                gtk_window_present (GTK_WINDOW (mainwin));
            }
        }
    }
}

void
on_sec_activate_requested (void) {
    deadbeef_toggle_play_pause();
}

void
on_scroll_requested (StatusNotifier *sn,
                     int diff,
                     StatusNotifierScrollOrientation direction)
{
    float vol = deadbeef->volume_get_db ();
    int sens = deadbeef->conf_get_int ("gtkui.tray_volume_sensitivity", 1);

    if (diff > 0) {
        vol += sens;
    }
    else {
        vol -= sens;
    }

    if (vol > 0) {
        vol = 0;
    }
    else if (vol < deadbeef->volume_get_min_db ()) {
        vol = deadbeef->volume_get_min_db ();
    }

    deadbeef->volume_set_db (vol);
}

void
sni_enable (int enable) {
    if ((icon && enable) || (!icon && !enable))
        return;

    if (enable && !icon) {
        icon = status_notifier_new_from_icon_name ("deadbeef", STATUS_NOTIFIER_CATEGORY_APPLICATION_STATUS, "deadbeef");
        status_notifier_set_status (icon, STATUS_NOTIFIER_STATUS_ACTIVE);
        status_notifier_set_title (icon, "DeaDBeeF");
        status_notifier_set_context_menu (icon, get_context_menu());

        g_signal_connect (icon, "activate", (GCallback) on_activate_requested, NULL);
        g_signal_connect (icon, "secondary-activate", (GCallback) on_sec_activate_requested, NULL);
        g_signal_connect (icon, "scroll", (GCallback) on_scroll_requested, NULL);

        status_notifier_register (icon);
    }
    else {
        g_object_unref(icon);
        icon = NULL;
    }
}


void
sni_toggle_play_pause(int play) {
    static int play_pause_state = 1;
    GtkWidget *play_item;
    GtkWidget *icon;

    if ((play_pause_state && play) || (!play_pause_state && !play))
        return;

    play_item = get_context_menu_item (SNI_MENU_ITEM_PLAY);
    icon = gtk_image_menu_item_get_image (GTK_IMAGE_MENU_ITEM (play_item));

    if (play_pause_state && !play) {
        gtk_menu_item_set_label (GTK_MENU_ITEM(play_item), _("Pause"));

        if (icon)
            gtk_image_set_from_icon_name (GTK_IMAGE (icon), "media-playback-pause", GTK_ICON_SIZE_MENU);

        play_pause_state = 0;
    }
    else {
        gtk_menu_item_set_label (GTK_MENU_ITEM (play_item), _("Play"));

        if (icon)
            gtk_image_set_from_icon_name (GTK_IMAGE (icon), "media-playback-start", GTK_ICON_SIZE_MENU);

        play_pause_state = 1;
    }
}

void _sni_update_tooltip(void *user_date);

void
sni_update_tooltip(void) {
    if (!icon)
        return;

    DB_output_t *output;
    output = deadbeef->get_output ();

    if (output) {
        int state = output->state ();

        switch (state) {
            case OUTPUT_STATE_STOPPED:
                status_notifier_set_tooltip (icon, "deadbeef", "DeaDBeeF", _("Playback stopped"));
                break;
            case OUTPUT_STATE_PAUSED:
            case OUTPUT_STATE_PLAYING:
            {
                DB_playItem_t *track = deadbeef->streamer_get_playing_track();
                static gchar *ns = NULL;
				if (!ns)
					ns = _("not specified");

                int64_t duration = (int64_t)deadbeef->pl_get_item_duration (track) * 1000000;
                const char *album = deadbeef->pl_find_meta (track, "album");
                const char *albumArtist = deadbeef->pl_find_meta (track, "albumartist");
                if (albumArtist == NULL)
                    albumArtist = deadbeef->pl_find_meta (track, "album artist");
                if (albumArtist == NULL)
                    albumArtist = deadbeef->pl_find_meta (track, "band");
                const char *artist = deadbeef->pl_find_meta (track, "artist");
                const char *date = deadbeef->pl_find_meta_raw (track, "year");
                const char *title = deadbeef->pl_find_meta (track, "title");
                const char *trackNumber = deadbeef->pl_find_meta (track, "track");

                gchar *escaped_title  = title  ? g_markup_escape_text (title, -1) : NULL;
                gchar *escaped_artist = (artist ? artist : albumArtist) ? g_markup_escape_text (artist ? artist : albumArtist, -1) : NULL;
                gchar *escaped_album  = album  ? g_markup_escape_text (album, -1) : NULL;
                gchar *escaped_date   = date   ? g_markup_escape_text (date, -1)  : NULL;

                gchar title_body[1000];
                date ?
                    g_sprintf (title_body, _(TOOLTIP_FORMAT), state == OUTPUT_STATE_PAUSED ? _("Playback paused<br>\n") : "",
                               escaped_title  ? escaped_title  : ns,
                               escaped_artist ? escaped_artist : ns,
                               escaped_album  ? escaped_album  : ns,
                               escaped_date) :
                    g_sprintf (title_body, _(TOOLTIP_FORMAT_WO_YEAR), state == OUTPUT_STATE_PAUSED ? _("Playback paused<br>\n") : "",
                               escaped_title  ? escaped_title  : ns,
                               escaped_artist ? escaped_artist : ns,
                               escaped_album  ? escaped_album  : ns);

                g_free(escaped_title);
                g_free(escaped_artist);
                g_free(escaped_album);
                g_free(escaped_date);

                GdkPixbuf * buf = gtkui_plugin->get_cover_art_pixbuf (deadbeef->pl_find_meta (track, ":URI"), artist, album, 128, _sni_update_tooltip, NULL);
                if (!buf) {
                    buf = gtkui_plugin->cover_get_default_pixbuf ();

                    if (buf)
                        status_notifier_set_from_pixbuf (icon, STATUS_NOTIFIER_TOOLTIP_ICON, buf);
                    else
                        status_notifier_set_from_icon_name (icon, STATUS_NOTIFIER_TOOLTIP_ICON, "deadbeef");
                }
                else
                    status_notifier_set_from_pixbuf (icon, STATUS_NOTIFIER_TOOLTIP_ICON, buf);

                status_notifier_set_tooltip_body (icon, title_body);

                deadbeef->pl_item_unref (track);
                break;
            }
        }
    }
    else {
        status_notifier_set_tooltip (icon, "deadbeef", "DeaDBeeF", NULL);
    }
}

void _sni_update_tooltip (void *user_date) {
    sni_update_tooltip ();
}

void
sni_update_status (void) {
    DB_output_t *output;
    GtkWidget *stop_item;

    if (!icon)
        return;

    output = deadbeef->get_output ();

    if (output) {
        switch (output->state ()) {
            case OUTPUT_STATE_PLAYING:
                status_notifier_set_from_icon_name (icon, STATUS_NOTIFIER_OVERLAY_ICON, "media-playback-start");

                stop_item = get_context_menu_item (SNI_MENU_ITEM_STOP);
                gtk_widget_set_sensitive (stop_item, TRUE);

                sni_toggle_play_pause (0);
                break;
            case OUTPUT_STATE_PAUSED:
                status_notifier_set_from_icon_name (icon, STATUS_NOTIFIER_OVERLAY_ICON, "media-playback-pause");

                sni_toggle_play_pause (1);
                break;
            case OUTPUT_STATE_STOPPED:
                status_notifier_set_from_icon_name (icon, STATUS_NOTIFIER_OVERLAY_ICON, NULL);

                stop_item = get_context_menu_item (SNI_MENU_ITEM_STOP);
                gtk_widget_set_sensitive (stop_item, FALSE);

                sni_toggle_play_pause (1);
                break;
        }
    }
    sni_update_tooltip ();
}


///////////////////////////////////
// Common deadbeef plugin stuff
///////////////////////////////////

void
deadbeef_toggle_play_pause (void) {
        DB_output_t *output;

    output = deadbeef->get_output ();
    if (output) {
        switch (output->state ()) {
            case OUTPUT_STATE_PLAYING:
                deadbeef->sendmessage (DB_EV_TOGGLE_PAUSE, 0, 0, 0);
                return;
            case OUTPUT_STATE_PAUSED:
                deadbeef->sendmessage (DB_EV_TOGGLE_PAUSE, 0, 0, 0);
                return;
        }
    }
    deadbeef->sendmessage (DB_EV_PLAY_CURRENT, 0, 0, 0);
}

void
sni_configchanged (void) {
    int enabled = deadbeef->conf_get_int ("sni.enabled", 1);
    int check_std_icon = deadbeef->conf_get_int ("sni.check_std_icon", 1);
    int hide_tray_icon = deadbeef->conf_get_int ("gtkui.hide_tray_icon", 0);
    sni_enable (enabled && ((check_std_icon && hide_tray_icon) || !check_std_icon));
}

static int
sni_message (uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    switch (id) {
    case DB_EV_CONFIGCHANGED:

        sni_configchanged ();
        break;
    case DB_EV_PAUSED:
    case DB_EV_TOGGLE_PAUSE:
    case DB_EV_SONGCHANGED:
    case DB_EV_SONGSTARTED:
    case DB_EV_SONGFINISHED:
    case DB_EV_TRACKINFOCHANGED:

        sni_update_status ();
        break;
    }
    return 0;
}

int
sni_connect() {
    gtkui_plugin = (ddb_gtkui_t *)deadbeef->plug_get_for_id (DDB_GTKUI_PLUGIN_ID);
    if (!gtkui_plugin) {
        fprintf (stderr, "sni: can't find gtkui plugin\n");
        return -1;
    }

    DB_plugin_action_t *actions = gtkui_plugin->gui.plugin.get_actions(NULL);
    while (actions) {
        if (g_strcmp0(actions->name, "toggle_player_window") == 0) {
            toggle_mainwindow_action = actions;
            break;
        }
        actions = actions->next;
    }

    if (!toggle_mainwindow_action) {
        fprintf (stderr, "sni: failed to find \"toggle_player_window\" gtkui plugin\n");
    }

    sni_configchanged ();
    return 0;
}


static const char settings_dlg[] =
    "property \"Enable StatusNotifierItem\" checkbox sni.enabled 1;\n"
    "property \"Allow only if standart GUI tray icon is disabled\" checkbox sni.check_std_icon 1;\n"
;


static DB_misc_t plugin = {
    .plugin.type = DB_PLUGIN_MISC,
    .plugin.api_vmajor = 1,
    .plugin.api_vminor = 5,
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
#if GTK_CHECK_VERSION(3,0,0)
    .plugin.id = "sni_gtk3",
    .plugin.name = "StatusNotifierItem for GTK3 UI",
#else
    .plugin.id = "sni_gtk2",
    .plugin.name = "StatusNotifierItem for GTK2 UI",
#endif
    .plugin.descr = "StatusNotifierItem for DE without support for xembedded icons\n"
    "(like plasma5). It also can be used for a better look&feel experience.\n",
    .plugin.copyright = 
        "StatusNotifier plugin for DeaDBeeF Player\n"
        "Copyright (C) 2015 Vladimir Perepechin <vovochka13@gmail.com>\n"
        "\n"
        "This program is free software: you can redistribute it and/or modify\n"
        "it under the terms of the GNU General Public License as published by\n"
        "the Free Software Foundation, either version 3 of the License, or\n"
        "(at your option) any later version.\n"

        "This program is distributed in the hope that it will be useful,\n"
        "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
        "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
        "GNU General Public License for more details.\n"

        "You should have received a copy of the GNU General Public License\n"
        "along with this program.  If not, see <http://www.gnu.org/licenses/>.\n",
    .plugin.website = "http://example.com",
    .plugin.configdialog = settings_dlg,
    .plugin.message = sni_message,
    .plugin.connect = sni_connect,
};

DB_plugin_t *
#if GTK_CHECK_VERSION(3,0,0)
sni_gtk3_load (DB_functions_t *api) {
#else
sni_gtk2_load (DB_functions_t *api) {
#endif
    deadbeef = api;
    return DB_PLUGIN(&plugin);
}
