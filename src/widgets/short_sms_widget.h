#pragma once

#include <QWidget>

#include "../common.h"

class ShortSMSWidget : public QWidget {
		Q_OBJECT

	public:
		ShortSMSWidget(const ShortSMS& shortSms, QWidget* parent = nullptr);

	protected:
		void paintEvent(QPaintEvent* e) override;
};
