/*
 * deadbeef-statusnotifier-plugin - Copyright (C) 2015 Vladimir Perepechin
 *
 * sni_upd.c
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

#define TOOLTIP_FORMAT                                                                             \
    "<b>[%s]</b><br/>"                                                                             \
    "<b>%s:</b> %s<br/>"                                                                           \
    "<b>%s:</b> %s<br/>"                                                                           \
    "<b>%s:</b> %s%s%s%s"
#define TOOLTIP_FORMAT_PLAIN                                                                       \
    "[%s]"                                                                                         \
    "\n%s: %s"                                                                                     \
    "\n%s: %s"                                                                                     \
    "\n%s: %s%s%s%s"

#define STATE_WAITING_CYCLE 10000
#define TOOLTIP_MAX_LENGTH 1000

#define GME_TEXT(X) (X) ? g_markup_escape_text((X), -1) : NULL

/* This function is necessary because at the time of the call,
   the output may be uninitialized and a data race will occur */

static inline int
playback_state_active_waiting(void) {
    DB_output_t *out = deadbeef->get_output();
    if (out) {
        ddb_playback_state_t state = 0;
        for (;;) {
            state = out->state();
            if ((state >= 0) && (state <= DDB_PLAYBACK_STATE_PAUSED))
                return state;
        }
    }
    return -1;
}

static inline const char *
get_track_info(DB_playItem_t *track, const char *info) {
    return deadbeef->pl_find_meta(track, info);
}

static inline const char *
get_track_artist(DB_playItem_t *track) {
    const char *artist = NULL;
    if ((artist = get_track_info(track, "artist")) == NULL)
        if ((artist = get_track_info(track, "albumartist")) == NULL)
            if ((artist = get_track_info(track, "album artist")) == NULL)
                artist = get_track_info(track, "band");
    return artist;
}

static void
sni_get_tooltip(DB_playItem_t *track, int state, const gchar *fmt, gchar *buf, size_t sz) {
    const gchar *ns = _("not specified");

    gchar *escaped_artist = GME_TEXT(get_track_artist(track));
    gchar *escaped_title = GME_TEXT(get_track_info(track, "title"));
    gchar *escaped_album = GME_TEXT(get_track_info(track, "album"));
    gchar *escaped_date = GME_TEXT(get_track_info(track, "year"));

    // clang-format off
    g_snprintf(buf, sz, fmt,
               (state == OUTPUT_STATE_PAUSED) ? _("Paused")
                                              : _("Playing"),
               _("Title"),  escaped_title  ? escaped_title  : ns,
               _("Artist"), escaped_artist ? escaped_artist : ns,
               _("Album"),  escaped_album  ? escaped_album  : ns,
                            escaped_date   ? " ["           : "",
                            escaped_date   ? escaped_date   : "",
                            escaped_date   ? "]"            : "");
    // clang-format on

    g_free(escaped_title);
    g_free(escaped_artist);
    g_free(escaped_album);
    g_free(escaped_date);
}

static inline GdkPixbuf *
sni_get_coverart(DB_playItem_t *track) {
    const char *uri = get_track_info(track, ":URI");
    const char *artist = get_track_artist(track);
    const char *album = get_track_info(track, "album");
    if (!album || !*album) {
        album = get_track_info(track, "title");
    }
#if (DDB_GTKUI_API_LEVEL >= 202)
    GdkPixbuf *buf = gtkui_plugin->get_cover_art_primary(uri, artist, album, 128, NULL, NULL);
#else
    GdkPixbuf *buf = gtkui_plugin->get_cover_art_pixbuf(uri, artist, album, 128, NULL, NULL);
#endif
    if (buf == NULL)
        buf = gtkui_plugin->cover_get_default_pixbuf();

    return buf;
}

static inline void
sni_set_tooltip_textonly(DB_playItem_t *track, int state) {
    gchar title_body[TOOLTIP_MAX_LENGTH];
    sni_get_tooltip(track, state, _(TOOLTIP_FORMAT_PLAIN), title_body, TOOLTIP_MAX_LENGTH);
    status_notifier_set_tooltip_body(icon, title_body);
}

