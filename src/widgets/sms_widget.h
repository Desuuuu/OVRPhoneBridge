#ifndef SMS_WIDGET_H
#define SMS_WIDGET_H

#include <QWidget>
#include <QMouseEvent>

#include "../common.h"

class SMSWidget : public QWidget {
		Q_OBJECT

	public:
		SMSWidget(const SMS& sms, QWidget* parent = nullptr);

		const QString& GetNumber();

	protected:
		void paintEvent(QPaintEvent* e) override;

	private:
		QString m_number;
};

#endif /* SMS_WIDGET_H */
