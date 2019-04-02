#ifndef NOTIFICATION_WIDGET_H
#define NOTIFICATION_WIDGET_H

#include <QWidget>

#include "../common.h"

class NotificationWidget : public QWidget {
		Q_OBJECT

	public:
		NotificationWidget(const Notification& notification, QWidget* parent = nullptr);

		const QString& GetKey();
		const bool& IsPersistent();

	protected:
		void paintEvent(QPaintEvent* e) override;

	signals:
		void Dismiss(const QString& key);

	private:
		QString m_key;
		bool m_persistent;
};

#endif /* NOTIFICATION_WIDGET_H */
