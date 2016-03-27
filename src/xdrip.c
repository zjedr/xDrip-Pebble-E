#include "pebble.h"

#define TRENDING 0

/* The line below will set the debug message level.  
Make sure you set this to 0 before building a release. */

#define DEBUG_LEVEL 1
#define ICON_MSGSTR_SIZE 4
#define BG_MSGSTR_SIZE 6
#define BGDELTA_MSGSTR_SIZE 13
#define BATTLEVEL_MSGSTR_SIZE 5

// global window variables
// display settings
#ifdef PBL_COLOR
  // for mmol users suffix with .0 on whole numbers ie. "4.0"
  static char high_bg[6] = "30.0";
  static char low_bg[6] = "1.0";
#endif
#ifdef PBL_ROUND 
  int displayFormat = 2;
#else
  // set to 1,2, or 3 
  int displayFormat = 1;
#endif
bool battery_graphic = true ;

// ANYTHING THAT IS CALLED BY PEBBLE API HAS TO BE NOT STATIC

Window *window_cgm = NULL;

TextLayer *bg_layer = NULL;
TextLayer *cgmtime_layer = NULL;
TextLayer *delta_layer = NULL;		// BG DELTA LAYER
TextLayer *message_layer = NULL;	// MESSAGE LAYER
TextLayer *battlevel_layer = NULL;
TextLayer *watch_battlevel_layer = NULL;
TextLayer *time_watch_layer = NULL;
//TextLayer *time_app_layer = NULL;
TextLayer *date_app_layer = NULL;
TextLayer *upper_layer = NULL ;
TextLayer *lower_layer = NULL ;
TextLayer *watch_battery_layer = NULL;
TextLayer *watch_battery_text_layer = NULL;
Layer *watch_battery_draw_layer = NULL;
TextLayer *bridge_battery_layer = NULL;
TextLayer *bridge_battery_text_layer = NULL;
Layer *bridge_battery_draw_layer = NULL;

BitmapLayer *icon_layer = NULL;
//BitmapLayer *appicon_layer = NULL;
BitmapLayer *bg_trend_layer = NULL;
BitmapLayer *upper_face_layer = NULL;
BitmapLayer *lower_face_layer = NULL;

static GFont bgFont, midFont, smallFont, timeFont, messageFont  ;

GBitmap *icon_bitmap = NULL;
GBitmap *appicon_bitmap = NULL;
GBitmap *specialvalue_bitmap = NULL;
GBitmap *bg_trend_bitmap = NULL;
GBitmap *battery_bitmap = NULL;

static char time_watch_text[] = "00:00";
static char date_app_text[] = "Wed 13 Jan";

// variables for AppSync
AppSync sync_cgm;
#define CHUNK_SIZE 1024
// CGM message is 57 bytes
// Pebble needs additional 62 Bytes?!? Pad with additional 20 bytes
//#define SYNC_BUFFER_SIZE 1024
//static uint8_t *sync_buffer_cgm;
//static uint8_t sync_buffer_cgm[CHUNK_SIZE];
//static uint8_t trend_buffer[4096];
bool doing_trend = false;
uint8_t trend_chunk[CHUNK_SIZE];
#ifdef TRENDING
uint8_t *trend_buffer = NULL;
static uint16_t trend_buffer_length = 0;
static uint16_t expected_trend_buffer_length = 0;
#endif
bool display_message = false;
bool bridgeCharging = false;
// variables for timers and time
AppTimer *timer_cgm = NULL;
AppTimer *BT_timer = NULL;
time_t time_now = 0;

// variable for component position
GPoint centerPoint ;
int third_w ; 
GRect grectWindow ;
GRect grectUpperLayer ;
GRect grectLowerLayer ;
GRect grectImageBounds  ;
GRect grectIconLayer ;
GRect grectMessageLayer ;
GRect grectDeltaLayer ;
GRect grectTimeAgoLayer ;
GRect grectBGLayer ;
GRect grectTimeLayer ;
GRect grectDateLayer ;
GRect grectWatchBatteryLayer ;
GRect grectBridgeBatteryLayer ;
GRect grectTrendLayer;
GColor8 dateColor, textColor, bgColor, upperColor, timeColor ;
GTextAlignment alignBG = GTextAlignmentCenter;
GTextAlignment alignTime = GTextAlignmentCenter;
GTextAlignment alignDate = GTextAlignmentCenter;
GTextAlignment alignDelta = GTextAlignmentCenter;
GTextAlignment alignTimeAgo = GTextAlignmentCenter;
GAlign alignIcon = GAlignTop;

		
// global variable for bluetooth connection
bool bluetooth_connected_cgm = true;

// BATTERY LEVEL FORMATTED SIZE used for Bridge/Phone and Watch battery indications
const uint8_t BATTLEVEL_FORMATTED_SIZE = 8;

// watch battery colors
static GColor8 watch_battery_fill_color, watch_battery_low_color, watch_battery_border_color;
static GColor8 bridge_battery_fill_color, bridge_battery_low_color, bridge_battery_border_color;

static int battery_pcnt = 0;
static bool battery_changing = false;

// global variables for sync tuple functions
// buffers have to be static and hardcoded
static char current_icon[2];
static char last_bg[6];
//static int current_bg = 0;
static bool currentBG_isMMOL = false;
static char last_battlevel[4];
static uint32_t current_cgm_time = 0;
static uint32_t current_app_time = 0;
static char current_bg_delta[14];

//static int converted_bgDelta = 0;

// global BG snooze timer
static uint8_t lastAlertTime = 0;

// global special value alert
static bool specvalue_alert = false;

// global overwrite variables for vibrating when hit a specific BG if already in a snooze
//static bool specvalue_overwrite = false;

// global variables for vibrating in special conditions
static bool DoubleDownAlert = false;
static bool AppSyncErrAlert = false;
static bool AppMsgInDropAlert = false;
static bool AppMsgOutFailAlert = false;
static bool BluetoothAlert = false;
static bool BT_timer_pop = false;
//static bool CGMOffAlert = false;
static bool PhoneOffAlert = false;
//static bool LowBatteryAlert = false;

// global constants for time durations
static const uint8_t MINUTEAGO = 60;
static const uint16_t HOURAGO = 60*(60);
static const uint32_t DAYAGO = 24*(60*60);
static const uint32_t WEEKAGO = 7*(24*60*60);
static const uint16_t MS_IN_A_SECOND = 1000;

// Constants for string buffers
// If add month to date, buffer size needs to increase to 12; also need to reformat date_app_text init string
static const uint8_t TIME_TEXTBUFF_SIZE = 6;
static const uint8_t DATE_TEXTBUFF_SIZE = 11;
static const uint8_t LABEL_BUFFER_SIZE = 6;
static const uint8_t TIMEAGO_BUFFER_SIZE = 10;

// ** START OF CONSTANTS THAT CAN BE CHANGED; DO NOT CHANGE IF YOU DO NOT KNOW WHAT YOU ARE DOING **
// ** FOR MMOL, ALL VALUES ARE STORED AS INTEGER; LAST DIGIT IS USED AS DECIMAL **
// ** BE EXTRA CAREFUL OF CHANGING SPECIAL VALUES OR TIMERS; DO NOT CHANGE WITHOUT EXPERT HELP **

// FOR BG RANGES
// DO NOT SET ANY BG RANGES EQUAL TO ANOTHER; LOW CAN NOT EQUAL MIDLOW
// LOW BG RANGES MUST BE IN ASCENDING ORDER; SPECVALUE < HYPOLOW < BIGLOW < MIDLOW < LOW
// HIGH BG RANGES MUST BE IN ASCENDING ORDER; HIGH < MIDHIGH < BIGHIGH
// DO NOT ADJUST SPECVALUE UNLESS YOU HAVE A VERY GOOD REASON
// DO NOT USE NEGATIVE NUMBERS OR DECIMAL POINTS OR ANYTHING OTHER THAN A NUMBER

// BG Ranges, MG/DL
static const uint16_t SPECVALUE_BG_MGDL = 20;
static const uint16_t SHOWLOW_BG_MGDL = 40;
static const uint16_t SHOWHIGH_BG_MGDL = 400;

// BG Ranges, MMOL
// VALUES ARE IN INT, NOT FLOATING POINT, LAST DIGIT IS DECIMAL
// FOR EXAMPLE : SPECVALUE IS 1.1, BIGHIGH IS 16.6
// ALWAYS USE ONE AND ONLY ONE DECIMAL POINT FOR LAST DIGIT
// GOOD : 5.0, 12.2 // BAD : 7 , 14.44
static const uint16_t SPECVALUE_BG_MMOL = 11;
static const uint16_t SHOWLOW_BG_MMOL = 22;
static const uint16_t SHOWHIGH_BG_MMOL = 220;

// BG Snooze Times, in Minutes; controls when vibrate again
// RANGE 0-240
static const uint8_t SPECVALUE_SNZ_MIN = 30;
	
// Vibration Levels; 0 = NONE; 1 = LOW; 2 = MEDIUM; 3 = HIGH
// IF YOU DO NOT WANT A SPECIFIC VIBRATION, SET TO 0
static const uint8_t SPECVALUE_VIBE = 2;
static const uint8_t DOUBLEDOWN_VIBE = 3;
static const uint8_t APPSYNC_ERR_VIBE = 1;
static const uint8_t APPMSG_INDROP_VIBE = 1;
static const uint8_t APPMSG_OUTFAIL_VIBE = 1;
static const uint8_t BTOUT_VIBE = 1;
static const uint8_t CGMOUT_VIBE = 1;
static const uint8_t PHONEOUT_VIBE = 1;
static const uint8_t LOWBATTERY_VIBE = 1;

// Icon Cross Out & Vibrate Once Wait Times, in Minutes
// RANGE 0-240
// IF YOU WANT TO WAIT LONGER TO GET CONDITION, INCREASE NUMBER
static const uint8_t CGMOUT_WAIT_MIN = 15;
static const uint8_t PHONEOUT_WAIT_MIN = 8;

// Control Messages
// IF YOU DO NOT WANT A SPECIFIC MESSAGE, SET TO true
static const bool TurnOff_NOBLUETOOTH_Msg = false;
static const bool TurnOff_CHECKCGM_Msg = false;
static const bool TurnOff_CHECKPHONE_Msg = false;

// Control Vibrations
// IF YOU WANT NO VIBRATIONS, SET TO true
static const bool TurnOffAllVibrations = false;
// IF YOU WANT LESS INTENSE VIBRATIONS, SET TO true
static const bool TurnOffStrongVibrations = false;

// Bluetooth Timer Wait Time, in Seconds
// RANGE 0-240
// THIS IS ONLY FOR BAD BLUETOOTH CONNECTIONS
// TRY EXTENDING THIS TIME TO SEE IF IT WILL HELP SMOOTH CONNECTION
// CGM DATA RECEIVED EVERY 60 SECONDS, GOING BEYOND THAT MAY RESULT IN MISSED DATA
static const uint8_t BT_ALERT_WAIT_SECS = 10;

// ** END OF CONSTANTS THAT CAN BE CHANGED; DO NOT CHANGE IF YOU DO NOT KNOW WHAT YOU ARE DOING **

// Message Timer Wait Times, in Seconds
static const uint16_t WATCH_MSGSEND_SECS = 60;
static const uint8_t LOADING_MSGSEND_SECS = 2;
static uint8_t minutes_cgm = 0;


#define	CGM_ICON_KEY 	0		// TUPLE_CSTRING, MAX 2 BYTES (10)
#define	CGM_BG_KEY 	1		// TUPLE_CSTRING, MAX 4 BYTES (253 OR 22.2)
#define	CGM_TCGM_KEY	2		// TUPLE_INT, 4 BYTES (CGM TIME)
#define	CGM_TAPP_KEY 	3		// TUPLE_INT, 4 BYTES (APP / PHONE TIME)
#define	CGM_DLTA_KEY 	4		// TUPLE_CSTRING, MAX 5 BYTES (BG DELTA, -100 or -10.0)
#define	CGM_UBAT_KEY 	5		// TUPLE_CSTRING, MAX 3 BYTES (UPLOADER BATTERY, 100)
#define	CGM_NAME_KEY 	6		// TUPLE_CSTRING, MAX 9 BYTES (Christine)
#define	CGM_TREND_BEGIN_KEY 	7		// TUPLE_INT, 4 BYTES (length of CGM_TREND_DATA_KEY
#define	CGM_TREND_DATA_KEY 	8		// TUPLE_BYTE[], No Maximum, based on value found in CGM_TREND_DATA_KEY
#define	CGM_TREND_END_KEY 	9		// TUPLE_INT, always 0.
#define CGM_MESSAGE_KEY		10
#define CGM_VIBE_KEY		11
#define CGM_SYNC_KEY		0xd1abada5	// key pebble will use to request and update.
 
// TOTAL MESSAGE DATA 4x3+2+5+3+9 = 31 BYTES
// TOTAL KEY HEADER DATA (STRINGS) 4x6+2 = 26 BYTES
// TOTAL MESSAGE 57 BYTES

// ARRAY OF SPECIAL VALUE ICONS
static const uint8_t SPECIAL_VALUE_ICONS[] = {
	RESOURCE_ID_IMAGE_NONE,			//0
	RESOURCE_ID_IMAGE_BROKEN_ANTENNA,	//1
	RESOURCE_ID_IMAGE_BLOOD_DROP,		//2
	RESOURCE_ID_IMAGE_STOP_LIGHT,		//3
	RESOURCE_ID_IMAGE_HOURGLASS,		//4
	RESOURCE_ID_IMAGE_QUESTION_MARKS,	//5
	RESOURCE_ID_IMAGE_LOGO,			//6
	RESOURCE_ID_IMAGE_ERR		//7
 };
	
// INDEX FOR ARRAY OF SPECIAL VALUE ICONS
static const uint8_t NONE_SPECVALUE_ICON_INDX = 0;
static const uint8_t BROKEN_ANTENNA_ICON_INDX = 1;
static const uint8_t BLOOD_DROP_ICON_INDX = 2;
static const uint8_t STOP_LIGHT_ICON_INDX = 3;
static const uint8_t HOURGLASS_ICON_INDX = 4;
static const uint8_t QUESTION_MARKS_ICON_INDX = 5;
static const uint8_t LOGO_SPECVALUE_ICON_INDX = 6;
static const uint8_t ERR_SPECVALUE_ICON_INDX = 7;

