#pragma once

#include <map>

#include <QObject>
#include <QWidget>
#include <QPointF>
#include <QSoundEffect>
#include <QOpenGLContext>
#include <QGraphicsScene>
#include <QOffscreenSurface>
#include <QOpenGLFramebufferObject>

#include <openvr.h>

namespace OpenVR {
	class OverlayController : public QObject {
			Q_OBJECT

		public:
			OverlayController(QWidget* widget);
			~OverlayController();

			static bool InitVRRuntime(vr::EVRApplicationType applicationType,
									  std::string* error = nullptr);
			static void ShutdownVRRuntime();

		public slots:
			bool ShowNotification(const std::string& identifier,
								  const std::string& text,
								  bool sound = true,
								  bool persistent = false);
			bool RemoveNotification(const std::string& identifier);
			bool ShowKeyboard(uint8_t identifier,
							  uint32_t maxLen,
							  const char* initialText = nullptr,
							  const char* description = nullptr,
							  bool singleLine = true,
							  bool password = false);
			bool UpdateKeyboardPosition();
			bool HideKeyboard();

		private slots:
			void RenderOverlay();
			void PollEvents();

		signals:
			void OverlayShown();
			void OverlayHidden();
			void NotificationOpened(const std::string& identifier);
			void KeyboardData(uint8_t identifier, const std::string& data);

		private:
			static const unsigned int USER_VALUE = 850486;

			QScopedPointer<QOpenGLContext> m_context;
			QScopedPointer<QOffscreenSurface> m_surface;
			QScopedPointer<QGraphicsScene> m_scene;
			QScopedPointer<QOpenGLFramebufferObject> m_frameBuffer;

			vr::VROverlayHandle_t m_overlayHandle;
			vr::VROverlayHandle_t m_overlayThumbnailHandle;

			QSoundEffect m_notificationSound;
			vr::NotificationBitmap_t m_notificationBitmap;

			QWidget* m_widget;

			QPointF m_lastPosition;
			Qt::MouseButtons m_lastButtons;

			std::map<std::string, vr::VRNotificationId> m_notifications;
			bool m_renderRequested;
			bool m_overlayVisible;
			bool m_keyboardVisible;

			bool InitOpenGL(std::string* error = nullptr);
			bool SetupOverlay(std::string* error = nullptr);
			void LoadNotificationAssets();
			bool SetupWidget(std::string* error = nullptr);
	};
};
