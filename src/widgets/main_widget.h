#ifndef MAIN_WIDGET_H
#define MAIN_WIDGET_H

#include <QDir>
#include <QWidget>
#include <QSettings>

#include "../common.h"
#include "../server.h"
#include "../bridge.h"

#include "fade_widget.h"

#include "tabs/notifications_tab_widget.h"
#include "tabs/sms_tab_widget.h"
#include "tabs/device_tab_widget.h"
#include "tabs/settings_tab_widget.h"

namespace Ui {
	class MainWidget;
}

class MainWidget : public FadeWidget {
		Q_OBJECT

	public:
		MainWidget(QSettings* settings, QWidget* parent = nullptr);
		~MainWidget();

	public slots:
		void VROverlayShown();
		void VRNotificationOpened(const std::string& identifier);
		void VRKeyboardData(uint8_t identifier, const std::string& data);

	private slots:
		void ServerStateChanged(bool connected);

		void on_notificationsButton_clicked();
		void on_smsButton_clicked();
		void on_deviceButton_clicked();
		void on_settingsButton_clicked();

	signals:
		void ShowVRNotification(const std::string& identifier,
								const std::string& text,
								bool sound = true,
								bool persistent = false);
		void RemoveVRNotification(const std::string& identifier);
		void ShowVRKeyboard(uint8_t identifier,
							uint32_t maxLen,
							const char* initialText = nullptr,
							const char* description = nullptr,
							bool singleLine = true,
							bool password = false);

	private:
		Ui::MainWidget* ui;

		Tab m_currentTab;
		ServerState m_serverState;

		QSettings* m_settings;
		Bridge* m_bridge;
		Server* m_server;

		NotificationsTabWidget* m_notificationsTab;
		SMSTabWidget* m_smsTab;
		DeviceTabWidget* m_deviceTab;
		SettingsTabWidget* m_settingsTab;

		QWidget* GetTabWidget(const Tab& tab);
		void SwitchTab(const Tab& tab, bool animate = true);

		void UpdateServerState(const ServerState& state);

		bool StartServer(std::string* error = nullptr);
		void StopServer();

		void SetupStylesheet();
		void SetupBridge();
		void SetupTabs();
		void SetupServer();

		void SetupNotificationsTab();
		void SetupSMSTab();
		void SetupDeviceTab();
		void SetupSettingsTab();
};

#endif /* MAIN_WIDGET_H */