/*
// ARRAY OF TIMEAGO ICONS
static const uint8_t TIMEAGO_ICONS[] = {
	RESOURCE_ID_IMAGE_NONE,			//0
	RESOURCE_ID_IMAGE_PHONEON,		//1
	RESOURCE_ID_IMAGE_PHONEOFF,	 	//2
};

// INDEX FOR ARRAY OF TIMEAGO ICONS
static const uint8_t NONE_TIMEAGO_ICON_INDX = 0;
static const uint8_t PHONEON_ICON_INDX = 1;
static const uint8_t PHONEOFF_ICON_INDX = 2;
*/
#ifdef DEBUG_LEVEL
static char *translate_app_error(AppMessageResult result) {
	switch (result) {
	case APP_MSG_OK: return "APP_MSG_OK";
		case APP_MSG_SEND_TIMEOUT: return "APP_MSG_SEND_TIMEOUT";
		case APP_MSG_SEND_REJECTED: return "APP_MSG_SEND_REJECTED";
		case APP_MSG_NOT_CONNECTED: return "APP_MSG_NOT_CONNECTED";
		case APP_MSG_APP_NOT_RUNNING: return "APP_MSG_APP_NOT_RUNNING";
		case APP_MSG_INVALID_ARGS: return "APP_MSG_INVALID_ARGS";
		case APP_MSG_BUSY: return "APP_MSG_BUSY";
		case APP_MSG_BUFFER_OVERFLOW: return "APP_MSG_BUFFER_OVERFLOW";
		case APP_MSG_ALREADY_RELEASED: return "APP_MSG_ALREADY_RELEASED";
		case APP_MSG_CALLBACK_ALREADY_REGISTERED: return "APP_MSG_CALLBACK_ALREADY_REGISTERED";
		case APP_MSG_CALLBACK_NOT_REGISTERED: return "APP_MSG_CALLBACK_NOT_REGISTERED";
		case APP_MSG_OUT_OF_MEMORY: return "APP_MSG_OUT_OF_MEMORY";
		case APP_MSG_CLOSED: return "APP_MSG_CLOSED";
		case APP_MSG_INTERNAL_ERROR: return "APP_MSG_INTERNAL_ERROR";
		default: return "APP UNKNOWN ERROR";
	}
}

static char *translate_dict_error(DictionaryResult result) {
	switch (result) {
		case DICT_OK: return "DICT_OK";
		case DICT_NOT_ENOUGH_STORAGE: return "DICT_NOT_ENOUGH_STORAGE";
		case DICT_INVALID_ARGS: return "DICT_INVALID_ARGS";
		case DICT_INTERNAL_INCONSISTENCY: return "DICT_INTERNAL_INCONSISTENCY";
		case DICT_MALLOC_FAILED: return "DICT_MALLOC_FAILED";
		default: return "DICT UNKNOWN ERROR";
	}
}
#endif

int myAtoi(char *str) {

	// VARIABLES
		int res = 0; // Initialize result
 
	// CODE START
	#if DEBUG_LEVEL > 1
	APP_LOG(APP_LOG_LEVEL_INFO, "MYATOI: ENTER CODE");
	#endif
	// Iterate through all characters of input string and update result
	for (int i = 0; str[i] != '\0'; ++i) {
		
		#if DEBUG_LEVEL > 2
		APP_LOG(APP_LOG_LEVEL_DEBUG, "MYATOI, STRING IN: %s", &str[i] );
		#endif
		
		if ( (str[i] >= ('0')) && (str[i] <= ('9')) ) {
			res = res*10 + str[i] - '0';
		}
		//APP_LOG(APP_LOG_LEVEL_DEBUG, "MYATOI, FOR RESULT OUT: %i", res );
	}
	#if DEBUG_LEVEL > 1
	APP_LOG(APP_LOG_LEVEL_DEBUG, "MYATOI, FINAL RESULT OUT: %i", res );
	#endif
	return res;
} // end myAtoi


int myBGAtoi(char *str) {

	// VARIABLES
	int res = 0; // Initialize result
	
	// CODE START
 
	// If we have the "???" special value, return 0
	if (strcmp(str, "???") == 0) return res;
	if (strcmp(str, "LOW") == 0) return 13;
	if (strcmp(str, "HIGH") == 0) return 410;
	
	// initialize currentBG_isMMOL flag
	currentBG_isMMOL = false;
	#ifdef DEBUG_LEVEL
	APP_LOG(APP_LOG_LEVEL_INFO, "myBGAtoi, START str is MMOL: %s", str );
	#endif
	// Iterate through all characters of input string and update result
	for (int i = 0; str[i] != '\0'; ++i) {
		#if DEBUG_LEVEL >2
		APP_LOG(APP_LOG_LEVEL_DEBUG, "myBGAtoi, STRING IN: %s", &str[i] );
		#endif
		if (str[i] == ('.')||str[i] == (',')) {
			currentBG_isMMOL = true;
		}
		else if ( (str[i] >= ('0')) && (str[i] <= ('9')) ) {
			res = res*10 + str[i] - '0';
		}
			
		//APP_LOG(APP_LOG_LEVEL_DEBUG, "myBGAtoi, FOR RESULT OUT: %i", res );
	}
	#ifdef DEBUG_LEVEL
	APP_LOG(APP_LOG_LEVEL_INFO, "myBGAtoi, currentBG is MMOL: %i", currentBG_isMMOL );		
 	APP_LOG(APP_LOG_LEVEL_INFO, "myBGAtoi, FINAL RESULT OUT: %i", res );
 	#endif
	return res;
} // end myBGAtoi

static void destroy_null_GBitmap(GBitmap **GBmp_image) {
	//APP_LOG(APP_LOG_LEVEL_INFO, "DESTROY NULL GBITMAP: ENTER CODE");
	
	if (*GBmp_image != NULL) {
		//APP_LOG(APP_LOG_LEVEL_INFO, "DESTROY NULL GBITMAP: POINTER EXISTS, DESTROY BITMAP IMAGE");
			gbitmap_destroy(*GBmp_image);
			if (*GBmp_image != NULL) {
				//APP_LOG(APP_LOG_LEVEL_INFO, "DESTROY NULL GBITMAP: POINTER EXISTS, SET POINTER TO NULL");
				*GBmp_image = NULL;
			}
	}
	
	 //APP_LOG(APP_LOG_LEVEL_INFO, "DESTROY NULL GBITMAP: EXIT CODE");
} // end destroy_null_GBitmap

static void destroy_null_BitmapLayer(BitmapLayer **bmp_layer) {
	//APP_LOG(APP_LOG_LEVEL_INFO, "DESTROY NULL BITMAP: ENTER CODE");
	
	if (*bmp_layer != NULL) {
		//APP_LOG(APP_LOG_LEVEL_INFO, "DESTROY NULL BITMAP: POINTER EXISTS, DESTROY BITMAP LAYER");
		bitmap_layer_destroy(*bmp_layer);
		if (*bmp_layer != NULL) {
			//APP_LOG(APP_LOG_LEVEL_INFO, "DESTROY NULL BITMAP: POINTER EXISTS, SET POINTER TO NULL");
			*bmp_layer = NULL;
		}
	}

	//APP_LOG(APP_LOG_LEVEL_INFO, "DESTROY NULL BITMAP: EXIT CODE");
} // end destroy_null_BitmapLayer

static void destroy_null_TextLayer(TextLayer **txt_layer) {
	//APP_LOG(APP_LOG_LEVEL_INFO, "DESTROY NULL TEXT LAYER: ENTER CODE");
	
	if (*txt_layer != NULL) {
		//APP_LOG(APP_LOG_LEVEL_INFO, "DESTROY NULL TEXT LAYER: POINTER EXISTS, DESTROY TEXT LAYER");
			text_layer_destroy(*txt_layer);
			if (*txt_layer != NULL) {
				//APP_LOG(APP_LOG_LEVEL_INFO, "DESTROY NULL TEXT LAYER: POINTER EXISTS, SET POINTER TO NULL");
				*txt_layer = NULL;
			}
	}
//APP_LOG(APP_LOG_LEVEL_INFO, "DESTROY NULL TEXT LAYER: EXIT CODE");
} // end destroy_null_TextLayer

static void create_update_bitmap(GBitmap **bmp_image, BitmapLayer *bmp_layer, const int resource_id) {
	//APP_LOG(APP_LOG_LEVEL_INFO, " CREATE UPDATE BITMAP: ENTER CODE");
	
	// if bitmap pointer exists, destroy and set to NULL
	destroy_null_GBitmap(bmp_image);
	
	// create bitmap and pointer
	//APP_LOG(APP_LOG_LEVEL_INFO, " CREATE UPDATE BITMAP: CREATE BITMAP");
	*bmp_image = gbitmap_create_with_resource(resource_id);
	
	if (*bmp_image == NULL) {
		// couldn't create bitmap, return so don't crash
		APP_LOG(APP_LOG_LEVEL_INFO, " CREATE UPDATE BITMAP: COULDNT CREATE BITMAP, RETURN");
		return;
	}
	else {
		// set bitmap
		//APP_LOG(APP_LOG_LEVEL_INFO, " CREATE UPDATE BITMAP: SET BITMAP");
		bitmap_layer_set_bitmap(bmp_layer, *bmp_image);
	}
	//APP_LOG(APP_LOG_LEVEL_INFO, " CREATE UPDATE BITMAP: EXIT CODE");
} // end create_update_bitmap


static void alert_handler_cgm(uint8_t alertValue) {
	//APP_LOG(APP_LOG_LEVEL_INFO, "ALERT HANDLER");
	#ifdef DEBUG_LEVEL
	APP_LOG(APP_LOG_LEVEL_DEBUG, "ALERT CODE: %d", alertValue);
	#endif
	// CONSTANTS
	// constants for vibrations patterns; has to be uint32_t, measured in ms, maximum duration 10000ms
	// Vibe pattern: ON, OFF, ON, OFF; ON for 500ms, OFF for 100ms, ON for 100ms; 
	
	// CURRENT PATTERNS
	const uint32_t highalert_fast[] = { 300,100,50,100,300,100,50,100,300,100,50,100,300,100,50,100,300,100,50,100,300,100,50,100,300,100,50,100,300,100,50,100,300 };
	const uint32_t medalert_long[] = { 500,100,100,100,500,100,100,100,500,100,100,100,500,100,100,100,500 };
	const uint32_t lowalert_beebuzz[] = { 75,50,50,50,75,50,50,50,75,50,50,50,75,50,50,50,75,50,50,50,75,50,50,50,75 };
	
	// PATTERN DURATION
	const uint8_t HIGHALERT_FAST_STRONG = 33;
	const uint8_t HIGHALERT_FAST_SHORT = (33/2);
	const uint8_t MEDALERT_LONG_STRONG = 17;
	const uint8_t MEDALERT_LONG_SHORT = (17/2);
	const uint8_t LOWALERT_BEEBUZZ_STRONG = 25;
	const uint8_t LOWALERT_BEEBUZZ_SHORT = (25/2);
	
	
	// CODE START
	
	if (TurnOffAllVibrations) {
			//turn off all vibrations is set, return out here
			return;
	}
	
	switch (alertValue) {

	case 0:
		//No alert
		//Normal (new data, in range, trend okay)
		break;
		
	case 1:;
		//Low
		#ifdef DEBUG_LEVEL
		APP_LOG(APP_LOG_LEVEL_INFO, "ALERT HANDLER: LOW ALERT");
		#endif
		VibePattern low_alert_pat = {
			.durations = lowalert_beebuzz,
			.num_segments = LOWALERT_BEEBUZZ_STRONG,
		};
		if (TurnOffStrongVibrations) { low_alert_pat.num_segments = LOWALERT_BEEBUZZ_SHORT; };
		vibes_enqueue_custom_pattern(low_alert_pat);
		break;

	case 2:;
		// Medium Alert
		#ifdef DEBUG_LEVEL
		APP_LOG(APP_LOG_LEVEL_INFO, "ALERT HANDLER: MEDIUM ALERT");
		#endif
		VibePattern med_alert_pat = {
			.durations = medalert_long,
			.num_segments = MEDALERT_LONG_STRONG,
		};
		if (TurnOffStrongVibrations) { med_alert_pat.num_segments = MEDALERT_LONG_SHORT; };
		vibes_enqueue_custom_pattern(med_alert_pat);
		break;

	case 3:;
		// High Alert
		#ifdef DEBUG_LEVEL
		APP_LOG(APP_LOG_LEVEL_INFO, "ALERT HANDLER: HIGH ALERT");
		#endif
		VibePattern high_alert_pat = {
			.durations = highalert_fast,
			.num_segments = HIGHALERT_FAST_STRONG,
		};
		if (TurnOffStrongVibrations) { high_alert_pat.num_segments = HIGHALERT_FAST_SHORT; };
		vibes_enqueue_custom_pattern(high_alert_pat);
		break;
	
	} // switch alertValue
	
} // end alert_handler_cgm

void BT_timer_callback(void *data);

