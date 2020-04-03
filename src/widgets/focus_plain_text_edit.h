#pragma once

#include <QWidget>
#include <QMouseEvent>
#include <QPlainTextEdit>

class FocusPlainTextEdit : public QPlainTextEdit {
		Q_OBJECT

	public:
		FocusPlainTextEdit(QWidget* parent = nullptr);

	protected:
		void keyPressEvent(QKeyEvent* e) override;
		void mouseReleaseEvent(QMouseEvent* e) override;

	signals:
		void FocusReceived(FocusPlainTextEdit* edit);
};
