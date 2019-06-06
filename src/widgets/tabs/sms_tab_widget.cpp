#include <QTimer>
#include <QScrollBar>
#include <QSvgWidget>
#include <QSvgRenderer>

#include "sms_tab_widget.h"
#include "ui_sms_tab_widget.h"

#include "../sms_widget.h"
#include "../short_sms_widget.h"

SMSTabWidget::SMSTabWidget(QWidget* parent)
	: QWidget(parent),
	  ui(new Ui::SMSTabWidget),
	  m_tab(Tab::NONE),
	  m_serverState(ServerState::NONE),
	  m_featureEnabled(false),
	  m_retryTimer(nullptr),
	  m_currentNumber(QString::null),
	  m_currentName(QString::null),
	  m_contentLoaded(false),
	  m_contentEmpty(true),
	  m_nextPage(0),
	  m_savedScroll(0) {
	ui->setupUi(this);

	QSvgWidget* loader = new QSvgWidget(":/images/loading.svg", this);
	loader->renderer()->setFramesPerSecond(60);
	loader->setFixedSize(70, 70);

	ui->loadingLayout->insertWidget(0, loader, 0, Qt::AlignHCenter);

	m_retryTimer = new QTimer(this);

	connect(m_retryTimer, &QTimer::timeout, this, &SMSTabWidget::LoadContent);

	m_retryTimer->setInterval(60000);
	m_retryTimer->setSingleShot(true);

	ui->statusLabel->setVisible(false);
	ui->loadingWidget->setVisible(false);
	ui->contentWidget->setVisible(false);
}

SMSTabWidget::~SMSTabWidget() {
	delete ui;
}

void SMSTabWidget::SetFeatureEnabled(bool enabled) {
	m_featureEnabled = enabled;
}

void SMSTabWidget::CurrentTabChanged(const Tab& tab) {
	m_tab = tab;

	if (m_serverState == ServerState::CONNECTED && m_tab == Tab::SMS) {
		QTimer::singleShot(LOAD_DELAY, this, &SMSTabWidget::LoadContent);
	}
}

void SMSTabWidget::ServerStateChanged(const ServerState& state) {
	m_serverState = state;

	setUpdatesEnabled(false);

	ClearContent();

	m_currentNumber = QString::null;
	m_currentName = QString::null;

	UpdateLayout();

	if (m_serverState == ServerState::CONNECTED && m_tab == Tab::SMS) {
		QTimer::singleShot(LOAD_DELAY, this, &SMSTabWidget::LoadContent);
	}
}

bool SMSTabWidget::eventFilter(QObject* obj, QEvent* e) {
	if (e->type() == QEvent::MouseButtonPress) {
		return true;
	}

	if (e->type() == QEvent::MouseButtonRelease) {
		SMSWidget* widget = qobject_cast<SMSWidget*>(obj);

		QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(e);

		if (widget != nullptr
				&& mouseEvent->button() == Qt::LeftButton
				&& widget->rect().contains(mouseEvent->localPos().toPoint())) {
			OpenThread(widget->GetNumber());
		}

		return true;
	}

	return QWidget::eventFilter(obj, e);
}