void handle_bluetooth_cgm(bool bt_connected) {
	//APP_LOG(APP_LOG_LEVEL_INFO, "HANDLE BT: ENTER CODE");
	
	if (bt_connected == false)
	{
	
		// Check BluetoothAlert for extended Bluetooth outage; if so, do nothing
		if (BluetoothAlert) {
			//Already vibrated and set message; out
			return;
		}
	
		// Check to see if the BT_timer needs to be set; if BT_timer is not null we're still waiting
		if (BT_timer == NULL) {
			// check to see if timer has popped
			if (!BT_timer_pop) {
				//set timer
				BT_timer = app_timer_register((BT_ALERT_WAIT_SECS*MS_IN_A_SECOND), BT_timer_callback, NULL);
				// have set timer; next time we come through we will see that the timer has popped
			return;
			}
		}
		else {
			// BT_timer is not null and we're still waiting
			return;
		}
	
		// timer has popped
		// Vibrate; BluetoothAlert takes over until Bluetooth connection comes back on
		#ifdef DEBUG_LEVEL
		APP_LOG(APP_LOG_LEVEL_INFO, "BT HANDLER: TIMER POP, NO BLUETOOTH");
		#endif
		alert_handler_cgm(BTOUT_VIBE);
		BluetoothAlert = true;
	
		// Reset timer pop
		BT_timer_pop = false;
	
		//APP_LOG(APP_LOG_LEVEL_INFO, "NO BLUETOOTH");
		if (!TurnOff_NOBLUETOOTH_Msg) {
			#ifdef PBL_COLOR
			text_layer_set_text_color(message_layer, GColorRed);
			#endif
			layer_set_hidden((Layer *)message_layer, false);
			text_layer_set_text(message_layer, "NO BLUETOOTH");
		} 
		
		// erase cgm and app ago times
		text_layer_set_text(cgmtime_layer, "");
		//text_layer_set_text(time_app_layer, "");
		
		// erase cgm icon
		//create_update_bitmap(&cgmicon_bitmap,cgmicon_layer,TIMEAGO_ICONS[NONE_TIMEAGO_ICON_INDX]);
		// turn phone icon off
		//create_update_bitmap(&appicon_bitmap,appicon_layer,TIMEAGO_ICONS[PHONEOFF_ICON_INDX]);
	}
		
	else {
		// Bluetooth is on, reset BluetoothAlert
		//APP_LOG(APP_LOG_LEVEL_INFO, "HANDLE BT: BLUETOOTH ON");
		BluetoothAlert = false;
		if (BT_timer == NULL) {
			// no timer is set, so need to reset timer pop
			BT_timer_pop = false;
		}
		#ifdef PBL_COLOR
		text_layer_set_text_color(message_layer, GColorBlack);
		#endif

	}
	
	//APP_LOG(APP_LOG_LEVEL_INFO, "BluetoothAlert: %i", BluetoothAlert);
} // end handle_bluetooth_cgm

void BT_timer_callback(void *data) {
	//APP_LOG(APP_LOG_LEVEL_INFO, "BT TIMER CALLBACK: ENTER CODE");
	
	// reset timer pop and timer
	BT_timer_pop = true;
	if (BT_timer != NULL) {
		BT_timer = NULL;
	}
	
	// check bluetooth and call handler
	bluetooth_connected_cgm = bluetooth_connection_service_peek();
	handle_bluetooth_cgm(bluetooth_connected_cgm);
	
} // end BT_timer_callback

static void draw_date_from_app() {
 
	// VARIABLES
	time_t d_app = time(NULL);
	struct tm *current_d_app = localtime(&d_app);
	size_t draw_return = 0;

	// CODE START
	
	// format current date from app
	if (strcmp(time_watch_text, "00:00") == 0) {
		if(clock_is_24h_style() == true) {
				draw_return = strftime(time_watch_text, TIME_TEXTBUFF_SIZE, "%H:%M", current_d_app);
			} else {
				draw_return = strftime(time_watch_text, TIME_TEXTBUFF_SIZE, "%l:%M", current_d_app);
			}
    	if (draw_return != 0) {
    			text_layer_set_text(time_watch_layer, time_watch_text);
    	}
  	}
  
 // GSize tw = graphics_text_layout_get_content_size(time_watch_text, FONT_KEY_BITHAM_42_BOLD, time_bound ,GTextOverflowModeWordWrap,GTextAlignmentCenter );
//  	APP_LOG(APP_LOG_LEVEL_DEBUG, "time w: %i h: %i", tw.w, tw.h);
  if ( displayFormat == 2 ) {
    draw_return = strftime(date_app_text, DATE_TEXTBUFF_SIZE, "%a %d", current_d_app);
  } else {
    draw_return = strftime(date_app_text, DATE_TEXTBUFF_SIZE, "%a %d %b", current_d_app);
  }
	if (draw_return != 0) {
		text_layer_set_text(date_app_layer, date_app_text);
	}

} // end draw_date_from_app

void sync_error_callback_cgm(DictionaryResult appsync_dict_error, AppMessageResult appsync_error, void *context) {

	// VARIABLES
	DictionaryIterator *iter = NULL;
	AppMessageResult appsync_err_openerr = APP_MSG_OK;
	AppMessageResult appsync_err_senderr = APP_MSG_OK;
	
	// CODE START
	
	// APPSYNC ERROR debug logs
	#ifdef DEBUG_LEVEL
	APP_LOG(APP_LOG_LEVEL_INFO, "APP SYNC ERROR");
	APP_LOG(APP_LOG_LEVEL_DEBUG, "APP SYNC MSG ERR CODE: %i RES: %s", appsync_error, translate_app_error(appsync_error));
	APP_LOG(APP_LOG_LEVEL_DEBUG, "APP SYNC DICT ERR CODE: %i RES: %s", appsync_dict_error, translate_dict_error(appsync_dict_error));
	#endif

	bluetooth_connected_cgm = bluetooth_connection_service_peek();
		
	if (!bluetooth_connected_cgm) {
		// bluetooth is out, BT message already set; return out
		return;
	}
	
	appsync_err_openerr = app_message_outbox_begin(&iter);
	
	#ifdef DEBUG_LEVEL
	APP_LOG(APP_LOG_LEVEL_DEBUG, "APP SYNC OPEN ERR CODE: %i RES: %s", appsync_err_openerr, translate_app_error(appsync_err_openerr));
	#endif
	
	if (appsync_err_openerr == APP_MSG_OK) {
		// reset AppSyncErrAlert to flag for vibrate
		AppSyncErrAlert = false;
	
		// send message
		appsync_err_senderr = app_message_outbox_send();
		//APP_LOG(APP_LOG_LEVEL_DEBUG, "APP SYNC SEND ERR CODE: %i RES: %s", appsync_err_senderr, translate_app_error(appsync_err_senderr));
		if (appsync_err_senderr != APP_MSG_OK  && appsync_err_senderr != APP_MSG_BUSY && appsync_err_senderr != APP_MSG_SEND_REJECTED) {
			#ifdef DEBUG_LEVEL
			APP_LOG(APP_LOG_LEVEL_INFO, "APP SYNC SEND ERROR");
			APP_LOG(APP_LOG_LEVEL_DEBUG, "APP SYNC SEND ERR CODE: %i RES: %s", appsync_err_senderr, translate_app_error(appsync_err_senderr));
			#endif
		} 
		else {
			return;
		}
	}
	#if DEBUG_LEVEL > 1
	APP_LOG(APP_LOG_LEVEL_INFO, "APP SYNC RESEND ERROR");
	APP_LOG(APP_LOG_LEVEL_DEBUG, "APP SYNC RESEND ERR CODE: %i RES: %s", appsync_err_openerr, translate_app_error(appsync_err_openerr));
	APP_LOG(APP_LOG_LEVEL_DEBUG, "AppSyncErrAlert:	%i", AppSyncErrAlert);
	#endif
	bluetooth_connected_cgm = bluetooth_connection_service_peek();
		
	if (!bluetooth_connected_cgm || appsync_err_openerr == APP_MSG_BUSY) {
		// bluetooth is out, BT message already set; return out
		return;
	}
		
	// set message to RESTART WATCH -> PHONE
	layer_set_hidden((Layer *)message_layer, false);
	#ifdef DEBUG
	text_layer_set_text(message_layer, translate_app_error(appsync_err_openerr));
	#else
	text_layer_set_text(message_layer, "RSTRT WCH/PH");
	#endif
		
	// erase cgm and app ago times
	text_layer_set_text(cgmtime_layer, "");
	//text_layer_set_text(time_app_layer, "");
		
	// erase cgm icon
	//create_update_bitmap(&cgmicon_bitmap,cgmicon_layer,TIMEAGO_ICONS[NONE_TIMEAGO_ICON_INDX]);
		
	// turn phone icon off
	//create_update_bitmap(&appicon_bitmap,appicon_layer,TIMEAGO_ICONS[PHONEOFF_ICON_INDX]);

	// check if need to vibrate
	if (!AppSyncErrAlert) {
		#ifdef DEBUG_LEVEL
		APP_LOG(APP_LOG_LEVEL_INFO, "APPSYNC ERROR: VIBRATE");
		#endif
		alert_handler_cgm(APPSYNC_ERR_VIBE);
		AppSyncErrAlert = true;
	} 
		
} // end sync_error_callback_cgm

void inbox_dropped_handler_cgm(AppMessageResult appmsg_indrop_error, void *context) {
	// incoming appmessage send back from Pebble app dropped; no data received
	
	// VARIABLES
	DictionaryIterator *iter = NULL;
	AppMessageResult appmsg_indrop_openerr = APP_MSG_OK;
	AppMessageResult appmsg_indrop_senderr = APP_MSG_OK;
	
	// CODE START
	
	// APPMSG IN DROP debug logs
	#ifdef DEBUG_LEVEL
	APP_LOG(APP_LOG_LEVEL_INFO, "APPMSG IN DROP ERROR");
	APP_LOG(APP_LOG_LEVEL_DEBUG, "APPMSG IN DROP ERR CODE: %i RES: %s", appmsg_indrop_error, translate_app_error(appmsg_indrop_error));
	#endif
	bluetooth_connected_cgm = bluetooth_connection_service_peek();
		
	if (!bluetooth_connected_cgm) {
			// bluetooth is out, BT message already set; return out
			return;
	}
	
	appmsg_indrop_openerr = app_message_outbox_begin(&iter);
	
	if (appmsg_indrop_openerr == APP_MSG_OK ) {
			// reset AppMsgInDropAlert to flag for vibrate
		AppMsgInDropAlert = false;
	
			// send message
			appmsg_indrop_senderr = app_message_outbox_send();
		if (appmsg_indrop_senderr != APP_MSG_OK || appmsg_indrop_senderr == APP_MSG_BUSY || appmsg_indrop_senderr == APP_MSG_SEND_REJECTED) {
			#ifdef DEBUG_LEVEL
			APP_LOG(APP_LOG_LEVEL_INFO, "APPMSG IN DROP SEND ERROR");
			APP_LOG(APP_LOG_LEVEL_DEBUG, "APPMSG IN DROP SEND ERR CODE: %i RES: %s", appmsg_indrop_senderr, translate_app_error(appmsg_indrop_senderr));
			#endif
		} 
		else {
			return;
		}
	}
	#ifdef DEBUG_LEVEL
	APP_LOG(APP_LOG_LEVEL_INFO, "APPMSG IN DROP RESEND ERROR");
	APP_LOG(APP_LOG_LEVEL_DEBUG, "APPMSG IN DROP RESEND ERR CODE: %i RES: %s", appmsg_indrop_openerr, translate_app_error(appmsg_indrop_openerr));
	APP_LOG(APP_LOG_LEVEL_DEBUG, "AppMsgInDropAlert:	%i", AppMsgInDropAlert);
	#endif
		
	bluetooth_connected_cgm = bluetooth_connection_service_peek();
		
	if (!bluetooth_connected_cgm) {
			// bluetooth is out, BT message already set; return out
			return;
	}
		
	// set message to RESTART WATCH -> PHONE
	layer_set_hidden((Layer *)message_layer, false);
	text_layer_set_text(message_layer, "RSTRT WCH/PHN");
		
	// erase cgm and app ago times
	text_layer_set_text(cgmtime_layer, "");
	//text_layer_set_text(time_app_layer, "");
		
	// erase cgm icon
	//create_update_bitmap(&cgmicon_bitmap,cgmicon_layer,TIMEAGO_ICONS[NONE_TIMEAGO_ICON_INDX]);
		
	// turn phone icon off
	//create_update_bitmap(&appicon_bitmap,appicon_layer,TIMEAGO_ICONS[PHONEOFF_ICON_INDX]);

	// check if need to vibrate
	if (!AppMsgInDropAlert) {
		#ifdef DEBUG_LEVEL
		APP_LOG(APP_LOG_LEVEL_INFO, "APPMSG IN DROP ERROR: VIBRATE");
		#endif
		alert_handler_cgm(APPMSG_INDROP_VIBE);
		AppMsgInDropAlert = true;
	} 
		
} // end inbox_dropped_handler_cgm









