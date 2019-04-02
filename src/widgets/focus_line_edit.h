#ifndef FOCUS_LINE_EDIT_H
#define FOCUS_LINE_EDIT_H

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

#endif /* FOCUS_LINE_EDIT_H */
