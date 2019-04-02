#include <QTimer>

#include "focus_plain_text_edit.h"

FocusPlainTextEdit::FocusPlainTextEdit(QWidget* parent)
	: QPlainTextEdit(parent) {
	setFocusPolicy(Qt::StrongFocus);
}

void FocusPlainTextEdit::keyPressEvent(QKeyEvent* e) {
	if (e->key() == Qt::Key_Return) {
		return;
	}

	QPlainTextEdit::keyPressEvent(e);
}

void FocusPlainTextEdit::mouseReleaseEvent(QMouseEvent* e) {
	QPlainTextEdit::mouseReleaseEvent(e);

	if (!rect().contains(e->localPos().toPoint())) {
		return;
	}

	QTimer::singleShot(0, this, [&]() {
		setFocus(Qt::MouseFocusReason);
	});

	emit FocusReceived(this);
}
