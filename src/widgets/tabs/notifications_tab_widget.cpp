#include <QTimer>
#include <QScrollBar>
#include <QSvgWidget>
#include <QSvgRenderer>

#include "notifications_tab_widget.h"
#include "ui_notifications_tab_widget.h"

#include "../notification_widget.h"

NotificationsTabWidget::NotificationsTabWidget(QWidget* parent)
	: QWidget(parent),
	  ui(new Ui::NotificationsTabWidget),
	  m_tab(Tab::NONE),
	  m_serverState(ServerState::NONE),
	  m_featureEnabled(false),
	  m_retryTimer(nullptr),
	  m_contentLoaded(false),
	  m_contentEmpty(true) {
	ui->setupUi(this);

	QSvgWidget* loader = new QSvgWidget(":/images/loading.svg", this);
	loader->renderer()->setFramesPerSecond(60);
	loader->setFixedSize(70, 70);

	ui->loadingLayout->insertWidget(0, loader, 0, Qt::AlignHCenter);

	m_retryTimer = new QTimer(this);

	connect(m_retryTimer, &QTimer::timeout, this, &NotificationsTabWidget::LoadContent);

	m_retryTimer->setInterval(30000);
	m_retryTimer->setSingleShot(true);

	ui->statusLabel->setVisible(false);
	ui->loadingWidget->setVisible(false);
	ui->contentWidget->setVisible(false);
}

NotificationsTabWidget::~NotificationsTabWidget() {
	delete ui;
}

void NotificationsTabWidget::SetFeatureEnabled(bool enabled) {
	m_featureEnabled = enabled;
}

void NotificationsTabWidget::CurrentTabChanged(const Tab& tab) {
	m_tab = tab;

	if (m_serverState == ServerState::CONNECTED && m_tab == Tab::NOTIFICATIONS) {
		QTimer::singleShot(LOAD_DELAY, this, &NotificationsTabWidget::LoadContent);
	}
}

void NotificationsTabWidget::ServerStateChanged(const ServerState& state) {
	m_serverState = state;

	setUpdatesEnabled(false);

	ClearContent();

	UpdateLayout();

	if (m_serverState == ServerState::CONNECTED && m_tab == Tab::NOTIFICATIONS) {
		QTimer::singleShot(LOAD_DELAY, this, &NotificationsTabWidget::LoadContent);
	}
}

void NotificationsTabWidget::UpdateLayout() {
	setUpdatesEnabled(false);

	switch (m_serverState) {
		case ServerState::STOPPED:
			ui->loadingWidget->setVisible(false);
			ui->contentWidget->setVisible(false);

			ui->statusLabel->setText("<font color='#8a2929'>Server not running</font>");
			ui->statusLabel->setVisible(true);
			break;

		case ServerState::STARTED:
			ui->loadingWidget->setVisible(false);
			ui->contentWidget->setVisible(false);

			ui->statusLabel->setText("No client connected");
			ui->statusLabel->setVisible(true);
			break;

		case ServerState::CONNECTED:
			if (!m_featureEnabled) {
				ui->loadingWidget->setVisible(false);
				ui->contentWidget->setVisible(false);

				ui->statusLabel->setText("Feature disabled");
				ui->statusLabel->setVisible(true);
			} else if (!m_contentLoaded) {
				ui->statusLabel->setVisible(false);
				ui->contentWidget->setVisible(false);

				ui->loadingWidget->setVisible(true);
			} else if (m_contentEmpty) {
				ui->loadingWidget->setVisible(false);
				ui->contentWidget->setVisible(false);

				ui->statusLabel->setText("No notification");
				ui->statusLabel->setVisible(true);
			} else {
				ui->statusLabel->setVisible(false);
				ui->loadingWidget->setVisible(false);

				ui->contentWidget->setVisible(true);

				ui->contentScrollArea->verticalScrollBar()->setValue(0);
			}

			break;

		default:
			break;
	}

	setUpdatesEnabled(true);
}

void NotificationsTabWidget::LoadContent() {
	m_retryTimer->stop();

	if (m_contentLoaded
			|| m_tab != Tab::NOTIFICATIONS
			|| m_serverState != ServerState::CONNECTED) {
		return;
	}

	emit ListNotifications();

	m_retryTimer->start();
}

void NotificationsTabWidget::ClearContent() {
	QLayoutItem* item;

	int i = ui->contentScrollLayout->count();

	while (--i >= 0) {
		item = ui->contentScrollLayout->itemAt(i);

		if (item == nullptr) {
			ui->contentScrollLayout->takeAt(i);

			continue;
		}

		if (item->spacerItem() != nullptr) {
			continue;
		}

		if (item->widget() != nullptr) {
			item->widget()->deleteLater();
		}

		if (item->layout() != nullptr) {
			item->layout()->deleteLater();
		}

		delete ui->contentScrollLayout->takeAt(i);
	}

	m_contentLoaded = false;
	m_contentEmpty = true;
}

void NotificationsTabWidget::InsertNotification(const Notification& notification, bool skipRemove) {
	if (!skipRemove) {
		RemoveNotification(notification.key);
	}

	NotificationWidget* widget = new NotificationWidget(notification, ui->contentScrollWidget);

	connect(widget,
			&NotificationWidget::Dismiss,
			this,
			&NotificationsTabWidget::DismissNotification);

	ui->contentScrollLayout->insertWidget(0, widget);
}

void NotificationsTabWidget::RemoveNotification(const QString& key) {
	QLayoutItem* item;
	NotificationWidget* widget;

	int i = ui->contentScrollLayout->count();

	while (--i >= 0) {
		item = ui->contentScrollLayout->itemAt(i);

		if (item == nullptr || item->widget() == nullptr) {
			continue;
		}

		widget = qobject_cast<NotificationWidget*>(item->widget());

		if (widget != nullptr && key == widget->GetKey()) {
			widget->deleteLater();

			delete ui->contentScrollLayout->takeAt(i);

			break;
		}
	}
}

void NotificationsTabWidget::NotificationReceived(const Notification& notification) {
	InsertNotification(notification);

	if (m_contentEmpty) {
		m_contentEmpty = false;

		UpdateLayout();
	}
}

void NotificationsTabWidget::NotificationRemoved(const Notification& notification) {
	RemoveNotification(notification.key);

	if (ui->contentScrollLayout->count() <= 1) {
		m_contentEmpty = true;

		UpdateLayout();
	}
}

void NotificationsTabWidget::NotificationList(const std::list<Notification>& list) {
	m_retryTimer->stop();

	if (m_contentLoaded) {
		return;
	}

	setUpdatesEnabled(false);

	ClearContent();

	auto iterator = list.rbegin();

	while (iterator != list.rend()) {
		InsertNotification(*iterator, true);

		m_contentEmpty = false;

		++iterator;
	}

	m_contentLoaded = true;

	UpdateLayout();
}

void NotificationsTabWidget::on_dismissAllButton_clicked() {
	QLayoutItem* item;
	NotificationWidget* widget;

	int i = ui->contentScrollLayout->count();

	while (--i >= 0) {
		item = ui->contentScrollLayout->itemAt(i);

		if (item == nullptr || item->widget() == nullptr) {
			continue;
		}

		widget = qobject_cast<NotificationWidget*>(item->widget());

		if (widget != nullptr && !widget->IsPersistent()) {
			emit DismissNotification(widget->GetKey());
		}
	}
}

void NotificationsTabWidget::on_refreshButton_clicked() {
	setUpdatesEnabled(false);

	ClearContent();

	UpdateLayout();

	QTimer::singleShot(LOAD_DELAY, this, &NotificationsTabWidget::LoadContent);
}