void outbox_failed_handler_cgm(DictionaryIterator *failed, AppMessageResult appmsg_outfail_error, void *context) {
	// outgoing appmessage send failed to deliver to Pebble
	
	// VARIABLES
	DictionaryIterator *iter = NULL;
	AppMessageResult appmsg_outfail_openerr = APP_MSG_OK;
	AppMessageResult appmsg_outfail_senderr = APP_MSG_OK;
	
	// CODE START
	
	// APPMSG OUT FAIL debug logs
	#ifdef DEBUG_LEVEL
	APP_LOG(APP_LOG_LEVEL_INFO, "APPMSG OUT FAIL ERROR");
	APP_LOG(APP_LOG_LEVEL_DEBUG, "APPMSG OUT FAIL ERR CODE: %i RES: %s", appmsg_outfail_error, translate_app_error(appmsg_outfail_error));
	#endif

	bluetooth_connected_cgm = bluetooth_connection_service_peek();
		
	if (!bluetooth_connected_cgm) {
			// bluetooth is out, BT message already set; return out
			return;
	}
	
	appmsg_outfail_openerr = app_message_outbox_begin(&iter);
	
	if (appmsg_outfail_openerr == APP_MSG_OK) {
		// reset AppMsgOutFailAlert to flag for vibrate
		AppMsgOutFailAlert = false;
	
		// send message
/*		appmsg_outfail_senderr = app_message_outbox_send();
		if (appmsg_outfail_senderr != APP_MSG_OK) {
			#ifdef DEBUG_LEVEL
			APP_LOG(APP_LOG_LEVEL_INFO, "APPMSG OUT FAIL SEND ERROR");
			APP_LOG(APP_LOG_LEVEL_DEBUG, "APPMSG OUT FAIL SEND ERR CODE: %i RES: %s", appmsg_outfail_senderr, translate_app_error(appmsg_outfail_senderr));
			#endif
		} 
		else {
*/			return;
//		}
	}

	#ifdef DEBUG_LEVEL
	APP_LOG(APP_LOG_LEVEL_INFO, "APPMSG OUT FAIL RESEND ERROR");
	APP_LOG(APP_LOG_LEVEL_DEBUG, "APPMSG OUT FAIL RESEND ERR CODE: %i RES: %s", appmsg_outfail_openerr, translate_app_error(appmsg_outfail_openerr));
	APP_LOG(APP_LOG_LEVEL_DEBUG, "AppMsgOutFailAlert:	%i", AppMsgOutFailAlert);
	#endif
		
	bluetooth_connected_cgm = bluetooth_connection_service_peek();
		
	if (!bluetooth_connected_cgm || appmsg_outfail_senderr != APP_MSG_SEND_REJECTED) {
			// bluetooth is out, BT message already set; return out
			return;
	}
		
	// set message to RESTART WATCH -> PHONE
	layer_set_hidden((Layer *)message_layer, false);
	#ifdef DEBUG
	text_layer_set_text(message_layer, translate_app_error(appmsg_outfail_openerr));
	#else
	text_layer_set_text(message_layer, "RSTRT WCH/PH");
	#endif
		
	// erase cgm and app ago times
	text_layer_set_text(cgmtime_layer, "");
	//text_layer_set_text(time_app_layer, "");
		
	// erase cgm icon
	//create_update_bitmap(&cgmicon_bitmap,cgmicon_layer,TIMEAGO_ICONS[NONE_TIMEAGO_ICON_INDX]);
		
	// turn phone icon off
	//create_update_bitmap(&appicon_bitmap,appicon_layer,TIMEAGO_ICONS[PHONEOFF_ICON_INDX]);

	// check if need to vibrate
	if (!AppMsgOutFailAlert) {
		#ifdef DEBUG_LEVEL
		APP_LOG(APP_LOG_LEVEL_INFO, "APPMSG OUT FAIL ERROR: VIBRATE");
		#endif
		alert_handler_cgm(APPMSG_OUTFAIL_VIBE);
		AppMsgOutFailAlert = true;
	} 
	
} // end outbox_failed_handler_cgm

static void load_icon() {
	//APP_LOG(APP_LOG_LEVEL_INFO, "LOAD ICON ARROW FUNCTION START");
	
	// CONSTANTS
	
	// ICON ASSIGNMENTS OF ARROW DIRECTIONS
	const char NO_ARROW[] = "0";
	const char DOUBLEUP_ARROW[] = "1";
	const char SINGLEUP_ARROW[] = "2";
	const char UP45_ARROW[] = "3";
	const char FLAT_ARROW[] = "4";
	const char DOWN45_ARROW[] = "5";
	const char SINGLEDOWN_ARROW[] = "6";
	const char DOUBLEDOWN_ARROW[] = "7";
	const char NOTCOMPUTE_ICON[] = "8";
	const char OUTOFRANGE_ICON[] = "9";
	
	// ARRAY OF ARROW ICON IMAGES
	const uint8_t ARROW_ICONS[] = {
		RESOURCE_ID_IMAGE_NONE,		 //0
		RESOURCE_ID_IMAGE_UPUP,		 //1
		RESOURCE_ID_IMAGE_UP,			 //2
		RESOURCE_ID_IMAGE_UP45,		 //3
		RESOURCE_ID_IMAGE_FLAT,		 //4
		RESOURCE_ID_IMAGE_DOWN45,	 //5
		RESOURCE_ID_IMAGE_DOWN,		 //6
		RESOURCE_ID_IMAGE_DOWNDOWN, //7
		RESOURCE_ID_IMAGE_LOGO,		 //8
		RESOURCE_ID_IMAGE_ERR			 //9
	};
		
	// INDEX FOR ARRAY OF ARROW ICON IMAGES
	const uint8_t NONE_ARROW_ICON_INDX = 0;
	const uint8_t UPUP_ICON_INDX = 1;
	const uint8_t UP_ICON_INDX = 2;
	const uint8_t UP45_ICON_INDX = 3;
	const uint8_t FLAT_ICON_INDX = 4;
	const uint8_t DOWN45_ICON_INDX = 5;
	const uint8_t DOWN_ICON_INDX = 6;
	const uint8_t DOWNDOWN_ICON_INDX = 7;
	const uint8_t LOGO_ARROW_ICON_INDX = 8;
	const uint8_t ERR_ARROW_ICON_INDX = 9;
	
	//APP_LOG(APP_LOG_LEVEL_INFO, "LOAD ARROW ICON, BEFORE CHECK SPEC VALUE BITMAP");
	// check if special value set
	if (specvalue_alert == false) {
	
		// no special value, set arrow
			// check for arrow direction, set proper arrow icon
		//APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD ICON, CURRENT ICON: %s", current_icon);
			if ( (strcmp(current_icon, NO_ARROW) == 0) || (strcmp(current_icon, NOTCOMPUTE_ICON) == 0) || (strcmp(current_icon, OUTOFRANGE_ICON) == 0) ) {
			create_update_bitmap(&icon_bitmap,icon_layer,ARROW_ICONS[NONE_ARROW_ICON_INDX]);
			DoubleDownAlert = false;
			} 
			else if (strcmp(current_icon, DOUBLEUP_ARROW) == 0) {
			create_update_bitmap(&icon_bitmap,icon_layer,ARROW_ICONS[UPUP_ICON_INDX]);
			DoubleDownAlert = false;
			}
			else if (strcmp(current_icon, SINGLEUP_ARROW) == 0) {
			create_update_bitmap(&icon_bitmap,icon_layer,ARROW_ICONS[UP_ICON_INDX]);
			DoubleDownAlert = false;
			}
			else if (strcmp(current_icon, UP45_ARROW) == 0) {
				create_update_bitmap(&icon_bitmap,icon_layer,ARROW_ICONS[UP45_ICON_INDX]);
			DoubleDownAlert = false;
			}
			else if (strcmp(current_icon, FLAT_ARROW) == 0) {
				create_update_bitmap(&icon_bitmap,icon_layer,ARROW_ICONS[FLAT_ICON_INDX]);
			DoubleDownAlert = false;
			}
			else if (strcmp(current_icon, DOWN45_ARROW) == 0) {
			create_update_bitmap(&icon_bitmap,icon_layer,ARROW_ICONS[DOWN45_ICON_INDX]);
			DoubleDownAlert = false;
			}
			else if (strcmp(current_icon, SINGLEDOWN_ARROW) == 0) {
				create_update_bitmap(&icon_bitmap,icon_layer,ARROW_ICONS[DOWN_ICON_INDX]);
			DoubleDownAlert = false;
			}
			else if (strcmp(current_icon, DOUBLEDOWN_ARROW) == 0) {
/*				if (!DoubleDownAlert) {
					APP_LOG(APP_LOG_LEVEL_INFO, "LOAD ICON, ICON ARROW: DOUBLE DOWN");
					alert_handler_cgm(DOUBLEDOWN_VIBE);
					DoubleDownAlert = true;
				}
*/				create_update_bitmap(&icon_bitmap,icon_layer,ARROW_ICONS[DOWNDOWN_ICON_INDX]);
			} 
			else {
				// check for special cases and set icon accordingly
				// check bluetooth
				bluetooth_connected_cgm = bluetooth_connection_service_peek();
					
				// check to see if we are in the loading screen	
				if (!bluetooth_connected_cgm) {
				// Bluetooth is out; in the loading screen so set logo
					create_update_bitmap(&icon_bitmap,icon_layer,ARROW_ICONS[LOGO_ARROW_ICON_INDX]);
				}
				else {
				// unexpected, set error icon
				create_update_bitmap(&icon_bitmap,icon_layer,ARROW_ICONS[ERR_ARROW_ICON_INDX]);
			}
			DoubleDownAlert = false;
		}
	} // if specvalue_alert == false
	else { // this is just for log when need it
		//APP_LOG(APP_LOG_LEVEL_INFO, "LOAD ICON, SPEC VALUE ALERT IS TRUE, DONE");
	} // else specvalue_alert == true
	
} // end load_icon

static void load_bg() {
		//APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, FUNCTION START");
	
	// CONSTANTS
	#define SENSOR_NOT_ACTIVE_VALUE "?SN"	
	#define MINIMAL_DEVIATION_VALUE	"?MD"
	#define NO_ANTENNA_VALUE "?NA"
	#define SENSOR_NOT_CALIBRATED_VALUE "?NC"
	#define STOP_LIGHT_VALUE "?CD"
	#define HOURGLASS_VALUE "hourglass"
	#define QUESTION_MARKS_VALUE "???"
	#define BAD_RF_VALUE "?RF"
	// CODE START
	
	// if special value set, erase anything in the icon field
	if (specvalue_alert == true) {
		create_update_bitmap(&specialvalue_bitmap,icon_layer,SPECIAL_VALUE_ICONS[NONE_SPECVALUE_ICON_INDX]);
	}

	// set special value alert to false no matter what
	specvalue_alert = false;
  

	#if DEBUG_LEVEL > 1
	APP_LOG(APP_LOG_LEVEL_DEBUG, "LAST BG: %s", last_bg);
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "CURRENT BG: %i", current_bg);
	#endif	
	
	// BG parse, check snooze, and set text 
		
	// check for init code or error code
	if (last_bg[0] == '-') {
		lastAlertTime = 0;
	
		// check bluetooth
		bluetooth_connected_cgm = bluetooth_connection_service_peek();
			
		if (!bluetooth_connected_cgm) {
			// Bluetooth is out; set BT message
			//APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, BG INIT: NO BT, SET NO BT MESSAGE");
			if (!TurnOff_NOBLUETOOTH_Msg) {
				layer_set_hidden((Layer *)message_layer, false);
				text_layer_set_text(message_layer, "NO BLUETOOTH");
			} // if turnoff nobluetooth msg
		}// if !bluetooth connected
		else {
			// if init code, we will set it right in message layer
			//APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, UNEXPECTED BG: SET ERR ICON");
			//APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD BG, UNEXP BG, CURRENT_BG: %d LAST_BG: %s ", current_bg, last_bg);
			text_layer_set_text(bg_layer, "ERR");
			create_update_bitmap(&icon_bitmap,icon_layer,SPECIAL_VALUE_ICONS[NONE_SPECVALUE_ICON_INDX]);
			specvalue_alert = true;
		}
			
	} // if current_bg <= 0
		
	else {

// valid BG
		// check for special value, if special value, then replace icon and blank BG; else send current BG	
		//APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, BEFORE CREATE SPEC VALUE BITMAP");
		layer_set_hidden((Layer *)message_layer, false);
		text_layer_set_text(message_layer, "");
		if (strcmp(last_bg, NO_ANTENNA_VALUE) == 0 || strcmp(last_bg, BAD_RF_VALUE) == 0) {
			//APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, SPECIAL VALUE: SET BROKEN ANTENNA");
			text_layer_set_text(bg_layer, "");
			create_update_bitmap(&specialvalue_bitmap,icon_layer, SPECIAL_VALUE_ICONS[BROKEN_ANTENNA_ICON_INDX]);
			specvalue_alert = true;
			text_layer_set_text(message_layer, "NO CONNECTION");
		}
		else if (strcmp(last_bg, SENSOR_NOT_CALIBRATED_VALUE) == 0) {
			//APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, SPECIAL VALUE: SET BLOOD DROP");
			text_layer_set_text(bg_layer, "");
			create_update_bitmap(&specialvalue_bitmap,icon_layer,SPECIAL_VALUE_ICONS[BLOOD_DROP_ICON_INDX]);
			specvalue_alert = true;				
			text_layer_set_text(message_layer, "CALIBRATE");
		}
		else if (strcmp(last_bg, SENSOR_NOT_ACTIVE_VALUE) == 0 || strcmp(last_bg, MINIMAL_DEVIATION_VALUE) == 0 
				|| strcmp(last_bg, STOP_LIGHT_VALUE) == 0) {
			//APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, SPECIAL VALUE: SET STOP LIGHT");
			text_layer_set_text(bg_layer, "");
			create_update_bitmap(&specialvalue_bitmap,icon_layer,SPECIAL_VALUE_ICONS[STOP_LIGHT_ICON_INDX]);
			specvalue_alert = true;
			text_layer_set_text(message_layer, "NO SENSOR");
		}
		else if (strcmp(last_bg, HOURGLASS_VALUE) == 0) {
			//APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, SPECIAL VALUE: SET HOUR GLASS");
			text_layer_set_text(bg_layer, "");
			create_update_bitmap(&specialvalue_bitmap,icon_layer,SPECIAL_VALUE_ICONS[HOURGLASS_ICON_INDX]);
			specvalue_alert = true;
		}
		else if (strcmp(last_bg, QUESTION_MARKS_VALUE) == 0) {
			//PP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, SPECIAL VALUE: SET QUESTION MARKS, CLEAR TEXT");
			text_layer_set_text(bg_layer, "");
			//APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, SPECIAL VALUE: SET QUESTION MARKS, SET BITMAP");
			create_update_bitmap(&specialvalue_bitmap,icon_layer,SPECIAL_VALUE_ICONS[QUESTION_MARKS_ICON_INDX]); 
			//APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, SPECIAL VALUE: SET QUESTION MARKS, DONE");
			specvalue_alert = true;
		}
		//APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, AFTER CREATE SPEC VALUE BITMAP");
			
		if (specvalue_alert == false) {
			// we didn't find a special value, so set BG instead
			// arrow icon already set separately
			//APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD BG, SET TO BG: %s ", last_bg);
      #ifdef PBL_COLOR
 	     GColor inTarget_color = COLOR_FALLBACK(GColorDukeBlue, GColorBlack) ;
       if ( strncmp(last_bg, " ",1) != 0 ) {
          if ( myAtoi(last_bg) <  myAtoi(low_bg) ) {
            inTarget_color = GColorRed ;
          } else if ( myAtoi(last_bg) > myAtoi(high_bg) ) {
            inTarget_color = GColorChromeYellow ;
          }
        }
       text_layer_set_background_color(lower_layer, inTarget_color);
     #endif
      
     
      if (strcmp(last_bg,  " ") == 0) {
        text_layer_set_text(bg_layer, "---");
      } else {
		    text_layer_set_text(bg_layer, last_bg);
      }
		} // end bg checks (if special_value_bitmap)
	} // else if current bg <= 0
			
	//APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, FUNCTION OUT");
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD BG, FUNCTION OUT, SNOOZE VALUE: %d", lastAlertTime);
	#ifdef DEBUG_LEVEL
	APP_LOG(APP_LOG_LEVEL_INFO, "LOAD_BG: bg_layer is \"%s\"", text_layer_get_text(bg_layer));
	#endif

	
} // end load_bg

