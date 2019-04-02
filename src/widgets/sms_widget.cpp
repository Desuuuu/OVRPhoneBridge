#include <QLabel>
#include <QPainter>
#include <QGridLayout>
#include <QStyleOption>

#include "sms_widget.h"

SMSWidget::SMSWidget(const SMS& sms, QWidget* parent)
	: QWidget(parent),
	  m_number(sms.number) {
	setObjectName("SMSWidget");

	QGridLayout* layout = new QGridLayout();

	layout->setSpacing(0);

	QLabel* title;

	if (!sms.name.isEmpty()) {
		title = new QLabel(ellipsize(sms.name, 40), this);
	} else {
		title = new QLabel(ellipsize(sms.number, 40), this);
	}

	title->setObjectName("smsTitle");
	title->setTextFormat(Qt::PlainText);

	layout->addWidget(title, 0, 0);

	QLabel* text;

	if (sms.incoming) {
		text = new QLabel(ellipsize(sms.body, SMS_PREVIEW_MAX_LENGTH), this);
	} else {
		text = new QLabel(QString("You: ") + ellipsize(sms.body, SMS_PREVIEW_MAX_LENGTH), this);
	}

	text->setObjectName("smsText");
	text->setTextFormat(Qt::PlainText);

	layout->addWidget(text, 1, 0);

	QLabel* date = new QLabel(sms.date.toString("MM/dd/yy"), this);
	date->setObjectName("smsDate");
	date->setTextFormat(Qt::PlainText);

	layout->addWidget(date, 0, 1, 2, 1, Qt::AlignVCenter);

	layout->setColumnStretch(0, 1);

	setLayout(layout);
}

const QString& SMSWidget::GetNumber() {
	return m_number;
}

void SMSWidget::paintEvent(QPaintEvent*) {
	QStyleOption opt;
	opt.init(this);
	QPainter p(this);
	style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}