void SMSTabWidget::UpdateLayout() {
	setUpdatesEnabled(false);

	switch (m_serverState) {
		case ServerState::STOPPED:
			ui->loadingWidget->setVisible(false);
			ui->contentWidget->setVisible(false);

			ui->statusLabel->setText("<font color='#8a2929'>Server not running</font>");
			ui->statusLabel->setVisible(true);
			break;

		case ServerState::STARTED:
			ui->loadingWidget->setVisible(false);
			ui->contentWidget->setVisible(false);

			ui->statusLabel->setText("No client connected");
			ui->statusLabel->setVisible(true);
			break;

		case ServerState::CONNECTED:
			if (!m_featureEnabled) {
				ui->loadingWidget->setVisible(false);
				ui->contentWidget->setVisible(false);

				ui->statusLabel->setText("Feature disabled");
				ui->statusLabel->setVisible(true);
			} else if (!m_contentLoaded) {
				ui->statusLabel->setVisible(false);
				ui->contentWidget->setVisible(false);

				ui->loadingWidget->setVisible(true);
			} else if (m_contentEmpty) {
				ui->loadingWidget->setVisible(false);
				ui->contentWidget->setVisible(false);

				ui->statusLabel->setText("No SMS");
				ui->statusLabel->setVisible(true);
			} else {
				ui->statusLabel->setVisible(false);
				ui->loadingWidget->setVisible(false);

				if (m_currentNumber.isEmpty()) {
					ui->backButton->setVisible(false);
					ui->titleLabel->setVisible(false);

					ui->loadMoreButton->setVisible(false);

					ui->inputWidget->setVisible(false);

					ui->contentWidget->setVisible(true);
				} else {
					ui->backButton->setVisible(true);

					ui->titleLabel->setText(m_currentName.isEmpty()
											? m_currentNumber
											: m_currentName);
					ui->titleLabel->setVisible(true);

					ui->loadMoreButton->setVisible(m_nextPage > 0);

					ui->inputWidget->setVisible(true);

					ui->contentWidget->setVisible(true);
				}
			}

			break;

		default:
			break;
	}

	setUpdatesEnabled(true);
}

void SMSTabWidget::LoadContent() {
	m_retryTimer->stop();

	if (m_contentLoaded
			|| m_tab != Tab::SMS
			|| m_serverState != ServerState::CONNECTED) {
		return;
	}

	if (m_currentNumber.isEmpty()) {
		emit ListSMS();
	} else {
		emit ListSMSFrom(m_currentNumber, 0);
	}

	m_retryTimer->start();
}

void SMSTabWidget::ClearContent() {
	QLayoutItem* item;

	int i = ui->contentScrollLayout->count();

	while (--i >= 1) {
		item = ui->contentScrollLayout->itemAt(i);

		if (item == nullptr) {
			ui->contentScrollLayout->takeAt(i);

			continue;
		}

		if (item->spacerItem() != nullptr) {
			continue;
		}

		if (item->widget() != nullptr) {
			item->widget()->deleteLater();
		}

		if (item->layout() != nullptr) {
			item->layout()->deleteLater();
		}

		delete ui->contentScrollLayout->takeAt(i);
	}

	ui->inputTextEdit->setPlainText("");

	m_contentLoaded = false;
	m_contentEmpty = true;
	m_nextPage = 0;
}

void SMSTabWidget::InsertSMS(const SMS& sms) {
	if (!m_currentNumber.isEmpty()) {
		return;
	}

	SMSWidget* widget = new SMSWidget(sms, ui->contentScrollWidget);

	widget->installEventFilter(this);

	ui->contentScrollLayout->insertWidget(1, widget);
}

void SMSTabWidget::InsertShortSMS(const ShortSMS& shortSms) {
	if (m_currentNumber.isEmpty()) {
		return;
	}

	ShortSMSWidget* widget = new ShortSMSWidget(shortSms, ui->contentScrollWidget);

	if (shortSms.incoming) {
		ui->contentScrollLayout->insertWidget(2, widget, 0, Qt::AlignLeft);
	} else {
		ui->contentScrollLayout->insertWidget(2, widget, 0, Qt::AlignRight);
	}
}

void SMSTabWidget::VRKeyboardData(uint8_t identifier, const std::string& data) {
	if (!m_contentLoaded || m_currentNumber.isEmpty()) {
		return;
	}

	if (identifier == KEYBOARD_INPUT_SMS) {
		std::string input = trim(data);

		if (input.length() > SMS_MAX_LENGTH) {
			input = input.substr(0, SMS_MAX_LENGTH);
		}

		ui->inputTextEdit->setPlainText(input.c_str());
	}
}

void SMSTabWidget::SMSList(const std::list<SMS>& list) {
	m_retryTimer->stop();

	if (m_contentLoaded) {
		return;
	}

	setUpdatesEnabled(false);

	ClearContent();

	m_currentNumber = QString::null;
	m_currentName = QString::null;

	auto iterator = list.rbegin();

	while (iterator != list.rend()) {
		InsertSMS(*iterator);

		m_contentEmpty = false;

		++iterator;
	}

	m_contentLoaded = true;

	UpdateLayout();

	ui->contentScrollArea->verticalScrollBar()->setValue(0);
}