// Gets the UTC offset of the local time in seconds 
// (pass in an existing localtime struct tm to save creating another one, or else pass NULL)
time_t get_UTC_offset(struct tm *t) {
#ifdef PBL_SDK_3
  if (t == NULL) {
    time_t temp;
    temp = time(NULL);
    t = localtime(&temp);
  }
  
  return t->tm_gmtoff + ((t->tm_isdst > 0) ? 3600 : 0);
#else
  // Aplite uses localtime instead of UTC for all time functions so always return 0
  return 0; 
#endif 
}

static void load_cgmtime() {
	//APP_LOG(APP_LOG_LEVEL_INFO, "LOAD CGMTIME FUNCTION START");
	
	// VARIABLES
	// NOTE: buffers have to be static and hardcoded
	uint32_t current_cgm_timeago = 0;
	int cgm_timeago_diff = 0;
	static char formatted_cgm_timeago[10];
	static char cgm_label_buffer[6];
		
	// CODE START
	
	// initialize label buffer
	strncpy(cgm_label_buffer, "", LABEL_BUFFER_SIZE);
			 
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD CGMTIME, NEW CGM TIME: %s", new_cgm_time);
		
	if (current_cgm_time == 0) {		 
		// Init code or error code; set text layer & icon to empty value 
		//APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD CGMTIME, CGM TIME AGO INIT OR ERROR CODE: %s", cgm_label_buffer);
		text_layer_set_text(cgmtime_layer, "now");
		//create_update_bitmap(&cgmicon_bitmap,cgmicon_layer,TIMEAGO_ICONS[NONE_TIMEAGO_ICON_INDX]);						
	}
	else {
		// set rcvr on icon
		//create_update_bitmap(&cgmicon_bitmap,cgmicon_layer,TIMEAGO_ICONS[RCVRON_ICON_INDX]);
		
		time_now = time(NULL);
		time_now = abs(time_now + get_UTC_offset(localtime(&time_now)));
		
		//APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD CGMTIME, CURRENT CGM TIME: %lu", current_cgm_time);
		#ifdef DEBUG_LEVEL
		APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD CGMTIME, time_now: %lu, current_cgm_time: %lu", time_now, current_cgm_time);
		#endif
						
		current_cgm_timeago = abs(time_now - current_cgm_time);
				
		//APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD CGMTIME, CURRENT CGM TIMEAGO: %lu", current_cgm_timeago);
			
		//APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD CGMTIME, GM TIME AGO LABEL IN: %s", cgm_label_buffer);
			
		if (current_cgm_timeago < MINUTEAGO) {
			cgm_timeago_diff = 0;
			strncpy (formatted_cgm_timeago, "now", TIMEAGO_BUFFER_SIZE);
		}
		else if (current_cgm_timeago < HOURAGO) {
			cgm_timeago_diff = (current_cgm_timeago / MINUTEAGO);
			snprintf(formatted_cgm_timeago, TIMEAGO_BUFFER_SIZE, "%i", cgm_timeago_diff);
			strncpy(cgm_label_buffer, "m", LABEL_BUFFER_SIZE);
			strcat(formatted_cgm_timeago, cgm_label_buffer);
		}
		else if (current_cgm_timeago < DAYAGO) {
			cgm_timeago_diff = (current_cgm_timeago / HOURAGO);
			snprintf(formatted_cgm_timeago, TIMEAGO_BUFFER_SIZE, "%i", cgm_timeago_diff);
			strncpy(cgm_label_buffer, "h", LABEL_BUFFER_SIZE);
			strcat(formatted_cgm_timeago, cgm_label_buffer);
		}
		else if (current_cgm_timeago < WEEKAGO) {
			cgm_timeago_diff = (current_cgm_timeago / DAYAGO);
			snprintf(formatted_cgm_timeago, TIMEAGO_BUFFER_SIZE, "%i", cgm_timeago_diff);
			strncpy(cgm_label_buffer, "d", LABEL_BUFFER_SIZE);
			strcat(formatted_cgm_timeago, cgm_label_buffer);
		}
		else {
			strncpy (formatted_cgm_timeago, "---", TIMEAGO_BUFFER_SIZE);
			//create_update_bitmap(&cgmicon_bitmap,cgmicon_layer,TIMEAGO_ICONS[NONE_TIMEAGO_ICON_INDX]);				
		}
			
		text_layer_set_text(cgmtime_layer, formatted_cgm_timeago);	
	} // else init code
		
	//#ifdef DEBUG_LEVEL
	//APP_LOG(APP_LOG_LEVEL_INFO, "LOAD_CGMTIME: time_app_layer is \"%s\"", text_layer_get_text(cgmtime_layer));
	//#endif
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD CGMTIME, CGM TIMEAGO LABEL OUT: %s", cgm_label_buffer);
} // end load_cgmtime

static void load_bg_delta() {
	#ifdef DEBUG_LEVEL
	APP_LOG(APP_LOG_LEVEL_INFO, "BG DELTA FUNCTION START");
	#endif
	#ifdef DEBUG_LEVEL
	APP_LOG(APP_LOG_LEVEL_INFO, "LOAD_BG_DELTA: current_bg_delta is \"%s\"", current_bg_delta);
	#endif

	
	// CONSTANTS
/*	const uint8_t MSGLAYER_BUFFER_SIZE = 14;
	const uint8_t BGDELTA_LABEL_SIZE = 14;
	const uint8_t BGDELTA_FORMATTED_SIZE = 14;
*/	
	#define MSGLAYER_BUFFER_SIZE 14
	#define BGDELTA_LABEL_SIZE 14
	#define BGDELTA_FORMATTED_SIZE 14
	// VARIABLES
	// NOTE: buffers have to be static and hardcoded
	//static char delta_label_buffer[BGDELTA_LABEL_SIZE];
	static char formatted_bg_delta[BGDELTA_FORMATTED_SIZE];

	// CODE START
	display_message = true;
	
	// check for CHECK PHONE condition, if true set message
	if ((PhoneOffAlert) && (!TurnOff_CHECKPHONE_Msg)) {
		layer_set_hidden((Layer *)message_layer, false);
		text_layer_set_text(message_layer, "CHECK PHONE");
		return;	
	}
	
	// check for special messages; if no string, set no message
	if (strcmp(current_bg_delta, "") == 0) {
		strncpy(formatted_bg_delta, "", MSGLAYER_BUFFER_SIZE);
		layer_set_hidden((Layer *)message_layer, false);
		text_layer_set_text(message_layer, formatted_bg_delta);
		return;	
	}
	

	// check if LOADING.., if true set message
	// put " " (space) in bg field so logo continues to show
	if (strcmp(current_bg_delta, "LOAD") == 0) {
			#ifdef DEBUG_LEVEL
			APP_LOG(APP_LOG_LEVEL_INFO, "LOAD_BG_DELTA: Found LOAD, current_bg_delta is \"%s\"", current_bg_delta);
			#endif

		strncpy(formatted_bg_delta, "LOADING...", MSGLAYER_BUFFER_SIZE);
		layer_set_hidden((Layer *)message_layer, false);
		text_layer_set_text(message_layer, formatted_bg_delta);
		create_update_bitmap(&icon_bitmap,icon_layer,SPECIAL_VALUE_ICONS[LOGO_SPECVALUE_ICON_INDX]);
		specvalue_alert = false;
		return;	
	}
 
	//check for "--" indicating an indeterminate delta.  Display it.
	if (strcmp(current_bg_delta, "???") == 0) {
		strncpy(formatted_bg_delta, current_bg_delta, BGDELTA_FORMATTED_SIZE);
		text_layer_set_text(delta_layer, formatted_bg_delta);
		return;	
	}
	
	//check for "ERR" indicating an indeterminate delta.  Display it.
	if (strcmp(current_bg_delta, "ERR") == 0) {
		strncpy(formatted_bg_delta, current_bg_delta, BGDELTA_FORMATTED_SIZE);
		layer_set_hidden((Layer *)message_layer, false);
		text_layer_set_text(message_layer, formatted_bg_delta);
		return;	
	}

	// Bluetooth is good, Phone is good, CGM connection is good, no special message 
	// set delta BG message
	
	#ifdef DEBUG_LEVEL
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD BG DELTA, DELTA STRING: %s", &current_bg_delta[i]);
	#endif
	//strcat(formatted_bg_delta, delta_label_buffer);
	strncpy(formatted_bg_delta, current_bg_delta, BGDELTA_FORMATTED_SIZE);

  #ifdef DEBUG_LEVEL
  APP_LOG(APP_LOG_LEVEL_INFO, "LOAD_BG_DELTA: All good. Setting \"%s\"", formatted_bg_delta);
  #endif
  if ( strncmp(formatted_bg_delta,"+",1) == 0 || strncmp(formatted_bg_delta,"-",1) == 0 || strncmp(formatted_bg_delta,"0",1) == 0 ) {
    display_message = false;
    text_layer_set_text(delta_layer, formatted_bg_delta);
    text_layer_set_text(message_layer, "");
  } else {
	  layer_set_hidden((Layer *)message_layer, false);
	  text_layer_set_text(message_layer, formatted_bg_delta);
  }
	#ifdef DEBUG_LEVEL
  APP_LOG(APP_LOG_LEVEL_INFO, "LOAD_BG_DELTA: delta_layer is \"%s\"", text_layer_get_text(delta_layer));
	#endif
	
} // end load_bg_delta

static void load_battlevel() {
	
	#if DEBUG_LEVEL >0
	APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD BATTLEVEL, LAST BATTLEVEL: %s", last_battlevel);
	#endif
  if (battery_graphic) {
    layer_mark_dirty(bridge_battery_draw_layer);
  } else {
    static char bridge_battlevel_percent[9];
    snprintf(bridge_battlevel_percent, BATTLEVEL_FORMATTED_SIZE, "B:%s", last_battlevel);
    text_layer_set_text_color(bridge_battery_text_layer, bridge_battery_border_color);
    #ifdef PBL_COLOR
    if(atoi(last_battlevel) < 20 ) {
      text_layer_set_text_color(bridge_battery_text_layer, bridge_battery_low_color);
    }
    #endif
    text_layer_set_text(bridge_battery_text_layer, bridge_battlevel_percent);
  }
	//APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BATTLEVEL, END FUNCTION");
} // end load_battlevel

