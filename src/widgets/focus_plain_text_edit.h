#ifndef FOCUS_PLAIN_TEXT_EDIT_H
#define FOCUS_PLAIN_TEXT_EDIT_H

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

#endif /* FOCUS_PLAIN_TEXT_EDIT_H */
