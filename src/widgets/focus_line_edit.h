#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QMouseEvent>

class FocusLineEdit : public QLineEdit {
		Q_OBJECT

	public:
		FocusLineEdit(QWidget* parent = nullptr);

	protected:
		void mouseReleaseEvent(QMouseEvent* e) override;

	signals:
		void FocusReceived(FocusLineEdit* edit);
};
