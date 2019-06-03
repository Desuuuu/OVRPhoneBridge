#include <QDir>
#include <QFile>
#include <QStyle>
#include <QGraphicsOpacityEffect>

#include <spdlog/spdlog.h>

#include "main_widget.h"
#include "ui_main_widget.h"
#include "../common.h"

MainWidget::MainWidget(QSettings* settings, QWidget* parent)
	: FadeWidget(parent),
	  ui(new Ui::MainWidget),
	  m_currentTab(Tab::NONE),
	  m_serverState(ServerState::NONE),
	  m_settings(settings),
	  m_bridge(nullptr),
	  m_server(nullptr),
	  m_notificationsTab(nullptr),
	  m_smsTab(nullptr),
	  m_deviceTab(nullptr),
	  m_settingsTab(nullptr) {
	ui->setupUi(this);

	ui->appNameLabel->setText(OVERLAY_NAME);
	ui->versionLabel->setText(APP_VERSION);

	SetupStylesheet();
	SetupBridge();
	SetupTabs();
	SetupServer();
}

MainWidget::~MainWidget() {
	m_serverState = ServerState::STOPPED;

	StopServer();

	delete ui;
}

void MainWidget::VROverlayShown() {
	SwitchTab(m_serverState == ServerState::CONNECTED
			  ? Tab::NOTIFICATIONS
			  : Tab::SETTINGS, false);
}

void MainWidget::VRNotificationOpened(const std::string& identifier) {
	if (m_serverState == ServerState::CONNECTED) {
		if (identifier.rfind("notif_", 0) == 0) {
			SwitchTab(Tab::NOTIFICATIONS, false);
		}
	}
}

void MainWidget::VRKeyboardData(uint8_t identifier, const std::string& data) {
	if (m_smsTab != nullptr) {
		m_smsTab->VRKeyboardData(identifier, data);
	}

	if (m_settingsTab != nullptr) {
		m_settingsTab->VRKeyboardData(identifier, data);
	}
}

void MainWidget::ServerStateChanged(bool connected) {
	if (connected) {
		UpdateServerState(ServerState::CONNECTED);
	} else {
		UpdateServerState(ServerState::STARTED);
	}
}

void MainWidget::on_notificationsButton_clicked() {
	SwitchTab(Tab::NOTIFICATIONS);
}

void MainWidget::on_smsButton_clicked() {
	SwitchTab(Tab::SMS);
}

void MainWidget::on_deviceButton_clicked() {
	SwitchTab(Tab::DEVICE);
}

void MainWidget::on_settingsButton_clicked() {
	SwitchTab(Tab::SETTINGS);
}

QWidget* MainWidget::GetTabWidget(const Tab& tab) {
	switch (tab) {
		case Tab::NOTIFICATIONS:
			return m_notificationsTab;

		case Tab::SMS:
			return m_smsTab;

		case Tab::DEVICE:
			return m_deviceTab;

		case Tab::SETTINGS:
			return m_settingsTab;

		default:
			return nullptr;
	}
}

