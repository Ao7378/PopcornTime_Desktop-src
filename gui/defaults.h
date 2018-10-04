#ifndef __DEFAULTS_H_INCL__
    #define __DEFAULTS_H_INCL__

    #if defined USE_QCEF || defined USE_WEBENGINE
const int MINIMUM_WIDTH = 960;
const int MINIMUM_HEIGHT = 520;
const int DEFAULT_WIDTH = 1100; // 1024;
const int DEFAULT_HEIGHT = 618; // 768;
const int DRAG_ZONE_LEFT_MARGIN = 135;
const int DRAG_ZONE1_HEIGHT = 32;
const int DRAG_ZONE2_HEIGHT = 28;
const int DRAG_ZONE1_RIGHT_MARGIN = 55;
const int DRAG_ZONE2_RIGHT_MARGIN = 215;
    #else
const int MINIMUM_WIDTH = 962;
const int MINIMUM_HEIGHT = 600;
const int DEFAULT_WIDTH = MINIMUM_WIDTH; // 1024;
const int DEFAULT_HEIGHT = MINIMUM_HEIGHT; // 768;
const int DRAG_ZONE_RIGHT_MARGIN = 60;
const int DRAG_ZONE_HEIGHT = 30;
    #endif
const int RESIZE_ZONE = 2;


const int CONTROL_HIDE_INTERVAL_MOUSE_AWAY_MS = 5 * 1000;

const char APP_NAME[] = "Popcorn Time";
const char COMPANY_NAME_MAC[] = "time4popcorn.eu";

const char STARTUP_URL[] = "http://ui.popcorn-time.to/?version=5.5.1.2&os=win";

const char SETTINGS_KEY_DOWNLOAD_DIR[] = "DownloadDir";
const char SETTINGS_KEY_CLEAN_ON_EXIT[] = "CleanOnExit";
const char SETTINGS_KEY_FONT_SIZE[] = "FontSize";
const char SETTINGS_KEY_VOLUME[] = "Volume";
const char SETTINGS_DEFAULT_FONT_SIZE = 0;
const char SETTINGS_KEY_CONFIG[] = "info";
const bool CLEAN_ON_EXIT_DEFAULT = true;

const char SUBTITLES_OFF[] = "<Subtitles off>";
const char SUBTITLES_CUSTOM[] = "<Custom subtitles>";

const int PROXY_CHECK_TIMEOUT_MS = 5 * 1000;
const int TORRENT_FILE_DOWNLOAD_TIMEOUT_MS = 15 * 1000;
const int TORRENT_INFO_UPDATE_DELAY_MS = 1000;

const int TORRENTIO_BUFFERING_CHECK_INTERVAL_MS = 1000;
const int TORRENTIO_FIRST_BUFFERING_TIMEOUT_S = 60;

const int PAUSE_IF_BUFFER_HAVE_LESS_THAN_MS = 10 * 1000;
const int DONT_PAUSE_IF_JUST_STARTED_TIME_MS = 30 * 1000;
const char TK_GETTING_METADATA[] = "Getting metadata...";
const char TK_ERROR_GETTING_METADATA[] = "Error getting metadata!";
const char TK_LOOKING_FOR_PEERS[] = "Looking for peers";
const char TK_DOWNLOADING[] = "Downloading";
const char TK_NET_BUFFERING[] = "Network Buffering";

#endif // __DEFAULTS_H_INCL__

