#ifndef NOTIFICATIONS_TAB_WIDGET_H
#define NOTIFICATIONS_TAB_WIDGET_H

#include <QWidget>

#include "../../common.h"

#define LOAD_DELAY  500

namespace Ui {
	class NotificationsTabWidget;
}

class NotificationsTabWidget : public QWidget {
		Q_OBJECT

	public:
		NotificationsTabWidget(QWidget* parent = nullptr);
		~NotificationsTabWidget();

		void SetFeatureEnabled(bool enabled);

	public slots:
		void CurrentTabChanged(const Tab& tab);
		void ServerStateChanged(const ServerState& state);

		void NotificationReceived(const Notification& notification);
		void NotificationRemoved(const Notification& notification);
		void NotificationList(const std::list<Notification>& list);

		void on_dismissAllButton_clicked();
		void on_refreshButton_clicked();

	signals:
		void ListNotifications();
		void DismissNotification(const QString& key);

	private:
		Ui::NotificationsTabWidget* ui;

		Tab m_tab;
		ServerState m_serverState;
		bool m_featureEnabled;

		QTimer* m_retryTimer;

		bool m_contentLoaded;
		bool m_contentEmpty;

		void UpdateLayout();

		void LoadContent();
		void ClearContent();

		void InsertNotification(const Notification& notification, bool skipRemove = false);
		void RemoveNotification(const QString& key);
};

#endif /* NOTIFICATIONS_TAB_WIDGET_H */