void MainWidget::SwitchTab(const Tab& tab, bool animate) {
	if (tab == m_currentTab) {
		return;
	}

	switch (tab) {
		case Tab::NOTIFICATIONS:
			ui->smsButton->SetHighlight(false);
			ui->deviceButton->SetHighlight(false);
			ui->settingsButton->SetHighlight(false);
			ui->notificationsButton->SetHighlight(true);

			if (animate) {
				FadeOutIn(GetTabWidget(m_currentTab), m_notificationsTab, 50, 100);
			} else {
				m_smsTab->setVisible(false);
				m_deviceTab->setVisible(false);
				m_settingsTab->setVisible(false);
				m_notificationsTab->setVisible(true);
			}

			break;

		case Tab::SMS:
			ui->notificationsButton->SetHighlight(false);
			ui->deviceButton->SetHighlight(false);
			ui->settingsButton->SetHighlight(false);
			ui->smsButton->SetHighlight(true);

			if (animate) {
				FadeOutIn(GetTabWidget(m_currentTab), m_smsTab, 50, 100);
			} else {
				m_notificationsTab->setVisible(false);
				m_deviceTab->setVisible(false);
				m_settingsTab->setVisible(false);
				m_smsTab->setVisible(true);
			}

			break;

		case Tab::DEVICE:
			ui->notificationsButton->SetHighlight(false);
			ui->smsButton->SetHighlight(false);
			ui->settingsButton->SetHighlight(false);
			ui->deviceButton->SetHighlight(true);

			if (animate) {
				FadeOutIn(GetTabWidget(m_currentTab), m_deviceTab, 50, 100);
			} else {
				m_notificationsTab->setVisible(false);
				m_smsTab->setVisible(false);
				m_settingsTab->setVisible(false);
				m_deviceTab->setVisible(true);
			}

			break;

		case Tab::SETTINGS:
			ui->notificationsButton->SetHighlight(false);
			ui->smsButton->SetHighlight(false);
			ui->deviceButton->SetHighlight(false);
			ui->settingsButton->SetHighlight(true);

			if (animate) {
				FadeOutIn(GetTabWidget(m_currentTab), m_settingsTab, 50, 100);
			} else {
				m_notificationsTab->setVisible(false);
				m_smsTab->setVisible(false);
				m_deviceTab->setVisible(false);
				m_settingsTab->setVisible(true);
			}

			break;

		default:
			return;
	}

	m_currentTab = tab;

	m_notificationsTab->CurrentTabChanged(m_currentTab);
	m_smsTab->CurrentTabChanged(m_currentTab);
}

void MainWidget::UpdateServerState(const ServerState& state) {
	if (state == m_serverState) {
		return;
	}

	switch (state) {
		case ServerState::STOPPED:
			ui->statusLabel->setText("<font color='#8a2929'>Stopped</font>");
			break;

		case ServerState::STARTED:
			ui->statusLabel->setText("Running");
			break;

		case ServerState::CONNECTED:
			m_notificationsTab->SetFeatureEnabled(m_server->HasNotifications());

			m_smsTab->SetFeatureEnabled(m_server->HasSMS());

			m_deviceTab->SetFeatures(m_server->HasNotifications(), m_server->HasSMS());
			m_deviceTab->SetDeviceName(m_server->GetClientDeviceName());
			m_deviceTab->SetAddress(m_server->GetClientAddress());
			m_deviceTab->SetAppVersion(m_server->GetClientAppVersion());
			m_deviceTab->SetOS(m_server->GetClientOSType(), m_server->GetClientOSVersion());

			ui->statusLabel->setText("<font color='#52a93e'>Connected</font>");
			break;

		default:
			return;
	}

	m_serverState = state;

	m_notificationsTab->ServerStateChanged(m_serverState);
	m_smsTab->ServerStateChanged(m_serverState);
	m_deviceTab->ServerStateChanged(m_serverState);
	m_settingsTab->ServerStateChanged(m_serverState);
}

bool MainWidget::StartServer(std::string* error) {
	if (m_server != nullptr) {
		return true;
	}

	QString publicKey = m_settings->value("public_key", "").toString();

	if (publicKey.isEmpty()) {
		if (error != nullptr) {
			*error = "Missing public key";
		}

		return false;
	}

	QString secretKey = m_settings->value("secret_key", "").toString();

	if (secretKey.isEmpty()) {
		if (error != nullptr) {
			*error = "Missing secret key";
		}

		return false;
	}

	uint serverPort = m_settings->value("server_port", DEFAULT_PORT).toUInt();

	if (serverPort < 1 || serverPort > 65535) {
		if (error != nullptr) {
			*error = "Invalid port";
		}

		return false;
	}

	try {
		m_server = new Server(publicKey,
							  secretKey,
							  static_cast<uint16_t>(serverPort),
							  QHostAddress::Any,
							  this);
	} catch (const std::runtime_error& ex) {
		if (error != nullptr) {
			*error = ex.what();
		}

		return false;
	}

	connect(m_server, &Server::ConnectedChange, this, &MainWidget::ServerStateChanged);

	connect(m_server, &Server::MessageReceived, m_bridge, &Bridge::ParseMessage);
	connect(m_bridge, &Bridge::EmitMessage, m_server, &Server::SendMessageToClient);

	UpdateServerState(ServerState::STARTED);

	return true;
}