void SMSTabWidget::SMSFromList(const QString& number,
							   const QString& name,
							   int page,
							   const std::list<ShortSMS>& list) {
	m_retryTimer->stop();

	if (page <= 0) {
		if (m_contentLoaded) {
			return;
		}

		setUpdatesEnabled(false);

		ClearContent();

		page = 0;
	} else if (page != m_nextPage) {
		return;
	} else {
		setUpdatesEnabled(false);
	}

	m_contentEmpty = true;

	m_currentNumber = number;
	m_currentName = name;

	m_savedScroll = ui->contentScrollArea->verticalScrollBar()->maximum()
					- ui->contentScrollArea->verticalScrollBar()->value();

	auto iterator = list.begin();

	while (iterator != list.end()) {
		InsertShortSMS(*iterator);

		m_contentEmpty = false;

		++iterator;
	}

	if (m_contentEmpty) {
		if (page == 0) {
			QLabel* emptyLabel = new QLabel("No SMS", ui->contentScrollWidget);

			ui->contentScrollLayout->insertWidget(1, emptyLabel, 1, Qt::AlignHCenter);
		}

		m_nextPage = 0;
	} else {
		m_nextPage = page + 1;
	}

	m_contentEmpty = false;
	m_contentLoaded = true;

	if (m_nextPage == 1) {
		m_savedScroll = 0;
	} else if (m_nextPage < 1) {
		m_savedScroll = -1;
	}

	if (m_nextPage > SMS_MAX_PAGE) {
		m_nextPage = 0;
	}

	UpdateLayout();

	if (m_savedScroll >= 0) {
		QTimer::singleShot(5, this, [&]() {
			ui->contentScrollArea->verticalScrollBar()->setValue(
					ui->contentScrollArea->verticalScrollBar()->maximum() - m_savedScroll);
		});
	}
}

void SMSTabWidget::SMSSent(const QString& number, bool success) {
	if (!m_contentLoaded || m_currentNumber.isEmpty()) {
		return;
	}

	if (m_currentNumber == number && success) {
		ClearContent();

		UpdateLayout();

		QTimer::singleShot(LOAD_DELAY, this, &SMSTabWidget::LoadContent);
	}
}

void SMSTabWidget::OpenThread(const QString& number) {
	setUpdatesEnabled(false);

	ClearContent();

	m_currentNumber = number;

	UpdateLayout();

	QTimer::singleShot(LOAD_DELAY, this, &SMSTabWidget::LoadContent);
}

void SMSTabWidget::on_backButton_clicked() {
	setUpdatesEnabled(false);

	ClearContent();

	m_currentNumber = QString::null;
	m_currentName = QString::null;

	UpdateLayout();

	QTimer::singleShot(LOAD_DELAY, this, &SMSTabWidget::LoadContent);
}

void SMSTabWidget::on_refreshButton_clicked() {
	setUpdatesEnabled(false);

	ClearContent();

	UpdateLayout();

	QTimer::singleShot(LOAD_DELAY, this, &SMSTabWidget::LoadContent);
}

void SMSTabWidget::on_loadMoreButton_clicked() {
	if (!m_contentLoaded || m_contentEmpty || m_currentNumber.isEmpty()) {
		return;
	}

	if (m_nextPage > 0) {
		emit ListSMSFrom(m_currentNumber, m_nextPage);
	}
}

void SMSTabWidget::on_inputTextEdit_FocusReceived(FocusPlainTextEdit* edit) {
	if (!m_contentLoaded || m_currentNumber.isEmpty()) {
		return;
	}

	emit ShowVRKeyboard(KEYBOARD_INPUT_SMS,
						SMS_MAX_LENGTH,
						edit->toPlainText().toStdString().c_str(),
						"SMS",
						true,
						false);
}

void SMSTabWidget::on_sendButton_clicked() {
	if (!m_contentLoaded || m_currentNumber.isEmpty()) {
		return;
	}

	QString body = ui->inputTextEdit->toPlainText();

	if (!body.isEmpty() && body.length() <= SMS_MAX_LENGTH) {
		ui->inputTextEdit->setPlainText("");

		emit SendSMS(m_currentNumber, body);
	}
}
