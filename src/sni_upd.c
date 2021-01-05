#include "sni.h"

#define TOOLTIP_FORMAT_WO_YEAR "%s"\
        "<b>Title:</b> %s<br/>"\
        "<b>Artist:</b> %s<br/>"\
        "<b>Album:</b> %s"
#define TOOLTIP_FORMAT "%s"\
        "<b>Title:</b> %s<br/>"\
        "<b>Artist:</b> %s<br/>"\
        "<b>Album:</b> %s [%s]"

#define TOOLTIP_FORMAT_PLAIN_WO_YEAR "[%s]"\
        "\nTitle: %s"\
        "\nArtist: %s"\
        "\nAlbum: %s"
#define TOOLTIP_FORMAT_PLAIN "[%s]"\
        "\nTitle: %s"\
        "\nArtist: %s"\
        "\nAlbum: %s [%s]"

#define TOOLTIP_MAX_LENGTH 1000

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
                int tt_plain_text = deadbeef->conf_get_int("sni.tooltip_plain_text", 0);

                DB_playItem_t *track = deadbeef->streamer_get_playing_track ();
                static gchar *ns = NULL;
                if (!ns)
                    ns = _("not specified");

                // HTML KDE-style
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
                if (tt_plain_text) {
                    date ?
                        g_snprintf (title_body, TOOLTIP_MAX_LENGTH, _(TOOLTIP_FORMAT_PLAIN), state == OUTPUT_STATE_PAUSED ? _("Playback paused") : _("Playback played"),
                                   escaped_title  ? escaped_title  : ns,
                                   escaped_artist ? escaped_artist : ns,
                                   escaped_album  ? escaped_album  : ns,
                                   escaped_date) :
                        g_snprintf (title_body, TOOLTIP_MAX_LENGTH, _(TOOLTIP_FORMAT_PLAIN_WO_YEAR), state == OUTPUT_STATE_PAUSED ? _("Playback paused") : _("Playback played"),
                                   escaped_title  ? escaped_title  : ns,
                                   escaped_artist ? escaped_artist : ns,
                                   escaped_album  ? escaped_album  : ns);
                } else {
                    date ?
                        g_snprintf (title_body, TOOLTIP_MAX_LENGTH, _(TOOLTIP_FORMAT), state == OUTPUT_STATE_PAUSED ? _("Playback paused") : _("Playback played"),
                                   escaped_title  ? escaped_title  : ns,
                                   escaped_artist ? escaped_artist : ns,
                                   escaped_album  ? escaped_album  : ns,
                                   escaped_date) :
                        g_snprintf (title_body, TOOLTIP_MAX_LENGTH, _(TOOLTIP_FORMAT_WO_YEAR), state == OUTPUT_STATE_PAUSED ? _("Playback paused") :  _("Playback played"),
                                   escaped_title  ? escaped_title  : ns,
                                   escaped_artist ? escaped_artist : ns,
                                   escaped_album  ? escaped_album  : ns);
                }

                g_free (escaped_title);
                g_free (escaped_artist);
                g_free (escaped_album);
                g_free (escaped_date);
                
                if (tt_plain_text == 0) {
                    g_debug("Going to query coverart");
#if (DDB_GTKUI_API_LEVEL >= 202)
                    GdkPixbuf * buf = gtkui_plugin->get_cover_art_primary (deadbeef->pl_find_meta (track, ":URI"), artist, album, 128, NULL, NULL);
#else
                    GdkPixbuf * buf = gtkui_plugin->get_cover_art_pixbuf  (deadbeef->pl_find_meta (track, ":URI"), artist, album, 128, NULL, NULL);
#endif
                    g_debug("Got GdbPixbuf: %zu", (uintptr_t) buf);
                    if (!buf) {
                        buf = gtkui_plugin->cover_get_default_pixbuf ();

                        if (buf)
                            status_notifier_set_from_pixbuf (icon, STATUS_NOTIFIER_TOOLTIP_ICON, buf);
                        else
                            status_notifier_set_from_icon_name (icon, STATUS_NOTIFIER_TOOLTIP_ICON, "deadbeef");
                    }
                    else
                        status_notifier_set_from_pixbuf (icon, STATUS_NOTIFIER_TOOLTIP_ICON, buf);
                }
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