void MainWidget::StopServer() {
	if (m_server != nullptr) {
		disconnect(m_server, &Server::ConnectedChange, this, &MainWidget::ServerStateChanged);

		if (m_bridge != nullptr) {
			disconnect(m_server, &Server::MessageReceived, m_bridge, &Bridge::ParseMessage);
			disconnect(m_bridge, &Bridge::EmitMessage, m_server, &Server::SendMessageToClient);
		}
	}

	UpdateServerState(ServerState::STOPPED);

	if (m_server != nullptr) {
		m_server->Stop();

		delete m_server;
		m_server = nullptr;
	}
}

void MainWidget::SetupStylesheet() {
	QFile styleFile(":/src/widgets/main_widget.qss");

	if (!styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
		throw std::runtime_error("Failed to load stylesheet");
	}

	setStyleSheet(styleFile.readAll());

	styleFile.close();
}

void MainWidget::SetupBridge() {
	m_bridge = new Bridge(this);

	connect(m_bridge, &Bridge::NotificationReceived, this, [&](const Notification & notification) {
		QString identifier = "notif_" + notification.key;
		QString text = notification.appName + "\n" + notification.title;

		emit ShowVRNotification(identifier.toStdString(),
								text.toStdString(),
								m_settings->value("notificationSound", true).toBool());
	});

	connect(m_bridge, &Bridge::NotificationRemoved, this, [&](const Notification & notification) {
		QString identifier = "notif_" + notification.key;

		emit RemoveVRNotification(identifier.toStdString());
	});

	connect(m_bridge, &Bridge::SMSSent, this, [&](const QString&, bool success) {
		emit ShowVRNotification("sms_sent",
								(success ? "SMS sent" : "Failed to send SMS"),
								false);
	});
}

void MainWidget::SetupTabs() {
	SetupNotificationsTab();
	SetupSMSTab();
	SetupDeviceTab();
	SetupSettingsTab();

	SwitchTab(Tab::SETTINGS, false);
}

void MainWidget::SetupServer() {
	UpdateServerState(ServerState::STOPPED);

	if (!m_settings->contains("public_key")
			|| !m_settings->contains("secret_key")
			|| !m_settings->contains("identifier")) {
		Crypto::GenerateKeyPair(m_settings);
	}

	if (m_settings->value("autostart", DEFAULT_AUTOSTART).toBool()) {
		std::string error;

		if (!StartServer(&error)) {
			error.insert(0, "Failed to start server: ");

			spdlog::error(error);
		}
	}
}

void MainWidget::SetupNotificationsTab() {
	QGraphicsOpacityEffect* m_notificationsEffect = new QGraphicsOpacityEffect(this);
	m_notificationsEffect->setOpacity(OPACITY_EFFECT_MAX);

	m_notificationsTab = new NotificationsTabWidget(this);
	m_notificationsTab->setVisible(false);
	m_notificationsTab->setGraphicsEffect(m_notificationsEffect);

	connect(m_bridge,
			&Bridge::NotificationReceived,
			m_notificationsTab,
			&NotificationsTabWidget::NotificationReceived);

	connect(m_bridge,
			&Bridge::NotificationRemoved,
			m_notificationsTab,
			&NotificationsTabWidget::NotificationRemoved);

	connect(m_bridge,
			&Bridge::NotificationList,
			m_notificationsTab,
			&NotificationsTabWidget::NotificationList);

	connect(m_notificationsTab,
			&NotificationsTabWidget::ListNotifications,
			m_bridge,
			&Bridge::ListNotifications);

	connect(m_notificationsTab,
			&NotificationsTabWidget::DismissNotification,
			m_bridge,
			&Bridge::DismissNotification);

	ui->contentLayout->addWidget(m_notificationsTab, 0, Qt::AlignHCenter);
}

