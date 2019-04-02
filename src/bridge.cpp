#include <QJsonArray>

#include <spdlog/spdlog.h>

#include "bridge.h"

Bridge::Bridge(QObject* parent)
	: QObject(parent) {
}

void Bridge::ListNotifications() {
	QJsonObject object;

	object.insert("type", "list_notifications");

	emit EmitMessage(object);
}

void Bridge::DismissNotification(const QString& key) {
	if (!key.isEmpty()) {
		QJsonObject object;

		object.insert("type", "dismiss_notification");
		object.insert("key", key);

		emit EmitMessage(object);
	}
}

void Bridge::ListSMS() {
	QJsonObject object;

	object.insert("type", "list_sms");

	emit EmitMessage(object);
}

void Bridge::ListSMSFrom(const QString& number, int page) {
	if (!number.isEmpty()) {
		QJsonObject object;

		object.insert("type", "list_sms_from");
		object.insert("number", number);

		if (page >= 0) {
			object.insert("page", page);
		}

		emit EmitMessage(object);
	}
}

void Bridge::SendSMS(const QString& destination, const QString& body) {
	if (!destination.isEmpty() && !body.isEmpty()) {
		QJsonObject object;

		object.insert("type", "send_sms");
		object.insert("destination", destination);
		object.insert("body", body);

		emit EmitMessage(object);
	}
}

void Bridge::ParseMessage(const QString& type, const QJsonObject& json) {
	if (type == "notification_received") {
		QJsonObject jsonNotification = json.value("notification").toObject();

		if (jsonNotification.contains("key")
				&& jsonNotification.contains("app_name")
				&& jsonNotification.contains("title")) {
			Notification notification = {
				jsonNotification.value("persistent").toBool(false), /* persistent */
				jsonNotification.value("key").toString(), /* key */
				jsonNotification.value("app_name").toString(), /* appName */
				jsonNotification.value("title").toString(), /* title */
				jsonNotification.value("text").toString() /* text */
			};

			if (!notification.key.isEmpty()
					&& !notification.appName.isEmpty()
					&& !notification.title.isEmpty()) {
				emit NotificationReceived(notification);
			}
		}
	} else if (type == "notification_removed") {
		QJsonObject jsonNotification = json.value("notification").toObject();

		if (jsonNotification.contains("key")
				&& jsonNotification.contains("app_name")
				&& jsonNotification.contains("title")) {
			Notification notification = {
				jsonNotification.value("persistent").toBool(false), /* persistent */
				jsonNotification.value("key").toString(), /* key */
				jsonNotification.value("app_name").toString(), /* appName */
				jsonNotification.value("title").toString(), /* title */
				jsonNotification.value("text").toString() /* text */
			};

			if (!notification.key.isEmpty()
					&& !notification.appName.isEmpty()
					&& !notification.title.isEmpty()) {
				emit NotificationRemoved(notification);
			}
		}
	} else if (type == "notification_list") {
		QJsonArray jsonList = json.value("list").toArray();

		std::list<Notification> list;

		auto iterator = jsonList.begin();

		while (iterator != jsonList.end()) {
			QJsonObject jsonNotification = iterator->toObject();

			if (jsonNotification.contains("key")
					&& jsonNotification.contains("app_name")
					&& jsonNotification.contains("title")) {
				Notification notification = {
					jsonNotification.value("persistent").toBool(false), /* persistent */
					jsonNotification.value("key").toString(), /* key */
					jsonNotification.value("app_name").toString(), /* appName */
					jsonNotification.value("title").toString(), /* title */
					jsonNotification.value("text").toString() /* text */
				};

				if (!notification.key.isEmpty()
						&& !notification.appName.isEmpty()
						&& !notification.title.isEmpty()) {
					list.push_back(notification);
				}
			}

			++iterator;
		}

		emit NotificationList(list);
	} else if (type == "sms_list") {
		QJsonArray jsonList = json.value("list").toArray();

		std::list<SMS> list;

		auto iterator = jsonList.begin();

		while (iterator != jsonList.end()) {
			QJsonObject jsonSms = iterator->toObject();

			if (jsonSms.contains("type")
					&& jsonSms.contains("date")
					&& jsonSms.contains("body")
					&& jsonSms.contains("number")) {
				SMS sms = {
					(jsonSms.value("type").toString() == "in"), /* incoming */
					QDateTime::fromString(jsonSms.value("date").toString(),
										  DATE_FORMAT), /* date */
					jsonSms.value("body").toString(), /* body */
					jsonSms.value("number").toString(), /* number */
					jsonSms.value("name").toString() /* name */
				};

				if (sms.date.isValid() && !sms.number.isEmpty()) {
					list.push_back(sms);
				}
			}

			++iterator;
		}

		emit SMSList(list);
	} else if (type == "sms_from_list") {
		QString number = json.value("number").toString();
		int page = json.value("page").toInt();

		if (!number.isEmpty()) {
			QString name = json.value("name").toString();

			QJsonArray jsonList = json.value("list").toArray();

			std::list<ShortSMS> list;

			auto iterator = jsonList.begin();

			while (iterator != jsonList.end()) {
				QJsonObject jsonSms = iterator->toObject();

				if (jsonSms.contains("type")
						&& jsonSms.contains("date")
						&& jsonSms.contains("body")) {
					ShortSMS shortSms = {
						(jsonSms.value("type").toString() == "in"), /* incoming */
						QDateTime::fromString(jsonSms.value("date").toString(),
											  DATE_FORMAT), /* date */
						jsonSms.value("body").toString() /* body */
					};

					if (shortSms.date.isValid()) {
						list.push_back(shortSms);
					}
				}

				++iterator;
			}

			emit SMSFromList(number, name, page, list);
		}
	} else if (type == "sms_sent") {
		QString number = json.value("number").toString();
		bool success = json.value("success").toBool();

		QJsonObject jsonSms = json.value("sms").toObject();

		if (!number.isEmpty()
				&& jsonSms.contains("type")
				&& jsonSms.contains("date")
				&& jsonSms.contains("body")) {
			ShortSMS shortSms = {
				(jsonSms.value("type").toString() == "in"), /* incoming */
				QDateTime::fromString(jsonSms.value("date").toString(),
									  DATE_FORMAT), /* date */
				jsonSms.value("body").toString() /* body */
			};

			emit SMSSent(number, success, shortSms);
		}
	} else {
		spdlog::warn(std::string("Unkown message type: ") + type.toStdString());
	}
}
