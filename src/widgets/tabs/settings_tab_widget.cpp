#include <QIntValidator>

#include <sstream>
#include <spdlog/spdlog.h>

#include "settings_tab_widget.h"
#include "ui_settings_tab_widget.h"
#include "../../crypto.h"

SettingsTabWidget::SettingsTabWidget(QSettings* settings, QWidget* parent)
	: QWidget(parent),
	  ui(new Ui::SettingsTabWidget),
	  m_settings(settings),
	  m_serverState(ServerState::NONE) {
	ui->setupUi(this);

	m_desktop = qApp->arguments().indexOf("--desktop") >= 0;

	ui->settingsLayout->setAlignment(Qt::AlignHCenter);

	uint serverPort = m_settings->value("server_port", DEFAULT_PORT).toUInt();

	if (serverPort >= 1 && serverPort <= 65535) {
		ui->portLineEdit->setText(std::to_string(serverPort).c_str());
	} else {
		ui->portLineEdit->setText("");
	}

	if (m_desktop) {
		ui->portLineEdit->setValidator(new QIntValidator(1, 65535, this));

		connect(ui->portLineEdit, &FocusLineEdit::editingFinished, this, [&]() {
			if (SavePort(ui->portLineEdit->text().toStdString())) {
				ui->portLineEdit->clearFocus();
			}
		});
	}

	if (m_settings->value("autostart", DEFAULT_AUTOSTART).toBool()) {
		ui->autoStartCheckBox->setCheckState(Qt::Checked);
	} else {
		ui->autoStartCheckBox->setCheckState(Qt::Unchecked);
	}

	if (m_settings->value("notificationSound", true).toBool()) {
		ui->notificationSoundCheckBox->setCheckState(Qt::Checked);
	} else {
		ui->notificationSoundCheckBox->setCheckState(Qt::Unchecked);
	}
}

SettingsTabWidget::~SettingsTabWidget() {
	delete ui;
}

void SettingsTabWidget::ServerStateChanged(const ServerState& state) {
	m_serverState = state;

	if (m_serverState == ServerState::STOPPED) {
		ui->toggleServerButton->setText("Start server");
	} else if (m_serverState == ServerState::STARTED || m_serverState == ServerState::CONNECTED) {
		ui->toggleServerButton->setText("Stop server");
	}
}

void SettingsTabWidget::VRKeyboardData(uint8_t identifier, const std::string& data) {
	if (identifier == KEYBOARD_INPUT_PORT) {
		std::string input = trim(data);

		if (SavePort(input)) {
			ui->portLineEdit->setText(input.c_str());
		}
	}
}

void SettingsTabWidget::on_portLineEdit_FocusReceived(FocusLineEdit* edit) {
	emit ShowVRKeyboard(KEYBOARD_INPUT_PORT,
						static_cast<uint32_t>(edit->maxLength()),
						edit->text().toStdString().c_str(),
						"Port");
}

void SettingsTabWidget::on_autoStartCheckBox_stateChanged(int state) {
	m_settings->setValue("autostart", (state != Qt::Unchecked));
}

void SettingsTabWidget::on_notificationSoundCheckBox_stateChanged(int state) {
	m_settings->setValue("notificationSound", (state != Qt::Unchecked));
}

void SettingsTabWidget::on_toggleServerButton_clicked() {
	if (m_serverState == ServerState::STOPPED) {
		emit StartServer();
	} else if (m_serverState == ServerState::STARTED || m_serverState == ServerState::CONNECTED) {
		emit StopServer();
	}
}

bool SettingsTabWidget::SavePort(const std::string& input) {
	try {
		size_t pos;

		int value = std::stoi(input, &pos, 10);

		if (pos != input.length()) {
			throw std::runtime_error("Invalid port");
		}

		if (value < 1 || value > 65535) {
			throw std::runtime_error("Port out of range");
		}

		m_settings->setValue("server_port", static_cast<uint>(value));
	} catch (const std::exception& ex) {
		spdlog::warn(ex.what());

		emit ShowVRNotification("invalid_port",
								"Failed to edit port\nInvalid port",
								false);

		return false;
	}

	return true;
}
