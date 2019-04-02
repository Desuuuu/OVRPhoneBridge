#include <QTimer>

#include "focus_line_edit.h"

FocusLineEdit::FocusLineEdit(QWidget* parent)
	: QLineEdit(parent) {
	setFocusPolicy(Qt::StrongFocus);
}

void FocusLineEdit::mouseReleaseEvent(QMouseEvent* e) {
	QLineEdit::mouseReleaseEvent(e);

	if (!rect().contains(e->localPos().toPoint())) {
		return;
	}

	QTimer::singleShot(0, this, [&]() {
		setFocus(Qt::MouseFocusReason);
	});

	emit FocusReceived(this);
}
