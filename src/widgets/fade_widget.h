#pragma once

#include <functional>

#include <QHash>
#include <QWidget>
#include <QSharedPointer>
#include <QPropertyAnimation>

class FadeWidget : public QWidget {
		Q_OBJECT

	public:
		FadeWidget(QWidget* parent = nullptr);
		~FadeWidget();

	protected:
		void FadeIn(QWidget* widget, int duration, const QEasingCurve& easing = QEasingCurve::Linear, std::function<void(bool)> done = nullptr);
		void FadeOut(QWidget* widget, int duration, const QEasingCurve& easing = QEasingCurve::Linear, std::function<void(bool)> done = nullptr);
		void FadeOutIn(QWidget* widgetOut, QWidget* widgetIn, int durationOut, int durationIn, const QEasingCurve& easingOut = QEasingCurve::Linear, const QEasingCurve& easingIn = QEasingCurve::Linear, std::function<void(bool)> done = nullptr);

	protected slots:
		void CancelAnimation(QWidget* widget);
		void CancelAnimations();

	private slots:
		void OnAnimationFinished();

	private:
		QHash<QWidget*, QPair<QSharedPointer<QPropertyAnimation>, std::function<void(bool)>>> m_animations;
};
