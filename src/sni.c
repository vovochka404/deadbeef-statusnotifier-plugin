/*
 * deadbeef-statusnotifier-plugin - Copyright (C) 2015 Vladimir Perepechin
 *
 * sni.c
 * Copyright (C) 2014 Vladimir Perepechin <vovochka13@gmail.com>
 *
 * This file is part of deadbeef-statusnotifier-plugin.
 *
 * deadbeef-statusnotifier-plugin is free software: you can redistribute it
 * and/or modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the License,
 * or (at your option) any later version.
 *
 * deadbeef-statusnotifier-plugin is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * deadbeef-statusnotifier-plugin. If not, see http://www.gnu.org/licenses/
 */

#include "sni.h"

static StatusNotifier *icon = NULL;

static DB_plugin_action_t *toggle_mainwindow_action = NULL;
static DB_plugin_action_t *preferences_action = NULL;
static DB_plugin_action_t *help_action = NULL;

static DB_functions_t *deadbeef = NULL;
static ddb_gtkui_t *gtkui_plugin;
static DB_misc_t plugin;

DB_functions_t *
deadbeef_get_instance(void) {
    return deadbeef;
}

void
deadbeef_toggle_play_pause(void);

#include "sni_flags.c"
#include "sni_upd.c"
#include "sni_timer.c"
#include "x11-force-focus.c"

static void
on_activate_requested(void) {
    if (toggle_mainwindow_action && 0) {
        toggle_mainwindow_action->callback2(toggle_mainwindow_action, -1);
    } else {
        GtkWidget *mainwin = gtkui_plugin->get_mainwin();
        GdkWindow *gdk_window = gtk_widget_get_window(mainwin);

        int iconified = gdk_window_get_state(gdk_window) & GDK_WINDOW_STATE_ICONIFIED;
        if (gtk_widget_get_visible(mainwin) && !iconified) {
            gtk_widget_hide(mainwin);
        } else {
            (iconified) ? gtk_window_deiconify(GTK_WINDOW(mainwin))
                        : gtk_window_present(GTK_WINDOW(mainwin));

            gtk_window_move(GTK_WINDOW(mainwin), deadbeef->conf_get_int("mainwin.geometry.x", 0),
                            deadbeef->conf_get_int("mainwin.geometry.y", 0));

            gdk_x11_window_force_focus(gdk_window, 0);
        }
    }
}

static void
on_sec_activate_requested(void) {
    deadbeef_toggle_play_pause();
}

static void
on_scroll_requested(StatusNotifier *sn, int diff, StatusNotifierScrollOrientation direction) {
    if (deadbeef->conf_get_int("sni.volume_hdirect_ignore", 1))
        if (direction == STATUS_NOTIFIER_SCROLL_ORIENTATION_HORIZONTAL)
            return;

    if (deadbeef->conf_get_int("sni.volume_reverse", 0))
        diff *= -1;

    float vol = deadbeef->volume_get_db();
    int sens = deadbeef->conf_get_int("gtkui.tray_volume_sensitivity", 1);

    if (diff) {
        vol = (diff > 0) ? vol + sens : vol - sens;
    }
    if (vol > 0) {
        vol = 0;
    } else if (vol < deadbeef->volume_get_min_db()) {
        vol = deadbeef->volume_get_min_db();
    }

    deadbeef->volume_set_db(vol);
}

/* Protection against manual configuration changes */
static inline int
get_gtkui_refresh_rate(void) {
    int fps = deadbeef->conf_get_int("gtkui.refresh_rate", 10);
    if (fps < 1) {
        fps = 1;
    } else if (fps > 30) {
        fps = 30;
    }
    return fps;
}

