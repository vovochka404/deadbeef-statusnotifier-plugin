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
#include "x11-force-focus.h"


#define TOOLTIP_FORMAT_WO_YEAR "%s"\
        "<b>Title:</b> %s<br/>"\
        "<b>Artist:</b> %s<br/>"\
        "<b>Album:</b> %s"
#define TOOLTIP_FORMAT "%s"\
        "<b>Title: </b>%s<br/>"\
        "<b>Artist:</b> %s<br/>"\
        "<b>Album:</b> %s [%s]"
#define TOOLTIP_MAX_LENGTH 1000

static gboolean auto_activated = FALSE;
static volatile gboolean sni_loaded = FALSE;

static StatusNotifier *icon = NULL;

static DB_plugin_action_t *toggle_mainwindow_action = NULL;
static DB_plugin_action_t *preferences_action = NULL;

DB_functions_t *deadbeef = NULL;
static ddb_gtkui_t *gtkui_plugin;
static DB_misc_t plugin;

void sni_update_status ();

DB_functions_t *
deadbeef_get_instance (void) {
    return deadbeef;
}

void
on_activate_requested (void) {
    if (toggle_mainwindow_action && 0) {
        toggle_mainwindow_action->callback2 (toggle_mainwindow_action, -1);
    }
    else {
        GtkWidget *mainwin = gtkui_plugin->get_mainwin ();
        GdkWindow *gdk_window = gtk_widget_get_window (mainwin);

        int iconified = gdk_window_get_state (gdk_window) & GDK_WINDOW_STATE_ICONIFIED;
        if (gtk_widget_get_visible (mainwin) && !iconified) {
            gtk_widget_hide (mainwin);
        }
        else {
            if (iconified) {
                gtk_window_deiconify (GTK_WINDOW (mainwin));
            }
            else {
                gtk_window_present (GTK_WINDOW (mainwin));
            }
            gtk_window_move(GTK_WINDOW (mainwin),
                            deadbeef->conf_get_int("mainwin.geometry.x", 0),
                            deadbeef->conf_get_int("mainwin.geometry.y", 0));
            
            gdk_x11_window_force_focus (gdk_window, 0);
        }
    }
}

