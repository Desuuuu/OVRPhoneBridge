#ifndef BRIDGE_H
#define BRIDGE_H

#include <QObject>
#include <QJsonObject>

#include "common.h"

class Bridge : public QObject {
		Q_OBJECT

	public:
		Bridge(QObject* parent = nullptr);

	public slots:
		void ParseMessage(const QString& type, const QJsonObject& json);

		void ListNotifications();
		void DismissNotification(const QString& key);
		void ListSMS();
		void ListSMSFrom(const QString& number, int page = 0);
		void SendSMS(const QString& destination, const QString& body);

	signals:
		void EmitMessage(const QJsonObject& json);

		void NotificationReceived(const Notification& notification);
		void NotificationRemoved(const Notification& notification);
		void NotificationList(const std::list<Notification>& list);
		void SMSList(const std::list<SMS>& list);
		void SMSFromList(const QString& number,
						 const QString& name,
						 int page,
						 const std::list<ShortSMS>& list);
		void SMSSent(const QString& number, bool success, const ShortSMS& shortSms);
};

#endif /* BRIDGE_H */
