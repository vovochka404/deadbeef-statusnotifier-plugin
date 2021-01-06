#include "sni.h"

#define TOOLTIP_FORMAT "<b>[%s]</b><br/>"\
        "<b>Title:</b> %s<br/>"\
        "<b>Artist:</b> %s<br/>"\
        "<b>Album:</b> %s%s%s%s"
#define TOOLTIP_FORMAT_PLAIN "[%s]"\
        "\nTitle: %s"\
        "\nArtist: %s"\
        "\nAlbum: %s%s%s%s"

#define TOOLTIP_MAX_LENGTH 1000

#define GME_TEXT(X) \
    (X) ? g_markup_escape_text ((X), -1) : NULL

static inline const char*
get_track_info (DB_playItem_t* track,
                const char* info) {
    return deadbeef->pl_find_meta (track, info);
}

static inline const char*
get_track_artist (DB_playItem_t* track) {
    const char *artist = deadbeef->pl_find_meta (track, "artist");
    if (artist == NULL) {
        artist = deadbeef->pl_find_meta (track, "albumartist");
        if (artist == NULL) {
            artist = deadbeef->pl_find_meta (track, "album artist");
            if (artist == NULL)
                artist = deadbeef->pl_find_meta (track, "band");
        }
    }
    return artist;
}

static void
sni_get_tooltip (DB_playItem_t *track,
                 int state,
                 const gchar *fmt,
                 gchar *buf,
                 size_t sz)
{
    const gchar *ns = _("not specified");

    gchar *escaped_artist = GME_TEXT(get_track_artist(track));
    gchar *escaped_title  = GME_TEXT(get_track_info(track, "title"));
    gchar *escaped_album  = GME_TEXT(get_track_info(track, "album"));
    gchar *escaped_date   = GME_TEXT(get_track_info(track, "year"));

    g_snprintf (buf, sz, fmt,
                (state == OUTPUT_STATE_PAUSED) ? _("Playback paused"):
                                                 _("Playback played"),
                escaped_title   ? escaped_title  : ns,
                escaped_artist  ? escaped_artist : ns,
                escaped_album   ? escaped_album  : ns,
                escaped_date    ? " ["           : "",
                escaped_date    ? escaped_date   : "",
                escaped_date    ? "]"            : "");

    g_free (escaped_title);
    g_free (escaped_artist);
    g_free (escaped_album);
    g_free (escaped_date);
}

static inline GdkPixbuf*
sni_get_coverart(DB_playItem_t* track) {
    const char *artist = get_track_artist(track);
    const char *album  = get_track_info(track, "album");
#if (DDB_GTKUI_API_LEVEL >= 202)
    GdkPixbuf * buf = gtkui_plugin->get_cover_art_primary (deadbeef->pl_find_meta (track, ":URI"), artist, album, 128, NULL, NULL);
#else
    GdkPixbuf * buf = gtkui_plugin->get_cover_art_pixbuf  (deadbeef->pl_find_meta (track, ":URI"), artist, album, 128, NULL, NULL);
#endif
    if (buf == NULL)
        buf = gtkui_plugin->cover_get_default_pixbuf ();

    return buf;
}

static inline void
sni_set_tooltip_textonly (DB_playItem_t *track,
                          int state) {
    gchar title_body[TOOLTIP_MAX_LENGTH];
    sni_get_tooltip (track, state, _(TOOLTIP_FORMAT_PLAIN), title_body, TOOLTIP_MAX_LENGTH);
    status_notifier_set_tooltip_body (icon, title_body);
}

static inline void
sni_set_tooltip_html (DB_playItem_t *track,
                      int state) {
    gchar title_body[TOOLTIP_MAX_LENGTH];

    sni_get_tooltip (track, state, _(TOOLTIP_FORMAT), title_body, TOOLTIP_MAX_LENGTH);
    GdkPixbuf *pic = sni_get_coverart(track);
    (pic) ? status_notifier_set_from_pixbuf (icon, STATUS_NOTIFIER_TOOLTIP_ICON, pic) :
            status_notifier_set_from_icon_name (icon, STATUS_NOTIFIER_TOOLTIP_ICON, "deadbeef");

    status_notifier_set_tooltip_body (icon, title_body);
}

/* === main update functions === */

static void
sni_update_tooltip (int state) {
    if (!icon)
        return;
    if (deadbeef->conf_get_int("sni.enable_tooltip", 0) == 0)
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
                if (!track) {
                    status_notifier_set_tooltip (icon, "deadbeef", "DeaDBeeF", _("Playing"));
                    break;
                }
                deadbeef->conf_get_int("sni.tooltip_plain_text", 0) ?
                    sni_set_tooltip_textonly (track, state):
                    sni_set_tooltip_html (track, state);


                deadbeef->pl_item_unref (track);
                break;
            }
        }
    }
    else {
        status_notifier_set_tooltip (icon, "deadbeef", "DeaDBeeF", NULL);
    }
}

static void
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
        // Temporary hotfix - use sleep(1) in function callback_wait_notifier_register()

        int enable_overlay = deadbeef->conf_get_int("sni.animated",1);

        switch (state) {
            case DDB_PLAYBACK_STATE_PLAYING:
                if (enable_overlay)
                    status_notifier_set_from_icon_name (icon, STATUS_NOTIFIER_OVERLAY_ICON, "media-playback-start");
                stop_item = get_context_menu_item (SNI_MENU_ITEM_STOP);
                dbusmenu_menuitem_property_set_bool (stop_item, DBUSMENU_MENUITEM_PROP_ENABLED, TRUE);

                sni_toggle_play_pause (0);
                break;
            case DDB_PLAYBACK_STATE_PAUSED:
                if (enable_overlay)
                    status_notifier_set_from_icon_name (icon, STATUS_NOTIFIER_OVERLAY_ICON, "media-playback-pause");
                sni_toggle_play_pause (1);
                break;
            case DDB_PLAYBACK_STATE_STOPPED:
                if (enable_overlay)
                    status_notifier_set_from_icon_name (icon, STATUS_NOTIFIER_OVERLAY_ICON, NULL);
                stop_item = get_context_menu_item (SNI_MENU_ITEM_STOP);
                dbusmenu_menuitem_property_set_bool (stop_item, DBUSMENU_MENUITEM_PROP_ENABLED, FALSE);

                sni_toggle_play_pause (1);
                break;
        }
    }
    sni_update_tooltip (state);
}
