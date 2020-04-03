#include "device_tab_widget.h"
#include "ui_device_tab_widget.h"

DeviceTabWidget::DeviceTabWidget(QWidget* parent)
	: QWidget(parent),
	  ui(new Ui::DeviceTabWidget) {
	ui->setupUi(this);

	ui->contentTable->setRowCount(5);
	ui->contentTable->setColumnCount(2);

	ui->contentTable->setItem(POSITION_NAME, 0, new QTableWidgetItem("Name"));
	ui->contentTable->setItem(POSITION_IDENTIFIER, 0, new QTableWidgetItem("Identifier"));
	ui->contentTable->setItem(POSITION_ADDRESS, 0, new QTableWidgetItem("Address"));
	ui->contentTable->setItem(POSITION_FEATURES, 0, new QTableWidgetItem("Features"));
	ui->contentTable->setItem(POSITION_VERSION, 0, new QTableWidgetItem("App version"));
	ui->contentTable->setItem(POSITION_OS, 0, new QTableWidgetItem("OS"));

	ui->statusLabel->setVisible(false);
	ui->contentWidget->setVisible(false);

	connect(ui->kickButton, &QPushButton::clicked, this, &DeviceTabWidget::KickClient);
}

DeviceTabWidget::~DeviceTabWidget() {
	delete ui;
}

void DeviceTabWidget::SetFeatures(bool notifications, bool sms) {
	QStringList list;

	if (notifications) {
		list.append("Notifications");
	}

	if (sms) {
		list.append("SMS");
	}

	QString result = list.join(", ");

	QTableWidgetItem* element = new QTableWidgetItem();
	element->setFlags(element->flags() & ~Qt::ItemIsEnabled);
	element->setTextAlignment(Qt::AlignRight);

	if (result.isEmpty()) {
		element->setText("None");
	} else {
		element->setText(result);
	}

	ui->contentTable->setItem(POSITION_FEATURES, 1, element);
}

void DeviceTabWidget::SetDeviceName(const QString& deviceName) {
	QTableWidgetItem* element = new QTableWidgetItem(deviceName);
	element->setFlags(element->flags() & ~Qt::ItemIsEnabled);
	element->setTextAlignment(Qt::AlignRight);

	ui->contentTable->setItem(POSITION_NAME, 1, element);
}

void DeviceTabWidget::SetIdentifier(const QString& identifier) {
	QTableWidgetItem* element = new QTableWidgetItem(identifier);
	element->setFlags(element->flags() & ~Qt::ItemIsEnabled);
	element->setTextAlignment(Qt::AlignRight);

	ui->contentTable->setItem(POSITION_IDENTIFIER, 1, element);
}

void DeviceTabWidget::SetAddress(const QString& address) {
	QTableWidgetItem* element = new QTableWidgetItem(address);
	element->setFlags(element->flags() & ~Qt::ItemIsEnabled);
	element->setTextAlignment(Qt::AlignRight);

	ui->contentTable->setItem(POSITION_ADDRESS, 1, element);
}

void DeviceTabWidget::SetAppVersion(const QString& version) {
	QTableWidgetItem* element = new QTableWidgetItem(version);
	element->setFlags(element->flags() & ~Qt::ItemIsEnabled);
	element->setTextAlignment(Qt::AlignRight);

	ui->contentTable->setItem(POSITION_VERSION, 1, element);
}

void DeviceTabWidget::SetOS(const QString& type, const QString& version) {
	QTableWidgetItem* element = new QTableWidgetItem();
	element->setFlags(element->flags() & ~Qt::ItemIsEnabled);
	element->setTextAlignment(Qt::AlignRight);

	if (type == "android") {
		element->setText(QString("Android ") + version);
	} else {
		element->setText("Unknown");
	}

	ui->contentTable->setItem(POSITION_OS, 1, element);
}

void DeviceTabWidget::ServerStateChanged(const ServerState& state) {
	if (state == ServerState::STOPPED) {
		ui->contentWidget->setVisible(false);

		ui->statusLabel->setText("<font color='#8a2929'>Server not running</font>");
		ui->statusLabel->setVisible(true);
	} else if (state == ServerState::STARTED) {
		ui->contentWidget->setVisible(false);

		ui->statusLabel->setText("No client connected");
		ui->statusLabel->setVisible(true);
	} else if (state == ServerState::CONNECTED) {
		ui->statusLabel->setVisible(false);

		ui->contentTable->resizeColumnsToContents();
		ui->contentTable->resizeRowsToContents();
		ui->contentTable->update();

		ui->contentWidget->setVisible(true);
	}
}