//void sync_tuple_changed_callback_cgm(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context)
void inbox_received_handler_cgm(DictionaryIterator *iterator, void *context) {
	Tuple *data = dict_read_first(iterator);
	#ifdef DEBUG_LEVEL
	//APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE");
	APP_LOG(APP_LOG_LEVEL_INFO, "inbox_received_callback_cgm: got dictionary");
	#endif


	// CONSTANTS
//	#define ICON_MSGSTR_SIZE 4
//	#define BG_MSGSTR_SIZE 6
//	#define BGDELTA_MSGSTR_SIZE 13
//	#define BATTLEVEL_MSGSTR_SIZE 5

	// CODE START
	
	while(data != NULL) {
		#ifdef DEBUG_LEVEL
		APP_LOG(APP_LOG_LEVEL_INFO, "inbox_received_callback_cgm: key is %lu", data->key);
		#endif
		switch (data->key) {

		case CGM_ICON_KEY:;
			#ifdef DEBUG_LEVEL
			APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE: ICON ARROW");
			#endif
			strncpy(current_icon, data->value->cstring, ICON_MSGSTR_SIZE);
			load_icon();
			break; // break for CGM_ICON_KEY

		case CGM_BG_KEY:;
			#ifdef DEBUG_LEVEL
			APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE: BG CURRENT");
			#endif
			strncpy(last_bg, data->value->cstring, BG_MSGSTR_SIZE);
			load_bg();
			break; // break for CGM_BG_KEY

		case CGM_TCGM_KEY:;
			#ifdef DEBUG_LEVEL
			APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE: READ CGM TIME");
			#endif
			current_cgm_time = data->value->uint32;
			load_cgmtime();
			// as long as current_cgm_time is not zero, we know we have gotten an update from the app,
			// so we reset minutes_cgm to 6, so the pebble doesn't request another one.
			if (current_cgm_time != 0) {		
				minutes_cgm = 6;
			}
			break; // break for CGM_TCGM_KEY

/*		case CGM_TAPP_KEY:;
			#ifdef DEBUG_LEVEL
			APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE: READ APP TIME NOW");
			#endif
			current_app_time = data->value->uint32;
			load_apptime();		
			break; // break for CGM_TAPP_KEY
*/	
		case CGM_DLTA_KEY:;
			strncpy(current_bg_delta, data->value->cstring, BGDELTA_MSGSTR_SIZE);
			#ifdef DEBUG_LEVEL
		 	APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE: BG DELTA - %s", current_bg_delta);
			#endif
			load_bg_delta();
			break; // break for CGM_DLTA_KEY
	
		case CGM_UBAT_KEY:;
			#ifdef DEBUG_LEVEL
		 	APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE: UPLOADER BATTERY LEVEL");
			#endif
		 	//APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE: BATTERY LEVEL IN, COPY LAST BATTLEVEL");
			strncpy(last_battlevel, data->value->cstring, BATTLEVEL_MSGSTR_SIZE);
			//APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE: BATTERY LEVEL, CALL LOAD BATTLEVEL");
			load_battlevel();
			//APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE: BATTERY LEVEL OUT");
			break; // break for CGM_UBAT_KEY

		case CGM_NAME_KEY:
			//APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE: T1D NAME");
      APP_LOG(APP_LOG_LEVEL_INFO, "NAME_KEY; Name key: %s", data->value->cstring );
			//text_layer_set_text(t1dname_layer, new_tuple->value->cstring);
			break; // break for CGM_NAME_KEY

		
		case CGM_TREND_BEGIN_KEY:
      #ifdef PBL_BW
        break;
      #endif
			expected_trend_buffer_length = data->value->uint16;
			#ifdef DEBUG_LEVEL
			APP_LOG(APP_LOG_LEVEL_INFO, "TREND_BEGIN; About to receive Trend Image of %i size.", expected_trend_buffer_length);
			#endif
			if(trend_buffer) {
				#ifdef DEBUG_LEVEL
				APP_LOG(APP_LOG_LEVEL_INFO, "TREND_BEGIN; Freeing trend_buffer.");
				#endif
				free(trend_buffer);
			}
			#ifdef DEBUG_LEVEL
			APP_LOG(APP_LOG_LEVEL_INFO, "TREND_BEGIN; Allocating trend_buffer.");
			#endif			
			trend_buffer = malloc(expected_trend_buffer_length);
			trend_buffer_length = 0;
			#if DEBUG_LEVEL > 1
			if(trend_buffer == NULL) {
				APP_LOG(APP_LOG_LEVEL_DEBUG, "TREND_BEGIN: Could not allocate trend_buffer");
				break;
			}
			APP_LOG(APP_LOG_LEVEL_DEBUG, "TREND_BEGIN: trend_buffer is %lx, trend_buffer_length is %i", (uint32_t)trend_buffer, trend_buffer_length);
			#endif
			break;
		case CGM_TREND_DATA_KEY:
      #ifdef PBL_BW
        break;
      #endif
			#ifdef DEBUG_LEVEL
			APP_LOG(APP_LOG_LEVEL_INFO, "TREND_DATA: receiving Trend Image chunk");
			#endif
			if(trend_buffer) {
				memcpy((trend_buffer+trend_buffer_length), data->value->data, data->length);
				trend_buffer_length += data->length;
				#ifdef DEBUG_LEVEL
				APP_LOG(APP_LOG_LEVEL_INFO, "TREND_DATA: received %u of %u so far", trend_buffer_length, expected_trend_buffer_length);
				#endif
			}
			#if DEBUG_LEVEL > 1
			else {
				APP_LOG(APP_LOG_LEVEL_DEBUG, "TREND_DATA: trend_buffer not allocated, ignoring");
			}
			#endif
			if(trend_buffer_length == expected_trend_buffer_length) doing_trend = true;
			break;
		
		case CGM_TREND_END_KEY:
      #ifdef PBL_BW
        break;
      #endif
			if(!doing_trend) {
				#ifdef DEBUG_LEVEL
				APP_LOG(APP_LOG_LEVEL_INFO, "Got a TREND_END without TREND_START");
				#endif
				break;
			}
			#ifdef DEBUG_LEVEL
			APP_LOG(APP_LOG_LEVEL_INFO, "Finished receiving Trend Image");
			#endif
			if(bg_trend_bitmap != NULL) {
				#if DEBUG_LEVEL > 1
				APP_LOG(APP_LOG_LEVEL_INFO, "Destroying bg_trend_bitmap");
				#endif
				gbitmap_destroy(bg_trend_bitmap);
				bg_trend_bitmap = NULL;
			}

			#ifdef DEBUG_LEVEL
			APP_LOG(APP_LOG_LEVEL_INFO, "Creating Trend Image");
			#endif
			
	//		APP_LOG(APP_LOG_LEVEL_DEBUG, "TREND_BEGIN: trend_buffer is %lx, trend_buffer_length is %i", (uint32_t)trend_buffer, trend_buffer_length);
//			#ifdef PBL_PLATFORM_BASALT
			bg_trend_bitmap = gbitmap_create_from_png_data(trend_buffer, trend_buffer_length);
      if (bg_trend_bitmap == NULL) {
        
        APP_LOG(APP_LOG_LEVEL_DEBUG, "TREND_BEGIN: can not allocate");
      }
  

			if(bg_trend_bitmap != NULL) {				
				#ifdef DEBUG_LEVEL
				APP_LOG(APP_LOG_LEVEL_INFO, "bg_trend_bitmap created, setting to layer");
				#endif
				bitmap_layer_set_bitmap(bg_trend_layer, bg_trend_bitmap);
			} 
			#ifdef DEBUG_LEVEL 
			else {
				APP_LOG(APP_LOG_LEVEL_INFO, "bg_trend_bitmap creation FAILED!");
			}
			#endif
			//free(trend_buffer);
			break;

		case CGM_MESSAGE_KEY:
			#ifdef DEBUG_LEVEL
			APP_LOG(APP_LOG_LEVEL_INFO, "Got Message Key, message is \"%s\"", data->value->cstring);
			#endif
			text_layer_set_text(message_layer,data->value->cstring);
			if(strcmp(data->value->cstring, "")==0) {
				#ifdef DEBUG_LEVEL
				APP_LOG(APP_LOG_LEVEL_INFO, "Setting Delta Only");
				#endif
				display_message = false;
			//	layer_set_hidden((Layer *)message_layer, true);
			//	layer_set_hidden((Layer *)delta_layer, false);
			} else {
				#ifdef DEBUG_LEVEL
				APP_LOG(APP_LOG_LEVEL_INFO, "Setting Message");
				#endif
				display_message = true;
			//	layer_set_hidden((Layer *)message_layer, false);
			//	layer_set_hidden((Layer *)delta_layer, true);
			}
			break;
			
		case CGM_VIBE_KEY:
			#ifdef DEBUG_LEVEL
			APP_LOG(APP_LOG_LEVEL_INFO, "Got Vibe Key, message is \"%u\"", data->value->uint8);
			#endif
			if(data->value->uint8 > 0 || data->value->uint8 <4) {
				alert_handler_cgm(data->value->uint8);
			}
			break;
							
		default:
			#ifdef DEBUG_LEVEL 
			APP_LOG(APP_LOG_LEVEL_INFO, "sync_tuple_cgm_callback: Dictionary Key not recognised: %s", data->value->cstring );
			#endif
			break;
		}	
		// end switch(key)
		data = dict_read_next(iterator);
	}
} // end sync_tuple_changed_callback_cgm()

static void send_cmd_cgm(void) {
	
	DictionaryIterator *iter = NULL;
	const uint32_t size = dict_calc_buffer_size(1,4);
	const uint32_t value = CGM_SYNC_KEY;
	uint8_t buffer[size];
	#ifdef DEBUG_LEVEL
		APP_LOG(APP_LOG_LEVEL_INFO, "send_cmd_cgm called.");
	#endif
	AppMessageResult sendcmd_openerr = APP_MSG_OK;
	AppMessageResult sendcmd_senderr = APP_MSG_OK;


	dict_write_begin(iter, buffer, sizeof(buffer));
	dict_write_int(iter, CGM_SYNC_KEY, &value ,4 ,false);	
	//APP_LOG(APP_LOG_LEVEL_INFO, "SEND CMD IN, ABOUT TO OPEN APP MSG OUTBOX");

	sendcmd_openerr = app_message_outbox_begin(&iter);
	
	//APP_LOG(APP_LOG_LEVEL_INFO, "SEND CMD, MSG OUTBOX OPEN, CHECK FOR ERROR");
	if (sendcmd_openerr != APP_MSG_OK) {
		#ifdef DEBUG_LEVEL
		APP_LOG(APP_LOG_LEVEL_INFO, "WATCH SENDCMD OPEN ERROR");
		APP_LOG(APP_LOG_LEVEL_DEBUG, "WATCH SENDCMD OPEN ERR CODE: %i RES: %s", sendcmd_openerr, translate_app_error(sendcmd_openerr));
		#endif
		return;
	}



	#ifdef DEBUG_LEVEL
	APP_LOG(APP_LOG_LEVEL_INFO, "SEND CMD, MSG OUTBOX OPEN, NO ERROR, ABOUT TO SEND MSG TO APP");
	#endif
	sendcmd_senderr = app_message_outbox_send();
	
	#ifdef DEBUG_LEVEL
	if (sendcmd_senderr != APP_MSG_OK && sendcmd_senderr != APP_MSG_BUSY && sendcmd_senderr != APP_MSG_SEND_REJECTED) {
		 APP_LOG(APP_LOG_LEVEL_INFO, "WATCH SENDCMD SEND ERROR");
		 APP_LOG(APP_LOG_LEVEL_DEBUG, "WATCH SENDCMD SEND ERR CODE: %i RES: %s", sendcmd_senderr, translate_app_error(sendcmd_senderr));
	}
	 #endif

	//APP_LOG(APP_LOG_LEVEL_INFO, "SEND CMD OUT, SENT MSG TO APP");
	
} // end send_cmd_cgm

void timer_callback_cgm(void *data) {

	//APP_LOG(APP_LOG_LEVEL_INFO, "TIMER CALLBACK IN, TIMER POP, ABOUT TO CALL SEND CMD");
	// reset msg timer to NULL
	if (timer_cgm != NULL) {
		timer_cgm = NULL;
	}
	//minutes_cgm tracks the minutes since the last update sent from the pebble.
	// if it reaches 0, we have not received an update in 6 minutes, so we ask for one.
	//This limits the number of messages the pebble sends, and therefore improves battery life.
	
	//APP_LOG(APP_LOG_LEVEL_INFO, "minutes_cgm: %d", minutes_cgm);
	if (minutes_cgm == 0) {
		minutes_cgm = 6;
		// send message
		#ifdef DEBUG_LEVEL
		APP_LOG(APP_LOG_LEVEL_INFO, "send_cmd_cgm: minutes_cgm is zero, setting to 6 and sending for data.");
		#endif
		send_cmd_cgm();
	} else {
		minutes_cgm--;
		load_cgmtime();
		load_bg_delta();
	}
	#ifdef DEBUG_LEVEL
	APP_LOG(APP_LOG_LEVEL_INFO, "minutes_cgm: %d", minutes_cgm);
	#endif
	//APP_LOG(APP_LOG_LEVEL_INFO, "TIMER CALLBACK, SEND CMD DONE, ABOUT TO REGISTER TIMER");
	// set msg timer
	timer_cgm = app_timer_register((WATCH_MSGSEND_SECS*MS_IN_A_SECOND), timer_callback_cgm, NULL);

	//APP_LOG(APP_LOG_LEVEL_INFO, "TIMER CALLBACK, REGISTER TIMER DONE");
	
} // end timer_callback_cgm

// format current time from watch

void handle_second_tick_cgm(struct tm* tick_time_cgm, TimeUnits units_changed_cgm) {
	//#ifdef DEBUG_LEVEL
	//APP_LOG(APP_LOG_LEVEL_INFO, "Handling minute tick");
	//#endif
	
	// VARIABLES
	size_t tick_return_cgm = 0;
	
	// CODE START
	if ((units_changed_cgm & SECOND_UNIT) && ((tick_time_cgm->tm_sec & 0x01)==1)) {
		
		#if DEBUG_LEVEL > 1
		APP_LOG(APP_LOG_LEVEL_INFO, "Handling 2 second tick, display_message is %i", display_message);
		#endif
		if(display_message){
			#if DEBUG_LEVEL > 1
			APP_LOG(APP_LOG_LEVEL_DEBUG, "message_layer toggling %i", (tick_time_cgm->tm_sec & 0x01)==1);
			#endif
			
		//	layer_set_hidden((Layer *)delta_layer, !(layer_get_hidden((Layer *)delta_layer)));
			layer_set_hidden((Layer *)message_layer, !(layer_get_hidden((Layer *)message_layer)));
	//	} else {
	//		if(!layer_get_hidden((Layer *)message_layer)) {
	//			layer_set_hidden((Layer *)message_layer, true);
	//		}
	//		if(layer_get_hidden((Layer *)delta_layer)) {
		//		layer_set_hidden((Layer *)delta_layer, false);
	//		}
		}
	}
	if (units_changed_cgm & MINUTE_UNIT) {
		//APP_LOG(APP_LOG_LEVEL_INFO, "TICK TIME MINUTE CODE");
	if(clock_is_24h_style() == true) {
		tick_return_cgm = strftime(time_watch_text, TIME_TEXTBUFF_SIZE, "%H:%M", tick_time_cgm);	
	} else {
		tick_return_cgm = strftime(time_watch_text, TIME_TEXTBUFF_SIZE, "%l:%M", tick_time_cgm);
	}
	if (tick_return_cgm != 0) {
			text_layer_set_text(time_watch_layer, time_watch_text);
	}
	
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "lastAlertTime IN:	%i", lastAlertTime);
	// increment BG snooze
		++lastAlertTime;
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "lastAlertTime OUT:	%i", lastAlertTime);
	
	} 
	else if (units_changed_cgm & DAY_UNIT) {
		//APP_LOG(APP_LOG_LEVEL_INFO, "TICK TIME DAY CODE");
		tick_return_cgm = strftime(date_app_text, DATE_TEXTBUFF_SIZE, "%a %d", tick_time_cgm);
		if (tick_return_cgm != 0) {
			text_layer_set_text(date_app_layer, date_app_text);
		}
	}
	
} // end handle_minute_tick_cgm

static void setColors() {
	textColor = COLOR_FALLBACK(GColorDukeBlue, GColorBlack) ;
  bgColor = textColor;
  dateColor = textColor ;
  timeColor = GColorWhite;
  upperColor = GColorClear;
  if ( displayFormat == 1 || displayFormat == 3 ) {
    watch_battery_fill_color  = COLOR_FALLBACK(GColorCyan, GColorWhite) ;
    watch_battery_low_color  = COLOR_FALLBACK(GColorRed, GColorWhite) ;
    watch_battery_border_color = GColorWhite ;
    bridge_battery_fill_color = COLOR_FALLBACK(GColorCyan, GColorWhite) ;
    bridge_battery_low_color = COLOR_FALLBACK(GColorRed, GColorWhite) ;
    bridge_battery_border_color = GColorWhite ;
    if ( displayFormat == 3 ) {
      dateColor = GColorWhite ;
    }
  } else if (displayFormat == 2 ) {
    watch_battery_fill_color  = COLOR_FALLBACK(GColorCyan, GColorBlack) ;
    watch_battery_low_color  = COLOR_FALLBACK(GColorRed, GColorBlack) ;
    watch_battery_border_color = COLOR_FALLBACK(GColorDukeBlue, GColorBlack) ;
    bridge_battery_fill_color = COLOR_FALLBACK(GColorCyan, GColorBlack) ;
    bridge_battery_low_color = COLOR_FALLBACK(GColorRed, GColorBlack) ;
    bridge_battery_border_color = COLOR_FALLBACK(GColorDukeBlue, GColorBlack) ;
  //  bgColor = GColorWhite;
  //  upperColor = COLOR_FALLBACK(GColorDukeBlue, GColorBlack) ;
  }
}