void MainWidget::SetupSMSTab() {
	QGraphicsOpacityEffect* m_smsEffect = new QGraphicsOpacityEffect(this);
	m_smsEffect->setOpacity(OPACITY_EFFECT_MAX);

	m_smsTab = new SMSTabWidget(this);
	m_smsTab->setVisible(false);
	m_smsTab->setGraphicsEffect(m_smsEffect);

	connect(m_smsTab,
			&SMSTabWidget::ShowVRKeyboard,
			this,
			&MainWidget::ShowVRKeyboard);

	connect(m_bridge,
			&Bridge::SMSList,
			m_smsTab,
			&SMSTabWidget::SMSList);

	connect(m_bridge,
			&Bridge::SMSFromList,
			m_smsTab,
			&SMSTabWidget::SMSFromList);

	connect(m_bridge,
			&Bridge::SMSSent,
			m_smsTab,
			&SMSTabWidget::SMSSent);

	connect(m_smsTab,
			&SMSTabWidget::ListSMS,
			m_bridge,
			&Bridge::ListSMS);

	connect(m_smsTab,
			&SMSTabWidget::ListSMSFrom,
			m_bridge,
			&Bridge::ListSMSFrom);

	connect(m_smsTab,
			&SMSTabWidget::SendSMS,
			m_bridge,
			&Bridge::SendSMS);

	ui->contentLayout->addWidget(m_smsTab, 0, Qt::AlignHCenter);
}

void MainWidget::SetupDeviceTab() {
	QGraphicsOpacityEffect* m_deviceEffect = new QGraphicsOpacityEffect(this);
	m_deviceEffect->setOpacity(OPACITY_EFFECT_MAX);

	m_deviceTab = new DeviceTabWidget(this);
	m_deviceTab->setVisible(false);
	m_deviceTab->setGraphicsEffect(m_deviceEffect);

	connect(m_deviceTab,
			&DeviceTabWidget::KickClient,
			this,
	[&]() {
		if (m_server != nullptr) {
			m_server->KickClient();
		}
	});

	ui->contentLayout->addWidget(m_deviceTab, 0, Qt::AlignHCenter);
}

void MainWidget::SetupSettingsTab() {
	QGraphicsOpacityEffect* m_settingsEffect = new QGraphicsOpacityEffect(this);
	m_settingsEffect->setOpacity(OPACITY_EFFECT_MAX);

	m_settingsTab = new SettingsTabWidget(m_settings, this);
	m_settingsTab->setVisible(false);
	m_settingsTab->setGraphicsEffect(m_settingsEffect);

	connect(m_settingsTab,
			&SettingsTabWidget::ShowVRNotification,
			this,
			&MainWidget::ShowVRNotification);

	connect(m_settingsTab,
			&SettingsTabWidget::ShowVRKeyboard,
			this,
			&MainWidget::ShowVRKeyboard);

	connect(m_settingsTab,
			&SettingsTabWidget::StartServer,
			this,
	[&]() {
		std::string error;

		if (!StartServer(&error)) {
			error.insert(0, "Failed to start server\n");

			emit ShowVRNotification("start_failed", error, false);
		}
	});

	connect(m_settingsTab,
			&SettingsTabWidget::StopServer,
			this,
			&MainWidget::StopServer);

	ui->contentLayout->addWidget(m_settingsTab, 0, Qt::AlignHCenter);
}