static void
callback_wait_notifier_register(void *ctx) {
    StatusNotifierState state = STATUS_NOTIFIER_STATE_NOT_REGISTERED;

    StatusNotifier *sni_ctx = (StatusNotifier *)ctx;
    if (sni_ctx == NULL)
        return;

    status_notifier_register(sni_ctx);

    uint32_t wait_time = deadbeef->conf_get_int("sni.waiting_load_sec", 30);
    for (uint32_t i = 0; i < wait_time; i++) {
        state = status_notifier_get_state(sni_ctx);
        if (state == STATUS_NOTIFIER_STATE_REGISTERED) {

            sni_flag_set(SNI_FLAG_LOADED);
            sni_timer_init(icon, 1000 / get_gtkui_refresh_rate());

            deadbeef->log_detailed((DB_plugin_t *)(&plugin), DDB_LOG_LAYER_INFO, "%s: %s\n",
                                   "Status notifier register success",
                                   status_notifier_get_id(sni_ctx));
            return;
        }
        if (state == STATUS_NOTIFIER_STATE_FAILED) {
            deadbeef->log_detailed((DB_plugin_t *)(&plugin), DDB_LOG_LAYER_DEFAULT, "%s: %s\n",
                                   "Status notifier register failed",
                                   status_notifier_get_id(sni_ctx));
            return;
        }
        sleep(1); // Timer tick
    }
    deadbeef->log_detailed((DB_plugin_t *)(&plugin), DDB_LOG_LAYER_DEFAULT, "%s: %s\n",
                           "Status notifier register failed (by timeout)",
                           status_notifier_get_id(sni_ctx));
}

static void
sni_reload_icon(gboolean enable) {
    if ((icon && enable) || (!icon && !enable))
        return;

    if (enable && !icon) {
        if (sni_context_menu_create() == 0) {
            icon = status_notifier_new_from_icon_name(
                "deadbeef", STATUS_NOTIFIER_CATEGORY_APPLICATION_STATUS, "deadbeef");
            status_notifier_set_status(icon, STATUS_NOTIFIER_STATUS_ACTIVE);
            status_notifier_set_title(icon, "DeaDBeeF");
            status_notifier_set_context_menu(icon, get_context_menu());

            g_signal_connect(icon, "activate", (GCallback)on_activate_requested, NULL);
            g_signal_connect(icon, "secondary-activate", (GCallback)on_sec_activate_requested,
                             NULL);
            g_signal_connect(icon, "scroll", (GCallback)on_scroll_requested, NULL);

            // Waiting notifier register process in separate thread
            deadbeef->thread_start(callback_wait_notifier_register, (void *)icon);
        } else
            deadbeef->log_detailed((DB_plugin_t *)(&plugin), DDB_LOG_LAYER_INFO, "%s\n",
                                   "DBus menu don't create");
    } else {
        sni_timer_free();
        g_object_unref(icon);
        icon = NULL;
    }
}

///////////////////////////////////
// Common deadbeef plugin stuff
///////////////////////////////////

void
deadbeef_toggle_play_pause(void) {
    DB_output_t *output = deadbeef->get_output();
    if (output) {
        switch (output->state()) {
        case DDB_PLAYBACK_STATE_PLAYING:
        case DDB_PLAYBACK_STATE_PAUSED:
            deadbeef->sendmessage(DB_EV_TOGGLE_PAUSE, 0, 0, 0);
            return;
        case DDB_PLAYBACK_STATE_STOPPED:
            break;
        }
    }
    deadbeef->sendmessage(DB_EV_PLAY_CURRENT, 0, 0, 0);
}

gboolean
deadbeef_preferences_available(void) {
    return preferences_action != NULL;
}

void
deadbeef_preferences_activate(void) {
    preferences_action->callback2(preferences_action, 0);
}

gboolean
deadbeef_help_available(void) {
    return help_action != NULL;
}

void
deadbeef_help_activate(void) {
    help_action->callback2(help_action, 0);
}

static void
sni_configchanged(void) {
    if (deadbeef->conf_get_int("sni.enabled", 1)) {
        sni_flag_set(SNI_FLAG_ENABLED);
        deadbeef->conf_set_int("gtkui.hide_tray_icon", 1);
    } else {
        sni_flag_unset(SNI_FLAG_ENABLED);
        deadbeef->conf_set_int("gtkui.hide_tray_icon", 0);
    }

    sni_reload_icon(sni_flag_get(SNI_FLAG_ENABLED));
}

static int
sni_message(uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    switch (id) {
    case DB_EV_TERMINATE:
        sni_flag_unset(SNI_FLAG_LOADED);
        sni_context_menu_release();
        break;
    case DB_EV_CONFIGCHANGED:
        g_debug("Event: DB_EV_CONFIGCHANGED");
        sni_configchanged();

        if (sni_flag_get(SNI_FLAG_LOADED))
            update_playback_controls();
        break;
    }
    return 0;
}