void
on_sec_activate_requested (void) {
    deadbeef_toggle_play_pause ();
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

static void 
callback_wait_notifier_register (void* ctx) {
    StatusNotifierState state = STATUS_NOTIFIER_STATE_NOT_REGISTERED;
    StatusNotifier* sni_ctx = (StatusNotifier*)ctx;

    status_notifier_register (sni_ctx);
    
    while (TRUE) {
        state = status_notifier_get_state(sni_ctx);
        if (state == STATUS_NOTIFIER_STATE_REGISTERED) {
            sleep(1);
            sni_loaded = TRUE;
            sni_update_status(-1);
            deadbeef->log_detailed((DB_plugin_t*)(&plugin), DDB_LOG_LAYER_INFO,
                                    "%s: %s\n","Status notifier register success", status_notifier_get_id(sni_ctx));
            return;
        }
        if (state == STATUS_NOTIFIER_STATE_FAILED) {
            deadbeef->log_detailed((DB_plugin_t*)(&plugin), DDB_LOG_LAYER_DEFAULT,
                                    "%s: %s\n","Status notifier register failed", status_notifier_get_id(sni_ctx));
            return;
        }
    }
}

void
sni_enable (int enable) {
    if ((icon && enable) || (!icon && !enable))
        return;

    if (enable && !icon) {
        icon = status_notifier_new_from_icon_name ("deadbeef", STATUS_NOTIFIER_CATEGORY_APPLICATION_STATUS, "deadbeef");
        status_notifier_set_status (icon, STATUS_NOTIFIER_STATUS_ACTIVE);
        status_notifier_set_title (icon, "DeaDBeeF");
        status_notifier_set_context_menu (icon, get_context_menu ());

        g_signal_connect (icon, "activate", (GCallback) on_activate_requested, NULL);
        g_signal_connect (icon, "secondary-activate", (GCallback) on_sec_activate_requested, NULL);
        g_signal_connect (icon, "scroll", (GCallback) on_scroll_requested, NULL);
        
        // Waiting notifier register process in separate thread
        deadbeef->thread_start(callback_wait_notifier_register, (void*)icon);
    }
    else {
        g_object_unref (icon);
        icon = NULL;
    }
}


void
sni_toggle_play_pause (int play) {
    static int play_pause_state = 1;
    DbusmenuMenuitem *play_item;

    if ((play_pause_state && play) || (!play_pause_state && !play))
        return;

    play_item = get_context_menu_item (SNI_MENU_ITEM_PLAY);

    if (play_pause_state && !play) {
        dbusmenu_menuitem_property_set (play_item, DBUSMENU_MENUITEM_PROP_LABEL, _("Pause"));
        dbusmenu_menuitem_property_set (play_item, DBUSMENU_MENUITEM_PROP_ICON_NAME, "media-playback-pause");

        play_pause_state = 0;
    }
    else {
        dbusmenu_menuitem_property_set (play_item, DBUSMENU_MENUITEM_PROP_LABEL, _("Play"));
        dbusmenu_menuitem_property_set (play_item, DBUSMENU_MENUITEM_PROP_ICON_NAME, "media-playback-start");

        play_pause_state = 1;
    }
}

void _sni_update_tooltip (void *user_date);

void
sni_update_tooltip (int state) {
    if (!icon)
        return;

    g_debug("sni_update_tooltip, status: %d", state);
    DB_output_t *output;
    output = deadbeef->get_output ();

    if (output) {
        if (state < 0)
            state = output->state ();

        switch (state) {
            case DDB_PLAYBACK_STATE_STOPPED:
                status_notifier_set_tooltip (icon, "deadbeef", "DeaDBeeF", _("Playback stopped"));
                break;
            case DDB_PLAYBACK_STATE_PAUSED:
            case DDB_PLAYBACK_STATE_PLAYING:
            {
                DB_playItem_t *track = deadbeef->streamer_get_playing_track ();
                static gchar *ns = NULL;
                if (!ns)
                    ns = _("not specified");

                if (!track) {
                    status_notifier_set_tooltip (icon, "deadbeef", "DeaDBeeF", _("Playing"));
                    break;
                }

                int64_t duration  = (int64_t)deadbeef->pl_get_item_duration (track) * 1000000;
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

                gchar title_body[TOOLTIP_MAX_LENGTH];
                date ?
                    g_snprintf (title_body, TOOLTIP_MAX_LENGTH, _(TOOLTIP_FORMAT), state == OUTPUT_STATE_PAUSED ? _("Playback paused<br>\n") : "",
                               escaped_title  ? escaped_title  : ns,
                               escaped_artist ? escaped_artist : ns,
                               escaped_album  ? escaped_album  : ns,
                               escaped_date) :
                    g_snprintf (title_body, TOOLTIP_MAX_LENGTH, _(TOOLTIP_FORMAT_WO_YEAR), state == OUTPUT_STATE_PAUSED ? _("Playback paused<br>\n") : "",
                               escaped_title  ? escaped_title  : ns,
                               escaped_artist ? escaped_artist : ns,
                               escaped_album  ? escaped_album  : ns);

                g_free (escaped_title);
                g_free (escaped_artist);
                g_free (escaped_album);
                g_free (escaped_date);

                g_debug("Going to query coverart");
#if (DDB_GTKUI_API_LEVEL >= 202)
                GdkPixbuf * buf = gtkui_plugin->get_cover_art_primary (deadbeef->pl_find_meta (track, ":URI"), artist, album, 128, NULL, NULL);
#else
                GdkPixbuf * buf = gtkui_plugin->get_cover_art_pixbuf  (deadbeef->pl_find_meta (track, ":URI"), artist, album, 128, NULL, NULL);
#endif
                g_debug("Got GdbPixbuf: %d", (int) buf);
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
    sni_update_tooltip (-1);
}

void
sni_update_status (int state) {
    g_debug("sni_update_status, status: %d", state);
    DB_output_t *output;
    DbusmenuMenuitem *stop_item;
    
    if (!icon)
        return;
    if (sni_loaded == FALSE)
        return;

    output = deadbeef->get_output ();

    if (output) {
        if (state < 0)
            state = output->state ();
        
        // FIXME Data race. Output maybe not initialized and returned invalid state value.
        // Temporary FIX - use sleep(1) in watchdog 

        switch (state) {
            case DDB_PLAYBACK_STATE_PLAYING:
                status_notifier_set_from_icon_name (icon, STATUS_NOTIFIER_OVERLAY_ICON, "media-playback-start");
                stop_item = get_context_menu_item (SNI_MENU_ITEM_STOP);
                dbusmenu_menuitem_property_set_bool (stop_item, DBUSMENU_MENUITEM_PROP_ENABLED, TRUE);

                sni_toggle_play_pause (0);
                break;
            case DDB_PLAYBACK_STATE_PAUSED:
                status_notifier_set_from_icon_name (icon, STATUS_NOTIFIER_OVERLAY_ICON, "media-playback-pause");
                sni_toggle_play_pause (1);
                break;
            case DDB_PLAYBACK_STATE_STOPPED:
                status_notifier_set_from_icon_name (icon, STATUS_NOTIFIER_OVERLAY_ICON, NULL);
                stop_item = get_context_menu_item (SNI_MENU_ITEM_STOP);
                dbusmenu_menuitem_property_set_bool (stop_item, DBUSMENU_MENUITEM_PROP_ENABLED, FALSE);

                sni_toggle_play_pause (1);
                break;
        }
    }
    sni_update_tooltip (state);
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
            case DDB_PLAYBACK_STATE_PLAYING:
                deadbeef->sendmessage (DB_EV_TOGGLE_PAUSE, 0, 0, 0);
                return;
            case DDB_PLAYBACK_STATE_PAUSED:
                deadbeef->sendmessage (DB_EV_TOGGLE_PAUSE, 0, 0, 0);
                return;
        }
    }
    deadbeef->sendmessage (DB_EV_PLAY_CURRENT, 0, 0, 0);
}

gboolean
deadbeef_preferences_available (void) {
    return preferences_action != NULL;
}

void
deadbeef_preferences_activate (void) {
    preferences_action->callback2 (preferences_action, 0);
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
        g_debug("Event: DB_EV_CONFIGCHANGED");
        sni_configchanged ();
        update_playback_controls ();
        break;

    case DB_EV_TRACKINFOCHANGED:
        g_debug("Event: DB_EV_TRACKINFOCHANGED");
        sni_update_tooltip (-1);
        break;

    case DB_EV_PAUSED:
        g_debug("Event: DB_EV_PAUSED");
        sni_update_status (-1);
        break;
    case DB_EV_SONGCHANGED:
        {
            ddb_event_trackchange_t* ev_change = (ddb_event_trackchange_t*)ctx;
            if (ev_change->to == NULL) {
                g_debug("Event: DB_EV_SONGCHANGED");
                sni_update_status (DDB_PLAYBACK_STATE_STOPPED);
            }
            break;
        }
    case DB_EV_STOP:
        g_debug("Event: DB_EV_STOP");
        sni_update_status (DDB_PLAYBACK_STATE_STOPPED);
        break;

    case DB_EV_SONGSTARTED:
        g_debug("Event: DB_EV_SONGSTARTED");
        sni_update_status (DDB_PLAYBACK_STATE_PLAYING);
        break;
    }
    return 0;
}

int
sni_connect () {
    gtkui_plugin = (ddb_gtkui_t *)deadbeef->plug_get_for_id (DDB_GTKUI_PLUGIN_ID);
    if (!gtkui_plugin) {
        deadbeef->log_detailed((DB_plugin_t*)(&plugin), DDB_LOG_LAYER_DEFAULT, "sni: can't find gtkui plugin\n");
        return -1;
    }

    DB_plugin_action_t *actions = gtkui_plugin->gui.plugin.get_actions (NULL);
    while (actions) {
        if (g_strcmp0 (actions->name, "toggle_player_window") == 0) {
            toggle_mainwindow_action = actions;
        }
        else if (g_strcmp0 (actions->name, "preferences") == 0) {
            preferences_action = actions;
        }
        actions = actions->next;
    }

    if (!toggle_mainwindow_action) {
        deadbeef->log_detailed ((DB_plugin_t*)(&plugin), DDB_LOG_LAYER_DEFAULT, "sni: failed to find \"toggle_player_window\" gtkui plugin\n");
    }

    int enabled = deadbeef->conf_get_int ("sni.enabled", 1);
    int enable_automaticaly = deadbeef->conf_get_int ("sni.enable_automaticaly", 1);
    int hide_tray_icon = deadbeef->conf_get_int ("gtkui.hide_tray_icon", 0);

    if (enabled && enable_automaticaly && !hide_tray_icon) {
        auto_activated = TRUE;
        deadbeef->conf_set_int ("gtkui.hide_tray_icon", 1);
    }
    else
        sni_configchanged ();

    return 0;
}

int
sni_disconnect () {
    if (auto_activated) {
        deadbeef->conf_set_int ("gtkui.hide_tray_icon", 0);
    }
    return 0;
}


static const char settings_dlg[] =
    "property \"Enable StatusNotifierItem\" checkbox sni.enabled 1;\n"
    "property \"Allow only if standart GUI tray icon is disabled\" checkbox sni.check_std_icon 1;\n"
    "property \"Automaticly disable standart GUI tray icon\" checkbox sni.enable_automaticaly 1;\n"
;


static DB_misc_t plugin = {
    .plugin.type = DB_PLUGIN_MISC,
    .plugin.flags = DDB_PLUGIN_FLAG_LOGGING,
    .plugin.api_vmajor = 1,
    .plugin.api_vminor = 11,
    .plugin.version_major = 1,
    .plugin.version_minor = 3,
#if GTK_CHECK_VERSION (3, 0, 0)
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
    .plugin.website = "https://github.com/vovochka404/deadbeef-statusnotifier-plugin",
    .plugin.configdialog = settings_dlg,
    .plugin.message = sni_message,
    .plugin.connect = sni_connect,
    .plugin.disconnect = sni_disconnect,
};

DB_plugin_t *
#if GTK_CHECK_VERSION (3 ,0, 0)
sni_gtk3_load (DB_functions_t *api) {
#else
sni_gtk2_load (DB_functions_t *api) {
#endif
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
