#ifndef SETTINGS_TAB_WIDGET_H
#define SETTINGS_TAB_WIDGET_H

#include <QWidget>
#include <QSettings>

#include "../../common.h"
#include "../focus_line_edit.h"

namespace Ui {
	class SettingsTabWidget;
}

class SettingsTabWidget : public QWidget {
		Q_OBJECT

	public:
		SettingsTabWidget(QSettings* settings, QWidget* parent = nullptr);
		~SettingsTabWidget();

	public slots:
		void ServerStateChanged(const ServerState& state);

		void VRKeyboardData(uint8_t identifier, const std::string& data);

	private slots:
		void on_portLineEdit_FocusReceived(FocusLineEdit* edit);
		void on_autoStartCheckBox_stateChanged(int state);
		void on_notificationSoundCheckBox_stateChanged(int state);
		void on_toggleServerButton_clicked();

	signals:
		void StartServer();
		void StopServer();

		void ShowVRNotification(const std::string& identifier,
								const std::string& text,
								bool sound = true,
								bool persistent = false);
		void ShowVRKeyboard(uint8_t identifier,
							uint32_t maxLen,
							const char* initialText = nullptr,
							const char* description = nullptr,
							bool singleLine = true,
							bool password = false);

	private:
		Ui::SettingsTabWidget* ui;

		QSettings* m_settings;
		ServerState m_serverState;
		bool m_desktop;

		bool SavePort(const std::string& input);
};

#endif /* SETTINGS_TAB_WIDGET_H */
