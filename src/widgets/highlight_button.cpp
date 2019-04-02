#include <QFile>
#include <QTimer>

#include "highlight_button.h"

HighlightButton::HighlightButton(QWidget* parent)
	: QPushButton(parent), m_highlight(false) {
	setAutoDefault(false);

	QFile styleFile(":/src/widgets/highlight_button.qss");

	if (!styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
		throw std::runtime_error("Failed to load stylesheet");
	}

	setStyleSheet(styleFile.readAll());

	styleFile.close();
}

bool HighlightButton::HasHighlight() {
	return m_highlight;
}

void HighlightButton::SetHighlight(bool value) {
	if (value == m_highlight) {
		return;
	}

	m_highlight = value;

	RefreshStylesheet();

	QTimer::singleShot(0, this, [&]() {
		setFocus();
	});

	emit HighlightChanged(value);
}

void HighlightButton::RefreshStylesheet() {
	setStyleSheet(styleSheet());
}
