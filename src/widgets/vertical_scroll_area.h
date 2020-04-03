#pragma once

#include <QWidget>
#include <QScrollArea>

class VerticalScrollArea : public QScrollArea {
		Q_OBJECT

	public:
		VerticalScrollArea(QWidget* parent = nullptr);

	protected:
		void resizeEvent(QResizeEvent* e) override;
};
