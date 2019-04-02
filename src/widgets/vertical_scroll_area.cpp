#include <QTimer>
#include <QScrollBar>

#include "vertical_scroll_area.h"

VerticalScrollArea::VerticalScrollArea(QWidget* parent)
	: QScrollArea(parent) {
}

void VerticalScrollArea::resizeEvent(QResizeEvent* e) {
	QScrollArea::resizeEvent(e);

	QTimer::singleShot(0, this, [&]() {
		int newWidth = width();

		if (verticalScrollBar()->isVisible()) {
			newWidth -= verticalScrollBar()->width();
		}

		widget()->setFixedWidth(newWidth);
	});
}