void setDisplay() {
  #ifdef PBL_BW
     	grectUpperLayer = GRect(0,0,144,83);
  		grectLowerLayer = GRect(0,84,144,165);
  		grectIconLayer = GRect(85, -7, 78, 51);
  		grectMessageLayer = GRect(0, 36, 143, 50);
      grectDeltaLayer = GRect(0, 36, 143, 50);
      grectTimeAgoLayer = GRect(52, 58, 40, 24);
      grectBGLayer = GRect(0, -5, 95, 47);
   		grectTimeLayer = GRect(0, 82, 143, 44);
  		grectDateLayer = GRect(0, 124, 143, 26);
  		grectWatchBatteryLayer = GRect(81, 148, 40, 20);
  		grectBridgeBatteryLayer = GRect(0, 148, 40, 20) ;
      dateColor = GColorWhite ;
  #else
    grectTrendLayer = GRect(centerPoint.x - 72, centerPoint.y - 42 , 144 , 84);
    switch (displayFormat) {
      case 1 :
        grectUpperLayer = GRect(0,0,grectWindow.size.w, centerPoint.y - 42 );
        grectLowerLayer = GRect(0,centerPoint.y + 42 ,grectWindow.size.w, centerPoint.y - 42 );
        grectIconLayer = GRect(grectWindow.size.w - 144, centerPoint.y - 42 , 60 , 60);
        grectMessageLayer = GRect(0, centerPoint.y - 20, grectWindow.size.w, 50);
        grectDeltaLayer = GRect(0 , centerPoint.y - 89 , third_w , 25);
        grectTimeAgoLayer = GRect(0, centerPoint.y - 70 , third_w , 25);
        grectBGLayer = GRect(third_w , centerPoint.y - 89 ,third_w * 2 , 50);
        grectTimeLayer = GRect(0 , centerPoint.y + 35 , grectWindow.size.w , 50); 
        grectDateLayer = GRect(0 , centerPoint.y + 15 , grectWindow.size.w , 30);
        grectWatchBatteryLayer = GRect(0, centerPoint.y + 60 , third_w , 25);
        grectBridgeBatteryLayer = GRect(0 , centerPoint.y + 40 , third_w , 25) ;
        alignIcon = GAlignTopLeft ;
        alignTime = GTextAlignmentRight;
        break;
      case 2:
        grectUpperLayer = GRect(0,0,grectWindow.size.w, centerPoint.y - 42 );
        grectLowerLayer = GRect(0,centerPoint.y + 42 ,grectWindow.size.w, centerPoint.y - 42 );
        grectIconLayer = GRect(0, centerPoint.y - 42 , grectWindow.size.w , 60);
        grectMessageLayer = GRect(0, centerPoint.y - 22, grectWindow.size.w, 50);
        grectBGLayer = GRect(0 , centerPoint.y - 89 , grectWindow.size.w , 50);
        #ifdef PBL_ROUND 
          grectDeltaLayer = GRect(centerPoint.x - 78 , centerPoint.y - 55 , 60 , 25);
          grectTimeAgoLayer = GRect(centerPoint.x + 17, centerPoint.y - 55  , 60, 25);
          grectWatchBatteryLayer = GRect(centerPoint.x - 80, centerPoint.y + 22 , 40, 20);
          grectBridgeBatteryLayer = GRect(centerPoint.x + 42 , centerPoint.y + 22 , 40, 20) ;
        #else
          grectDeltaLayer = GRect(centerPoint.x - 72 , centerPoint.y - 55 , 60 , 25);
          grectTimeAgoLayer = GRect(centerPoint.x + 12, centerPoint.y - 55  , 60, 25);
          grectWatchBatteryLayer = GRect(centerPoint.x - 72, centerPoint.y + 22 , 40, 20);
          grectBridgeBatteryLayer = GRect(centerPoint.x + 35 , centerPoint.y + 22 , 40, 20) ;
        #endif
        grectTimeLayer = GRect(0 , centerPoint.y + 35 , grectWindow.size.w , 50);
        grectDateLayer = GRect(0 , centerPoint.y + 15 , grectWindow.size.w , 30);
        alignTimeAgo = GTextAlignmentRight;
        alignDelta = GTextAlignmentLeft;
        break;
      case 3:
        grectUpperLayer = GRect(0,0,grectWindow.size.w, centerPoint.y - 42 );
        grectLowerLayer = GRect(0,centerPoint.y + 42 ,grectWindow.size.w, centerPoint.y - 42 );
        grectIconLayer = GRect(grectWindow.size.w - 144, centerPoint.y - 42 , 60 , 60);
        grectMessageLayer = GRect(0, centerPoint.y - 20, grectWindow.size.w, 50);
        grectDeltaLayer = GRect(0 , centerPoint.y - 89 , third_w , 25);
        grectTimeAgoLayer = GRect(0, centerPoint.y - 70 , third_w , 25);
        grectBGLayer = GRect(third_w , centerPoint.y - 89 ,third_w * 2 , 50);
        grectDateLayer = GRect(third_w , centerPoint.y + 34 , third_w * 2 , 25);
        grectTimeLayer = GRect(third_w , centerPoint.y + 53 , third_w * 2 , 50);
        if (battery_graphic) {
          grectWatchBatteryLayer = GRect(5, centerPoint.y + 60 , third_w - 5, 25);
          grectBridgeBatteryLayer = GRect(5 , centerPoint.y + 40 , third_w - 5, 25) ;
        } else {
          grectWatchBatteryLayer = GRect(0, centerPoint.y + 60 , third_w , 25);
          grectBridgeBatteryLayer = GRect(0 , centerPoint.y + 40 , third_w , 25) ;
        }
        break;
        alignIcon = GAlignTopLeft ;
    }
  #endif
}

// battery_handler - updates the pebble battery percentage.
static void watch_battery_handler(BatteryChargeState charge_state) {
  battery_changing = charge_state.is_charging ;
  battery_pcnt = charge_state.charge_percent;
 
  if (battery_graphic) {
    layer_mark_dirty(watch_battery_draw_layer);
  } else {
    setColors();
    static char watch_battlevel_percent[9];
    snprintf(watch_battlevel_percent, BATTLEVEL_FORMATTED_SIZE, "W:%i", battery_pcnt);
    if(battery_changing) {
  		text_layer_set_text_color(watch_battery_text_layer, watch_battery_fill_color);
  		text_layer_set_background_color(watch_battery_text_layer, watch_battery_border_color);
   	} else {
      text_layer_set_text_color(watch_battery_text_layer, watch_battery_border_color);
  		#ifdef PBL_COLOR
    		if(battery_pcnt < 20 ) {
       			text_layer_set_text_color(watch_battery_text_layer, watch_battery_low_color);
      		}
      #endif
 		  text_layer_set_background_color(watch_battery_text_layer, GColorClear);
  	}	
    text_layer_set_text(watch_battery_text_layer, watch_battlevel_percent);
  }
} // end watch_battery_handler

static void draw_battery(GContext *ctx, GColor borderColor, GColor fillColor, GColor lowColor, int battPcnt, bool charging) {
  // draw battery
  graphics_context_set_stroke_color(ctx, borderColor );
  graphics_draw_round_rect(ctx, GRect(0, 0, 22, 12),1);
  
  graphics_context_set_fill_color(ctx, borderColor);
  graphics_fill_rect(ctx, GRect(22 , 3, 2, 6), 0, GCornerNone);
  
  // fill battery
  int width = (int)(float)(((float)battPcnt / 100.0F) * 20.0F) ;
  if ( battPcnt <= 20 ) { 
    graphics_context_set_fill_color(ctx, lowColor); 
  } else {
    graphics_context_set_fill_color(ctx, fillColor);
  }
  graphics_fill_rect(ctx, GRect(1 , 1, width, 10), 0, GCornerNone);
      
  if ( charging ) {
    graphics_context_set_stroke_color(ctx, lowColor);
    graphics_draw_line(ctx, GPoint(12, 0), GPoint(10, 6));
    graphics_draw_line(ctx, GPoint(12, 0), GPoint(9, 6));
    graphics_draw_line(ctx, GPoint(9, 5), GPoint(13, 5));
    graphics_draw_line(ctx, GPoint(9, 6), GPoint(13, 6));
    graphics_draw_line(ctx, GPoint(12, 6), GPoint(9, 12));
    graphics_draw_line(ctx, GPoint(11, 6), GPoint(9, 12));
  }
}

static void watch_battery_proc(Layer *layer, GContext *ctx) {
  setColors();
  draw_battery(ctx, watch_battery_border_color, watch_battery_fill_color, watch_battery_low_color, battery_pcnt, battery_changing );
}

static void bridge_battery_proc(Layer *layer, GContext *ctx) {
  setColors();
  draw_battery(ctx, bridge_battery_border_color, bridge_battery_fill_color, bridge_battery_low_color, atoi(last_battlevel), false );
}

void setFonts() {
  bgFont = fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD) ;
  midFont = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD) ;
  smallFont = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD) ;
  messageFont = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD) ;
  if ( displayFormat == 3 ) {
    timeFont = fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK) ;
  } else {
    timeFont = fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD) ;
  }
 
}