static int
sni_connect() {
    gtkui_plugin = (ddb_gtkui_t *)deadbeef->plug_get_for_id(DDB_GTKUI_PLUGIN_ID);
    if (!gtkui_plugin) {
        deadbeef->log_detailed((DB_plugin_t *)(&plugin), DDB_LOG_LAYER_DEFAULT,
                               "sni: can't find gtkui plugin\n");
        return -1;
    }

    DB_plugin_action_t *actions = gtkui_plugin->gui.plugin.get_actions(NULL);
    while (actions) {
        if (g_strcmp0(actions->name, "toggle_player_window") == 0) {
            toggle_mainwindow_action = actions;
        } else if (g_strcmp0(actions->name, "preferences") == 0) {
            preferences_action = actions;
        } else if (g_strcmp0(actions->name, "help") == 0) {
            help_action = actions;
        }
        actions = actions->next;
    }

    if (!toggle_mainwindow_action) {
        deadbeef->log_detailed((DB_plugin_t *)(&plugin), DDB_LOG_LAYER_DEFAULT,
                               "sni: failed to find \"toggle_player_window\" gtkui plugin\n");
    }

    sni_configchanged();

    return 0;
}

static int
sni_disconnect() {
    if (sni_flag_get(SNI_FLAG_ENABLED))
        sni_reload_icon(FALSE);
    return 0;
}

// clang-format off
static const char settings_dlg[] =
    "property \"Enable Status Notifier\" checkbox sni.enabled 1;\n"

    "property \"Display playback status on icon (if DE support overlay icons)\" checkbox sni.enable_overlay 1;\n"
    "property \"Display Status Notifier tooltip (if DE support this)\" checkbox sni.enable_tooltip 1;\n"
    "property \"Use plain text tooltip (if DE not support HTML tooltips)\" checkbox sni.tooltip_plain_text 0;\n"
    "property \"Set tooltip icon (if DE support this)\" checkbox sni.tooltip_enable_icon 1;\n"
    
    "property \"Enable playback options in menu (need restart)\" checkbox sni.menu_enable_playback 1;\n"

    "property \"Volume control ignore horizontal scroll\" checkbox sni.volume_hdirect_ignore 1;\n"
    "property \"Volume control use inverse scroll direction\" checkbox sni.volume_reverse 0;\n"
    
    "property \"Notifier registration waiting time (sec.)\" spinbtn[10,120,5] sni.waiting_load_sec 30;\n"
;
// clang-format on

static DB_misc_t plugin = {
    .plugin.type = DB_PLUGIN_MISC,
    .plugin.flags = DDB_PLUGIN_FLAG_LOGGING,
    .plugin.api_vmajor = 1,
    .plugin.api_vminor = 11,
    .plugin.version_major = 1,
    .plugin.version_minor = 5,
#if GTK_CHECK_VERSION(3, 0, 0)
    .plugin.id = "sni_gtk3",
    .plugin.name = "Status Notifier GTK3 UI",
#else
    .plugin.id = "sni_gtk2",
    .plugin.name = "Status Notifier GTK2 UI",
#endif
    .plugin.descr = "StatusNotifierItem for DE without support for xembedded icons\n"
                    "(like plasma5 or GNOME3). It also can be used for a better look&feel "
                    "experience.\n",
    .plugin.copyright = "StatusNotifier plugin for DeaDBeeF Player\n"
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
                        "along with this program.  If not, see "
                        "<http://www.gnu.org/licenses/>.\n",
    .plugin.website = "https://github.com/vovochka404/deadbeef-statusnotifier-plugin",
    .plugin.configdialog = settings_dlg,
    .plugin.message = sni_message,
    .plugin.connect = sni_connect,
    .plugin.disconnect = sni_disconnect,
};

SNI_EXPORT_FUNC DB_plugin_t *
#if GTK_CHECK_VERSION(3, 0, 0)
sni_gtk3_load(DB_functions_t *api) {
#else
sni_gtk2_load(DB_functions_t *api) {
#endif
    deadbeef = api;
    return DB_PLUGIN(&plugin);
}
