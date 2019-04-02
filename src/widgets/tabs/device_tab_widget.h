#ifndef DEVICE_TAB_WIDGET_H
#define DEVICE_TAB_WIDGET_H

#include <QWidget>

#include "../../common.h"

namespace Ui {
	class DeviceTabWidget;
}

class DeviceTabWidget : public QWidget {
		Q_OBJECT

	public:
		DeviceTabWidget(QWidget* parent = nullptr);
		~DeviceTabWidget();

		void SetFeatures(bool notifications, bool sms);
		void SetDeviceName(const QString& deviceName);
		void SetAddress(const QString& address);
		void SetAppVersion(const QString& version);
		void SetOS(const QString& type, const QString& version);

	public slots:
		void ServerStateChanged(const ServerState& state);

	signals:
		void KickClient();

	private:
		Ui::DeviceTabWidget* ui;
};

#endif /* DEVICE_TAB_WIDGET_H */
