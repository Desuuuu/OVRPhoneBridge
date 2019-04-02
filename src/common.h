#ifndef COMMON_H
#define COMMON_H

#include <locale>
#include <stdint.h>
#include <algorithm>

#include <QString>
#include <QDateTime>

#define APP_ORG                     "desuuuu"
#define APP_NAME                    "OVRPhoneBridge"

#define MANIFEST_PATH               "ovr_phone_bridge.vrmanifest"
#define MANIFEST_KEY                "desuuuu.ovr_phone_bridge"

#define OVERLAY_NAME                "Phone Bridge"
#define OVERLAY_THUMB_PATH          "overlay_icon.png"

#define LOG_MAX_SIZE                (5 * 1024 * 1024)
#define LOG_MAX_FILES               3

#define SETTINGS_PATH               "settings.ini"
#define DEFAULT_PORT                8888
#define DEFAULT_AUTOSTART           true
#define KICK_BANTIME                300U

#define NOTIF_TITLE_MAX_LENGTH      40
#define NOTIF_TEXT_MAX_LENGTH       250
#define SMS_PREVIEW_MAX_LENGTH      45
#define SMS_MAX_PAGE                8
#define SMS_MAX_LENGTH              150

#define ENCRYPTION_KEY_MIN_LENGTH   6
#define ENCRYPTION_KEY_SALT         "OVRPhoneBridge"
#define TIMESTAMP_LEEWAY            300U

#define SCROLL_SPEED                30.0f

#define DATE_FORMAT                 "yyyy-MM-dd'T'HH:mm:ss'Z'"

#define KEYBOARD_INPUT_PORT         0
#define KEYBOARD_INPUT_PASSWORD     1
#define KEYBOARD_INPUT_SMS          2

//Prevents placement issues with fully opaque QGraphicsOpacityEffect
#define OPACITY_EFFECT_MAX          0.9999

enum class ServerState : uint8_t {
	NONE = 0,
	STOPPED,
	STARTED,
	CONNECTED
};

enum class Tab : uint8_t {
	NONE = 0,
	NOTIFICATIONS,
	SMS,
	DEVICE,
	SETTINGS
};

struct ShortSMS {
	bool incoming;
	QDateTime date;
	QString body;
};

struct SMS {
	bool incoming;
	QDateTime date;
	QString body;
	QString number;
	QString name;
};

struct Notification {
	bool persistent;
	QString key;
	QString appName;
	QString title;
	QString text;
};

inline bool operator==(const Notification& lhs, const Notification& rhs) {
	return (lhs.key == rhs.key);
}

inline bool operator<(const Notification& lhs, const Notification& rhs) {
	return (lhs.key < rhs.key);
}

inline std::string trim(const std::string& str) {
	std::string result = str;

	result.erase(result.begin(), std::find_if(result.begin(), result.end(), [](char c) {
		return !std::isspace(c);
	}));

	result.erase(std::find_if(result.rbegin(), result.rend(), [](char c) {
		return !std::isspace(c);
	}).base(), result.end());

	return result;
}

inline QString ellipsize(const QString& str, int maxLen) {
	if (str.isEmpty() || str.length() <= maxLen) {
		return str;
	}

	return str.left(maxLen - 1) + "…";
}

#endif /* COMMON_H */