static inline void
sni_set_tooltip_html(DB_playItem_t *track, int state) {
    gchar title_body[TOOLTIP_MAX_LENGTH];

    sni_get_tooltip(track, state, _(TOOLTIP_FORMAT), title_body, TOOLTIP_MAX_LENGTH);
    if (deadbeef->conf_get_int("sni.tooltip_enable_icon", 1)) {
        GdkPixbuf *pic = sni_get_coverart(track);
        if (pic) {
            status_notifier_set_from_pixbuf(icon, STATUS_NOTIFIER_TOOLTIP_ICON, pic);
            g_object_unref(pic);
        } else {
            status_notifier_set_from_icon_name(icon, STATUS_NOTIFIER_TOOLTIP_ICON, "deadbeef");
        }
    }
    status_notifier_set_tooltip_body(icon, title_body);
}

/* === main update functions === */

static void
sni_update_tooltip(int state) {

    if (sni_flag_get(SNI_FLAG_LOADED) == FALSE)
        return;
    if (!icon)
        return;
    if (deadbeef->conf_get_int("sni.enable_tooltip", 0) == 0)
        return;

    g_debug("sni_update_tooltip, status: %d", state);

    int out_state = (state < 0) ? playback_state_active_waiting() : state;
    if (out_state >= 0) {
        switch (out_state) {
        case DDB_PLAYBACK_STATE_STOPPED:
            status_notifier_set_tooltip(icon, "deadbeef", "DeaDBeeF", _("Playback stopped"));
            break;
        case DDB_PLAYBACK_STATE_PAUSED:
        case DDB_PLAYBACK_STATE_PLAYING: {
            DB_playItem_t *track = deadbeef->streamer_get_playing_track();
            if (track) {
                deadbeef->conf_get_int("sni.tooltip_plain_text", 0)
                    ? sni_set_tooltip_textonly(track, out_state)
                    : sni_set_tooltip_html(track, out_state);

                deadbeef->pl_item_unref(track);
            } else {
                status_notifier_set_tooltip(icon, "deadbeef", "DeaDBeeF", _("Playing"));
            }
            break;
        }
        }
    } else {
        status_notifier_set_tooltip(icon, "deadbeef", "DeaDBeeF", NULL);
    }
}

static void
sni_update_status(int state) {

    if (sni_flag_get(SNI_FLAG_LOADED) == FALSE)
        return;
    if (!icon)
        return;

    g_debug("sni_update_status, status: %d", state);
    DbusmenuMenuitem *stop_item;

    int out_state = (state < 0) ? playback_state_active_waiting() : state;
    if (out_state >= 0) {

        int enable_overlay = deadbeef->conf_get_int("sni.enable_overlay", 1);
        switch (out_state) {
        case DDB_PLAYBACK_STATE_PLAYING:
            if (enable_overlay)
                status_notifier_set_from_icon_name(icon, STATUS_NOTIFIER_OVERLAY_ICON,
                                                   "media-playback-start");
            stop_item = get_context_menu_item(SNI_MENU_ITEM_STOP);
            dbusmenu_menuitem_property_set_bool(stop_item, DBUSMENU_MENUITEM_PROP_ENABLED, TRUE);

            sni_toggle_play_pause(SNI_STATE_TOOGLE_PLAY);
            break;
        case DDB_PLAYBACK_STATE_PAUSED:
            if (enable_overlay)
                status_notifier_set_from_icon_name(icon, STATUS_NOTIFIER_OVERLAY_ICON,
                                                   "media-playback-pause");
            sni_toggle_play_pause(SNI_STATE_TOOGLE_PAUSE);
            break;
        case DDB_PLAYBACK_STATE_STOPPED:
            if (enable_overlay)
                status_notifier_set_from_icon_name(icon, STATUS_NOTIFIER_OVERLAY_ICON, NULL);
            stop_item = get_context_menu_item(SNI_MENU_ITEM_STOP);
            dbusmenu_menuitem_property_set_bool(stop_item, DBUSMENU_MENUITEM_PROP_ENABLED, FALSE);

            sni_toggle_play_pause(SNI_STATE_TOOGLE_PAUSE);
            break;
        }
    }
    sni_update_tooltip(out_state);
}
