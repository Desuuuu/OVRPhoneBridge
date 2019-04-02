#ifndef SHORT_SMS_WIDGET_H
#define SHORT_SMS_WIDGET_H

#include <QWidget>

#include "../common.h"

class ShortSMSWidget : public QWidget {
		Q_OBJECT

	public:
		ShortSMSWidget(const ShortSMS& shortSms, QWidget* parent = nullptr);

	protected:
		void paintEvent(QPaintEvent* e) override;
};

#endif /* SHORT_SMS_WIDGET_H */
