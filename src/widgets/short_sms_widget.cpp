#include <QLabel>
#include <QPainter>
#include <QVBoxLayout>
#include <QStyleOption>

#include "short_sms_widget.h"

ShortSMSWidget::ShortSMSWidget(const ShortSMS& shortSms, QWidget* parent)
	: QWidget(parent) {
	if (shortSms.incoming) {
		setObjectName("ShortSMSInWidget");
	} else {
		setObjectName("ShortSMSOutWidget");
	}

	QVBoxLayout* layout = new QVBoxLayout();

	layout->setSpacing(0);

	QLabel* text = new QLabel(shortSms.body, this);

	if (shortSms.incoming) {
		text->setObjectName("shortSmsInText");
	} else {
		text->setObjectName("shortSmsOutText");
	}

	text->setTextFormat(Qt::PlainText);

	text->ensurePolished();

	if (text->fontMetrics().boundingRect(text->text()).width() > 500) {
		text->setFixedWidth(500);
		text->setWordWrap(true);
		text->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	}

	layout->addWidget(text);

	setLayout(layout);
}

void ShortSMSWidget::paintEvent(QPaintEvent*) {
	QStyleOption opt;
	opt.init(this);
	QPainter p(this);
	style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}