void window_load_cgm(Window *window_cgm) {
	//APP_LOG(APP_LOG_LEVEL_INFO, "WINDOW LOAD");
	
	// VARIABLES
	Layer *window_layer_cgm = NULL;
  
  // CODE START

	window_layer_cgm = window_get_root_layer(window_cgm);

	//Paint the backgrounds for upper and lower halves of the watch face.
	#ifdef DEBUG_LEVEL
	APP_LOG(APP_LOG_LEVEL_INFO, "Creating Upper and Lower face panels");
	#endif
  
	window_set_background_color(window_cgm, GColorWhite);

	grectWindow = layer_get_bounds(window_layer_cgm);
	centerPoint = grect_center_point(&grectWindow);
	third_w = grectWindow.size.w / 3; 
	
	setFonts();
  setDisplay();	
	setColors();
  
	upper_layer = text_layer_create(grectUpperLayer);
	text_layer_set_background_color(upper_layer, upperColor );
	layer_add_child(window_layer_cgm, text_layer_get_layer(upper_layer));

	lower_layer = text_layer_create(grectLowerLayer);
	text_layer_set_background_color(lower_layer, textColor );
	layer_add_child(window_layer_cgm, text_layer_get_layer(lower_layer));

	// ARROW OR SPECIAL VALUE
	#ifdef DEBUG_LEVEL
	  APP_LOG(APP_LOG_LEVEL_INFO, "Creating Arrow Bitmap layer");
	#endif
	icon_layer = bitmap_layer_create(grectIconLayer);
	#ifdef PBL_COLOR
	  bitmap_layer_set_compositing_mode(icon_layer, GCompOpSet);
	#endif
	bitmap_layer_set_alignment(icon_layer, alignIcon);
	bitmap_layer_set_background_color(icon_layer, GColorClear);
	layer_add_child(window_layer_cgm, bitmap_layer_get_layer(icon_layer));

	// MESSAGE
	#ifdef DEBUG_LEVEL
	APP_LOG(APP_LOG_LEVEL_INFO, "Creating Message Text layer");
	#endif
	message_layer = text_layer_create(grectMessageLayer);
	text_layer_set_text_color(message_layer, textColor);
	text_layer_set_background_color(message_layer, GColorClear);
	text_layer_set_font(message_layer, messageFont);
	text_layer_set_text_alignment(message_layer, GTextAlignmentCenter);
	layer_set_hidden((Layer *)message_layer, false);
	layer_add_child(window_layer_cgm, text_layer_get_layer(message_layer));

	// DELTA BG
	#ifdef DEBUG_LEVEL
	APP_LOG(APP_LOG_LEVEL_INFO, "Creating Delta BG Text layer");
	#endif
  delta_layer = text_layer_create(grectDeltaLayer );
	text_layer_set_text(delta_layer, "+0.00");
	text_layer_set_text_color(delta_layer, textColor);
	text_layer_set_background_color(delta_layer, GColorClear);
	text_layer_set_font(delta_layer, midFont);
	text_layer_set_text_alignment(delta_layer, alignDelta);
	layer_add_child(window_layer_cgm, text_layer_get_layer(delta_layer));

	// CGM TIME AGO READING
	#ifdef DEBUG_LEVEL
	APP_LOG(APP_LOG_LEVEL_INFO, "Creating CGM Time Ago Bitmap layer");
	#endif
  cgmtime_layer = text_layer_create(grectTimeAgoLayer);
	text_layer_set_text(cgmtime_layer, "now");
	text_layer_set_text_color(cgmtime_layer, textColor);                         
	text_layer_set_background_color(cgmtime_layer, GColorClear);
	text_layer_set_font(cgmtime_layer, midFont);
	text_layer_set_text_alignment(cgmtime_layer, alignTimeAgo);
	layer_add_child(window_layer_cgm, text_layer_get_layer(cgmtime_layer));

	// BG
	#ifdef DEBUG_LEVEL
	APP_LOG(APP_LOG_LEVEL_INFO, "Creating BG Text layer");
	#endif
  bg_layer = text_layer_create(grectBGLayer);
	text_layer_set_text_color(bg_layer, bgColor);
	text_layer_set_background_color(bg_layer, GColorClear);
	text_layer_set_font(bg_layer, bgFont);
	text_layer_set_text_alignment(bg_layer, alignBG);
	layer_add_child(window_layer_cgm, text_layer_get_layer(bg_layer));
	
	// CURRENT ACTUAL TIME FROM WATCH
	#ifdef DEBUG_LEVEL
	APP_LOG(APP_LOG_LEVEL_INFO, "Creating Watch Time Text layer");
	#endif
	time_watch_layer = text_layer_create(grectTimeLayer);
	text_layer_set_text_color(time_watch_layer, timeColor);
	text_layer_set_background_color(time_watch_layer, GColorClear);
	text_layer_set_font(time_watch_layer, timeFont);
	text_layer_set_text_alignment(time_watch_layer, alignTime);
	layer_add_child(window_layer_cgm, text_layer_get_layer(time_watch_layer));
	
	// CURRENT ACTUAL DATE FROM APP
	#ifdef DEBUG_LEVEL
	APP_LOG(APP_LOG_LEVEL_INFO, "Creating Watch Date Text layer");
	#endif
	date_app_layer = text_layer_create(grectDateLayer);
	text_layer_set_text_color(date_app_layer, dateColor);
	text_layer_set_background_color(date_app_layer, GColorClear);
	text_layer_set_font(date_app_layer, midFont);
	text_layer_set_text_alignment(date_app_layer, alignDate);
	draw_date_from_app();
	layer_add_child(window_layer_cgm, text_layer_get_layer(date_app_layer));

	// BRIDGE BATTERY LEVEL
	#ifdef DEBUG_LEVEL
	APP_LOG(APP_LOG_LEVEL_INFO, "Creating Phone Battery Text layer");
	#endif
    
	bridge_battery_text_layer = text_layer_create(grectBridgeBatteryLayer);
	text_layer_set_font(bridge_battery_text_layer, smallFont);
	text_layer_set_text_alignment(bridge_battery_text_layer, GTextAlignmentLeft);
	text_layer_set_background_color(bridge_battery_text_layer, GColorClear );
	text_layer_set_text_color(bridge_battery_text_layer, bridge_battery_border_color );
	text_layer_set_text(bridge_battery_text_layer, "B");
	layer_add_child(window_layer_cgm, text_layer_get_layer(bridge_battery_text_layer));
  
	if (battery_graphic) {
		grectImageBounds = grect_inset(grectBridgeBatteryLayer, GEdgeInsets(7,0,0,12));

		bridge_battery_layer = text_layer_create(grectImageBounds);
		text_layer_set_background_color(bridge_battery_layer, GColorClear );
		layer_add_child(window_layer_cgm, text_layer_get_layer(bridge_battery_layer));
	  
		bridge_battery_draw_layer = layer_create(grectImageBounds);
		layer_set_update_proc(bridge_battery_draw_layer, bridge_battery_proc);
		layer_add_child(window_layer_cgm, bridge_battery_draw_layer);	
	}
	#ifdef DEBUG_LEVEL
	//	APP_LOG(APP_LOG_LEVEL_INFO, "battlevel_layer; %s", text_layer_get_text(battlevel_layer));
	#endif
	

	// WATCH BATTERY LEVEL
	#ifdef DEBUG_LEVEL
	APP_LOG(APP_LOG_LEVEL_INFO, "Creating Watch Battery Text layer");
	#endif
	BatteryChargeState charge_state = battery_state_service_peek();
  

	watch_battery_text_layer = text_layer_create(grectWatchBatteryLayer);
	text_layer_set_font(watch_battery_text_layer, smallFont);
	text_layer_set_text_alignment(watch_battery_text_layer, GTextAlignmentLeft);
	text_layer_set_background_color(watch_battery_text_layer, GColorClear );
	text_layer_set_text_color(watch_battery_text_layer, watch_battery_border_color );
	text_layer_set_text(watch_battery_text_layer, "W");
	layer_add_child(window_layer_cgm, text_layer_get_layer(watch_battery_text_layer));

	if (battery_graphic) {
		grectImageBounds = grect_inset(grectWatchBatteryLayer, GEdgeInsets(7,0,0, 12));

		watch_battery_layer = text_layer_create(grectImageBounds);
		text_layer_set_background_color(watch_battery_layer, GColorClear );
		layer_add_child(window_layer_cgm, text_layer_get_layer(watch_battery_layer));
	  
		watch_battery_draw_layer = layer_create(grectImageBounds);
	  
		layer_set_update_proc(watch_battery_draw_layer, watch_battery_proc);
		layer_add_child(window_layer_cgm, watch_battery_draw_layer);
	}
 
	//subscribe to the battery handler
	battery_state_service_subscribe(watch_battery_handler);

	watch_battery_handler(charge_state);

  
  
	#ifdef DEBUG_LEVEL
//	APP_LOG(APP_LOG_LEVEL_INFO, "watch_battlevel_layer; %s", text_layer_get_text(watch_battlevel_layer));
	#endif

  //create the bg_trend_layer
	#ifdef TRENDING
	#ifdef DEBUG_LEVEL
	APP_LOG(APP_LOG_LEVEL_INFO, "Creating BG Trend Bitmap layer");
	#endif
  // bitmap is 144,84
	bg_trend_layer = bitmap_layer_create(grectTrendLayer ); 
  if ( bg_trend_layer == NULL) {
    APP_LOG(APP_LOG_LEVEL_INFO, "bg_trend_layer no alloc");
  }
	// bg_trend_layer = bitmap_layer_create(GRect(0,26,144,84));
	#ifdef PBL_PLATFORM_BASALT
	bitmap_layer_set_compositing_mode(bg_trend_layer, GCompOpSet);
	#endif
	#ifdef PBL_PLATFORM_APLITE
	bitmap_layer_set_compositing_mode(bg_trend_layer, GCompOpAnd);
	#endif
	bitmap_layer_set_background_color(bg_trend_layer, GColorClear);
	layer_add_child(window_layer_cgm, bitmap_layer_get_layer(bg_trend_layer));
	#endif

	// put " " (space) in bg field so logo continues to show
	// " " (space) also shows these are init values, not bad or null values
	snprintf(current_icon, 1, " ");
	load_icon();
	snprintf(last_bg, BG_MSGSTR_SIZE, " ");
	load_bg();
	current_cgm_time = 0;
	load_cgmtime();
	current_app_time = 0;
	//load_apptime();		
	snprintf(current_bg_delta, BGDELTA_MSGSTR_SIZE, "LOAD");
	load_bg_delta();
	snprintf(last_battlevel, BATTLEVEL_MSGSTR_SIZE, " ");
	load_battlevel();

	//APP_LOG(APP_LOG_LEVEL_INFO, "WINDOW LOAD, ABOUT TO CALL APP SYNC INIT");
	//app_sync_init(&sync_cgm, sync_buffer_cgm, sizeof(sync_buffer_cgm), initial_values_cgm, ARRAY_LENGTH(initial_values_cgm), sync_tuple_changed_callback_cgm, sync_error_callback_cgm, NULL);
	// init timer to null if needed, and register timer
	//APP_LOG(APP_LOG_LEVEL_INFO, "WINDOW LOAD, APP INIT DONE, ABOUT TO REGISTER TIMER");	
	if (timer_cgm != NULL) {
		timer_cgm = NULL;
	}
	timer_cgm = app_timer_register((LOADING_MSGSEND_SECS*MS_IN_A_SECOND), timer_callback_cgm, NULL);
	//APP_LOG(APP_LOG_LEVEL_INFO, "WINDOW LOAD, TIMER REGISTER DONE");
	
} // end window_load_cgm

void window_unload_cgm(Window *window_cgm) {
	//APP_LOG(APP_LOG_LEVEL_INFO, "WINDOW UNLOAD IN");
	
	//APP_LOG(APP_LOG_LEVEL_INFO, "WINDOW UNLOAD, APP SYNC DEINIT");
	app_sync_deinit(&sync_cgm);

	//destroy the trend bitmap and layer
	destroy_null_GBitmap(&bg_trend_bitmap);
	destroy_null_BitmapLayer(&bg_trend_layer);
	//APP_LOG(APP_LOG_LEVEL_INFO, "WINDOW UNLOAD, DESTROY GBITMAPS IF EXIST");
	destroy_null_GBitmap(&icon_bitmap);
	destroy_null_GBitmap(&appicon_bitmap);
	//destroy_null_GBitmap(&cgmicon_bitmap);
	destroy_null_GBitmap(&specialvalue_bitmap);
	//destroy_null_GBitmap(&batticon_bitmap);

	//APP_LOG(APP_LOG_LEVEL_INFO, "WINDOW UNLOAD, DESTROY BITMAPS IF EXIST");	
	destroy_null_BitmapLayer(&icon_layer);
	//destroy_null_BitmapLayer(&cgmicon_layer);
	//destroy_null_BitmapLayer(&appicon_layer);
	//destroy_null_BitmapLayer(&batticon_layer);

	//APP_LOG(APP_LOG_LEVEL_INFO, "WINDOW UNLOAD, DESTROY TEXT LAYERS IF EXIST");	
	destroy_null_TextLayer(&bg_layer);
	destroy_null_TextLayer(&cgmtime_layer);
	destroy_null_TextLayer(&delta_layer);
	destroy_null_TextLayer(&message_layer);
	destroy_null_TextLayer(&battlevel_layer);
	destroy_null_TextLayer(&watch_battery_text_layer);
	destroy_null_TextLayer(&bridge_battery_text_layer);
  if (battery_graphic) {
  	destroy_null_TextLayer(&watch_battery_layer);
  	destroy_null_TextLayer(&bridge_battery_layer);
	  layer_destroy(bridge_battery_draw_layer);
    layer_destroy(watch_battery_draw_layer);
  }
	//destroy_null_TextLayer(&watch_battlevel_layer);
	//destroy_null_TextLayer(&t1dname_layer);
	destroy_null_TextLayer(&time_watch_layer);
	//destroy_null_TextLayer(&time_app_layer);
	destroy_null_TextLayer(&date_app_layer);

	//destroy the face background layers.
	destroy_null_BitmapLayer(&lower_face_layer);
	destroy_null_BitmapLayer(&upper_face_layer);
	//APP_LOG(APP_LOG_LEVEL_INFO, "WINDOW UNLOAD, DESTROY INVERTER LAYERS IF EXIST");	
	//destroy_null_InverterLayer(&inv_battlevel_layer);
	
	//APP_LOG(APP_LOG_LEVEL_INFO, "WINDOW UNLOAD OUT");
} // end window_unload_cgm

static void init_cgm(void) {
	//APP_LOG(APP_LOG_LEVEL_INFO, "INIT CODE IN");

	// subscribe to the tick timer service
	tick_timer_service_subscribe(SECOND_UNIT, &handle_second_tick_cgm);

	// subscribe to the bluetooth connection service
	bluetooth_connection_service_subscribe(handle_bluetooth_cgm);
	
	// init the window pointer to NULL if it needs it
	if (window_cgm != NULL) {
		window_cgm = NULL;
	}
  
	// create the windows
	window_cgm = window_create();
	window_set_background_color(window_cgm, GColorBlack);
	window_set_window_handlers(window_cgm, (WindowHandlers) {
		.load = window_load_cgm,
		.unload = window_unload_cgm	
	});

	//APP_LOG(APP_LOG_LEVEL_INFO, "INIT CODE, REGISTER APP MESSAGE ERROR HANDLERS"); 

	app_message_register_inbox_dropped(inbox_dropped_handler_cgm);
	app_message_register_outbox_failed(outbox_failed_handler_cgm);

	app_message_register_inbox_received(inbox_received_handler_cgm);
	
//	APP_LOG(APP_LOG_LEVEL_INFO, "INIT CODE, ABOUT TO CALL APP MSG OPEN"); 
#ifdef PBL_PLATFORM_APLITE
	app_message_open(2048, 2048);
#else
	app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
#endif
//APP_LOG(APP_LOG_LEVEL_INFO, "INIT CODE, APP MSG OPEN DONE");

	
	const bool animated_cgm = true;
	window_stack_push(window_cgm, animated_cgm);
	
	//APP_LOG(APP_LOG_LEVEL_INFO, "INIT CODE OUT"); 
}	// end init_cgm

static void deinit_cgm(void) {
	//APP_LOG(APP_LOG_LEVEL_INFO, "DEINIT CODE IN");
	
	// unsubscribe to the tick timer service
	//APP_LOG(APP_LOG_LEVEL_INFO, "DEINIT, UNSUBSCRIBE TICK TIMER");
	tick_timer_service_unsubscribe();

	// unsubscribe to the bluetooth connection service
	//APP_LOG(APP_LOG_LEVEL_INFO, "DEINIT, UNSUBSCRIBE BLUETOOTH");
	bluetooth_connection_service_unsubscribe();

	battery_state_service_unsubscribe();
	
	// cancel timers if they exist
	//APP_LOG(APP_LOG_LEVEL_INFO, "DEINIT, CANCEL APP TIMER");
	if (timer_cgm != NULL) {
		app_timer_cancel(timer_cgm);
		timer_cgm = NULL;
	}
	
	//APP_LOG(APP_LOG_LEVEL_INFO, "DEINIT, CANCEL BLUETOOTH TIMER");
	if (BT_timer != NULL) {
		app_timer_cancel(BT_timer);
		BT_timer = NULL;
	}
	
	// destroy the window if it exists
	//APP_LOG(APP_LOG_LEVEL_INFO, "DEINIT, CHECK WINDOW POINTER FOR DESTROY");
	if (window_cgm != NULL) {
		//APP_LOG(APP_LOG_LEVEL_INFO, "DEINIT, WINDOW POINTER NOT NULL, DESTROY");
		window_destroy(window_cgm);
	}
	//APP_LOG(APP_LOG_LEVEL_INFO, "DEINIT, CHECK WINDOW POINTER FOR NULL");
	if (window_cgm != NULL) {
		//APP_LOG(APP_LOG_LEVEL_INFO, "DEINIT, WINDOW POINTER NOT NULL, SET TO NULL");
		window_cgm = NULL;
	}
	
	//APP_LOG(APP_LOG_LEVEL_INFO, "DEINIT CODE OUT");
} // end deinit_cgm

int main(void) {

	init_cgm();
	app_event_loop();
	deinit_cgm();
	
} // end main

