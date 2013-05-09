#include <glib.h>
#include <gio/gio.h>
#include <getopt.h>
#include <stdio.h>
#include <string.h>

typedef struct {
	GError *error;
	GDBusConnection *bus;
} DBus;

static int parse_options(int argc, char *argv[], DBus *dbus);
static int print_nowplaying(DBus *dbus);
static int send_dbus_message(DBus *dbus, char *msg, char *arg, char *type);
static void usage();

int parse_options(int argc, char *argv[], DBus *dbus) {
	int opt, option_index = 0;
	int retval = 3;
	char *url;

	static const struct option opts[] = {
		{"help",       no_argument,        0, 'h'},
		{"play",       no_argument,        0, 'p'},
		{"stop",       no_argument,        0, 's'},
		{"next",       no_argument,        0, 'n'},
		{"previous",   no_argument,        0, 'r'},
		{"nowplaying", no_argument,        0, 'i'},
		{"openurl",  required_argument,    0, 'o'},
		{0, 0, 0, 0}
	};

	while((opt = getopt_long(argc, argv, "o:hpsnri", opts, &option_index)) != -1) {
		switch(opt) {
			case 'h':
				usage();
				retval = 0;
				break;

			case 'p':
				return send_dbus_message(dbus, "PlayPause", NULL, NULL);

			case 's':
				return send_dbus_message(dbus, "Stop", NULL, NULL);

			case 'n':
				return send_dbus_message(dbus, "Next", NULL, NULL);

			case 'r':
				return send_dbus_message(dbus, "Previous", NULL, NULL);

			case 'i':
				return print_nowplaying(dbus);

			case 'o':
				return send_dbus_message(dbus, "OpenUri", argv[argc - 1], "(s)");

			default:
				retval = 2;
		}
	}
	return retval;
}

int print_nowplaying(DBus * dbus) {
	GVariant *result, *props;
	gchar **artists = NULL, *artist = NULL, *title = NULL;

	result = g_dbus_connection_call_sync(dbus->bus, "org.mpris.MediaPlayer2.spotify", "/org/mpris/MediaPlayer2", "org.freedesktop.DBus.Properties",
			"Get", g_variant_new("(ss)", "org.mpris.MediaPlayer2.Player", "Metadata"), G_VARIANT_TYPE("(v)"), G_DBUS_CALL_FLAGS_NONE, -1, NULL, &dbus->error);

	if (!result) {
		g_warning("Failed to call Get: %s\n", dbus->error->message);
		g_error_free(dbus->error);
		return 1;
	}

	g_variant_get(result, "(v)", &props);
	g_variant_lookup(props, "xesam:artist", "^as", &artists);
	g_variant_lookup(props, "xesam:title", "s", &title);

	if (artists)
		artist = g_strjoinv(", ", artists);
	else
		artist = "(Unknown Artist)";

	if (!title)
		title = "(Unknown Song)";

	g_printf("%s – %s\n", artist, title);

	return 0;
}

int send_dbus_message(DBus *dbus, char *msg, char *arg, char *type)
{
	int retval = 0;
	GVariant *result, *props;
	if(arg == NULL) {
		result = g_dbus_connection_call_sync(dbus->bus, "org.mpris.MediaPlayer2.spotify", "/org/mpris/MediaPlayer2", "org.mpris.MediaPlayer2.Player",
				msg, NULL, NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, &dbus->error);
	}
	else {
		result = g_dbus_connection_call_sync(dbus->bus, "org.mpris.MediaPlayer2.spotify", "/org/mpris/MediaPlayer2", "org.mpris.MediaPlayer2.Player",
				msg, g_variant_new(type, arg), NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, &dbus->error);
	}

	if (!result) {
		g_warning("Failed to call Get: %s\n", dbus->error->message);
		g_error_free(dbus->error);
		retval = 1;
	}

	return retval;
}

void usage() {
	fprintf(stderr, "spotifycli %s\n"
			"Usage: spotifycli [options]\n\n");
	fprintf(stderr,
			" Options:\n "
			"  -h, --help           display this help and exit\n "
			"  -p, --play           starts playing current selected song or resuming a paused one\n "
			"  -s, --stop           stops playing current song\n "
			"  -n, --next           changes to the next song in play queue\n "
			"  -r, --previous       changes to the previous played song in queue\n "
			"  -i, --nowplaying     prints current song that is paused or playing \n"
			"  -o, --openurl <url>  opens spotify uri/http url and starts playing it\n");
}

int main(int argc, char *argv[]) {
	DBus dbus;
	dbus.error = NULL;

	dbus.bus = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &dbus.error);
	if (!dbus.bus) {
		g_warning("Failed to connect to session bus: %s", dbus.error->message);
		g_error_free(dbus.error);
		return 1;
	}

	int retval = parse_options(argc, argv, &dbus);

	if(retval == 3)
		fprintf(stderr, "error: no operation specified (use -h for help)\n");
	else if(retval == 1)
		fprintf(stderr, "error: something went horrible wrong\n");

	g_object_unref(dbus.bus);
	return retval;
}
