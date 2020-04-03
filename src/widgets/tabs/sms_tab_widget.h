#pragma once

#include <QWidget>

#include "../../common.h"
#include "../focus_plain_text_edit.h"

#define LOAD_DELAY  500

namespace Ui {
	class SMSTabWidget;
}

class SMSTabWidget : public QWidget {
		Q_OBJECT

	public:
		SMSTabWidget(QWidget* parent = nullptr);
		~SMSTabWidget() override;

		void SetFeatureEnabled(bool enabled);

	public slots:
		void CurrentTabChanged(const Tab& tab);
		void ServerStateChanged(const ServerState& state);

		void VRKeyboardData(uint8_t identifier, const std::string& data);

		void SMSList(const std::list<SMS>& list);
		void SMSFromList(const QString& number,
						 const QString& name,
						 int page,
						 const std::list<ShortSMS>& list);
		void SMSSent(const QString& number, bool success);

		void OpenThread(const QString& number);

		void on_backButton_clicked();
		void on_refreshButton_clicked();
		void on_loadMoreButton_clicked();
		void on_inputTextEdit_FocusReceived(FocusPlainTextEdit* edit);
		void on_sendButton_clicked();

	protected:
		bool eventFilter(QObject* obj, QEvent* e) override;

	signals:
		void ShowVRKeyboard(uint8_t identifier,
							uint32_t maxLen,
							const char* initialText = nullptr,
							const char* description = nullptr,
							bool singleLine = true,
							bool password = false);

		void ListSMS();
		void ListSMSFrom(const QString& number, int page = 0);
		void SendSMS(const QString& destination, const QString& body);

	private:
		Ui::SMSTabWidget* ui;

		Tab m_tab;
		ServerState m_serverState;
		bool m_featureEnabled;

		QTimer* m_retryTimer;

		QString m_currentNumber;
		QString m_currentName;
		bool m_contentLoaded;
		bool m_contentEmpty;
		int m_nextPage;
		int m_savedScroll;

		void UpdateLayout();

		void LoadContent();
		void ClearContent();

		void InsertSMS(const SMS& sms);
		void InsertShortSMS(const ShortSMS& shortSms);
};
