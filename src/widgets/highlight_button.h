#ifndef HIGHLIGHT_BUTTON_H
#define HIGHLIGHT_BUTTON_H

#include <QWidget>
#include <QPushButton>

class HighlightButton : public QPushButton {
		Q_OBJECT
		Q_PROPERTY(bool highlight READ HasHighlight WRITE SetHighlight NOTIFY HighlightChanged)

	public:
		HighlightButton(QWidget* parent = nullptr);

		bool HasHighlight();
		void SetHighlight(bool value);

	public slots:
		void RefreshStylesheet();

	signals:
		void HighlightChanged(bool newValue);

	private:
		bool m_highlight;
};

#endif /* HIGHLIGHT_BUTTON_H */
