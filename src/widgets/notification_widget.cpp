#include <QLabel>
#include <QPainter>
#include <QGridLayout>
#include <QPushButton>
#include <QStyleOption>

#include "notification_widget.h"

NotificationWidget::NotificationWidget(const Notification& notification, QWidget* parent)
	: QWidget(parent),
	  m_key(notification.key),
	  m_persistent(notification.persistent) {
	setObjectName("NotificationWidget");

	QGridLayout* layout = new QGridLayout();

	layout->setSpacing(0);

	QLabel* appName = new QLabel(notification.appName, this);
	appName->setObjectName("notificationAppName");
	appName->setTextFormat(Qt::PlainText);

	layout->addWidget(appName, 0, 0, 1, 3, Qt::AlignHCenter);

	QLabel* title = new QLabel(ellipsize(notification.title, NOTIF_TITLE_MAX_LENGTH), this);

	title->setObjectName("notificationTitle");
	title->setTextFormat(Qt::PlainText);

	layout->addWidget(title, 1, 0, 1, 2);

	QLabel* text = new QLabel(ellipsize(notification.text, NOTIF_TEXT_MAX_LENGTH), this);

	text->setObjectName("notificationText");
	text->setTextFormat(Qt::PlainText);
	text->setWordWrap(true);

	layout->addWidget(text, 2, 0, 1, 2);

	if (notification.persistent) {
		layout->addItem(new QSpacerItem(1, 1, QSizePolicy::Fixed, QSizePolicy::Fixed), 2, 2);
	} else {
		QPushButton* dismiss = new QPushButton("Dismiss", this);
		dismiss->setObjectName("dismissButton");

		connect(dismiss, &QPushButton::clicked, this, [&]() {
			emit Dismiss(m_key);
		});

		layout->addWidget(dismiss, 0, 2, 3, 1, Qt::AlignVCenter);
	}

	layout->setColumnStretch(1, 1);

	setLayout(layout);
}

const QString& NotificationWidget::GetKey() {
	return m_key;
}

const bool& NotificationWidget::IsPersistent() {
	return m_persistent;
}

void NotificationWidget::paintEvent(QPaintEvent*) {
	QStyleOption opt;
	opt.init(this);
	QPainter p(this);
	style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}
