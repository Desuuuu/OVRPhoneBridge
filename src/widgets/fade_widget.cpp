#include <QGraphicsOpacityEffect>

#include "fade_widget.h"
#include "../common.h"

FadeWidget::FadeWidget(QWidget* parent)
	: QWidget(parent) {
}

FadeWidget::~FadeWidget() {
	for (auto pair : m_animations) {
		pair.first->deleteLater();
	}

	m_animations.clear();
}

void FadeWidget::CancelAnimation(QWidget* widget) {
	if (widget != nullptr && m_animations.contains(widget)) {
		auto pair = m_animations.value(widget);

		m_animations.remove(widget);

		pair.first->stop();
		pair.first->deleteLater();

		if (pair.second != nullptr) {
			pair.second(true);
		}
	}
}

void FadeWidget::CancelAnimations() {
	QVector<std::function<void(bool)>> doneVector;

	for (auto pair : m_animations) {
		pair.first->stop();
		pair.first->deleteLater();

		doneVector.append(pair.second);
	}

	m_animations.clear();

	for (auto done : doneVector) {
		if (done != nullptr) {
			done(true);
		}
	}
}

void FadeWidget::OnAnimationFinished() {
	QPropertyAnimation* source = qobject_cast<QPropertyAnimation*>(sender());

	if (source != nullptr) {
		std::function<void(bool)> done = nullptr;

		auto iterator = m_animations.begin();

		while (iterator != m_animations.end()) {
			if (source == iterator.value().first) {
				if (iterator.value().first->objectName() == "fadeOut") {
					iterator.key()->setVisible(false);
				}

				iterator.value().first->deleteLater();

				done = iterator.value().second;

				m_animations.erase(iterator);

				break;
			}

			++iterator;
		}

		if (done != nullptr) {
			done(false);
		}
	}
}

void FadeWidget::FadeIn(QWidget* widget, int duration, const QEasingCurve& easing, std::function<void(bool)> done) {
	if (widget == nullptr) {
		if (done != nullptr) {
			done(false);
		}

		return;
	}

	CancelAnimation(widget);

	QGraphicsOpacityEffect* effect = qobject_cast<QGraphicsOpacityEffect*>(widget->graphicsEffect());

	if (effect == nullptr) {
		widget->setVisible(true);

		if (done != nullptr) {
			done(false);
		}

		return;
	}

	if (widget->isVisible() && effect->opacity() >= OPACITY_EFFECT_MAX) {
		if (done != nullptr) {
			done(false);
		}

		return;
	}

	QPropertyAnimation* animation = new QPropertyAnimation(effect, "opacity");

	m_animations.insert(widget, QPair<QPropertyAnimation*, std::function<void(bool)>>(animation, done));

	connect(animation, &QPropertyAnimation::finished, this, &FadeWidget::OnAnimationFinished);

	animation->setObjectName("fadeIn");
	animation->setDuration(duration);
	animation->setStartValue(widget->isVisible() ? effect->opacity() : 0);
	animation->setEndValue(OPACITY_EFFECT_MAX);
	animation->setEasingCurve(easing);
	animation->start();

	widget->setVisible(true);
	widget->repaint();
}

void FadeWidget::FadeOut(QWidget* widget, int duration, const QEasingCurve& easing, std::function<void(bool)> done) {
	if (widget == nullptr) {
		if (done != nullptr) {
			done(false);
		}

		return;
	}

	CancelAnimation(widget);

	QGraphicsOpacityEffect* effect = qobject_cast<QGraphicsOpacityEffect*>(widget->graphicsEffect());

	if (effect == nullptr || effect->opacity() <= 0) {
		widget->setVisible(false);

		if (done != nullptr) {
			done(false);
		}

		return;
	}

	if (!widget->isVisible()) {
		if (done != nullptr) {
			done(false);
		}

		return;
	}

	QPropertyAnimation* animation = new QPropertyAnimation(effect, "opacity");

	m_animations.insert(widget, QPair<QPropertyAnimation*, std::function<void(bool)>>(animation, done));

	connect(animation, &QPropertyAnimation::finished, this, &FadeWidget::OnAnimationFinished);

	animation->setObjectName("fadeOut");
	animation->setDuration(duration);
	animation->setStartValue(effect->opacity());
	animation->setEndValue(0);
	animation->setEasingCurve(easing);
	animation->start();
}

void FadeWidget::FadeOutIn(QWidget* widgetOut, QWidget* widgetIn, int durationOut, int durationIn, const QEasingCurve& easingOut, const QEasingCurve& easingIn, std::function<void(bool)> done) {
	CancelAnimation(widgetIn);
	CancelAnimation(widgetOut);

	FadeOut(widgetOut, durationOut, easingOut, [ = ](bool aborted) {
		if (aborted) {
			if (done != nullptr) {
				done(aborted);
			}

			return;
		}

		FadeIn(widgetIn, durationIn, easingIn, done);
	});
}
