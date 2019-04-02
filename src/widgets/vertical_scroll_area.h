#ifndef VERTICAL_SCROLL_AREA_H
#define VERTICAL_SCROLL_AREA_H

#include <QWidget>
#include <QScrollArea>

class VerticalScrollArea : public QScrollArea {
		Q_OBJECT

	public:
		VerticalScrollArea(QWidget* parent = nullptr);

	protected:
		void resizeEvent(QResizeEvent* e) override;
};

#endif /* VERTICAL_SCROLL_AREA_H */
