#include <QDir>
#include <QFile>
#include <QTimer>
#include <QString>
#include <QPainter>
#include <QApplication>
#include <QOpenGLFunctions>
#include <QOpenGLPaintDevice>
#include <QGraphicsSceneMouseEvent>

#include <spdlog/spdlog.h>

#include "../common.h"
#include "overlay_controller.h"

using namespace vr;
using namespace OpenVR;

OverlayController::OverlayController(QWidget* widget)
	: QObject(),
	  m_context(nullptr),
	  m_surface(nullptr),
	  m_scene(nullptr),
	  m_frameBuffer(nullptr),
	  m_overlayHandle(k_ulOverlayHandleInvalid),
	  m_overlayThumbnailHandle(k_ulOverlayHandleInvalid),
	  m_widget(widget),
	  m_lastPosition(0, 0),
	  m_lastButtons(Qt::NoButton),
	  m_renderRequested(false),
	  m_overlayVisible(false),
	  m_keyboardVisible(false) {
	if (!VR_IsHmdPresent()) {
		throw std::runtime_error("HMD not found");
	}

	std::string error;

	if (!InitOpenGL(&error)) {
		throw std::runtime_error(error);
	}

	if (!InitVRRuntime(VRApplication_Overlay, &error)) {
		throw std::runtime_error(error);
	}

	if (!VR_IsInterfaceVersionValid(IVROverlay_Version)
			|| !VR_IsInterfaceVersionValid(IVRNotifications_Version)) {
		throw std::runtime_error("Incompatible OpenVR version");
	}

	if (!SetupOverlay(&error)) {
		throw std::runtime_error(error);
	}

	LoadNotificationAssets();

	if (!SetupWidget(&error)) {
		throw std::runtime_error(error);
	}

	QTimer* renderTimer = new QTimer(this);

	connect(renderTimer, &QTimer::timeout, this, [&]() {
		if (m_renderRequested) {
			RenderOverlay();
		}
	});

	renderTimer->setInterval(15);
	renderTimer->start();

	QTimer* eventTimer = new QTimer(this);

	connect(eventTimer, &QTimer::timeout, this, &OverlayController::PollEvents);

	eventTimer->setInterval(20);
	eventTimer->start();
}

OverlayController::~OverlayController() {
	ShutdownVRRuntime();

	if (m_notificationBitmap.m_pImageData != nullptr) {
		delete[] reinterpret_cast<char*>(m_notificationBitmap.m_pImageData);
		m_notificationBitmap.m_pImageData = nullptr;
	}

	CleanupOpenGL();
}

bool OverlayController::InitOpenGL(std::string* error) {
	QSurfaceFormat format;

	format.setVersion(2, 1);
	format.setDepthBufferSize(24);
	format.setStencilBufferSize(8);
	format.setSamples(4);

	m_context.reset(new QOpenGLContext());
	m_context->setFormat(format);

	if (!m_context->create()) {
		if (error != nullptr) {
			*error = "Failed to create OpenGL context";
		}

		return false;
	}

	m_surface.reset(new QOffscreenSurface());
	m_surface->setFormat(m_context->format());
	m_surface->create();

	m_context->makeCurrent(m_surface.get());

	m_scene.reset(new QGraphicsScene());

	connect(m_scene.get(), &QGraphicsScene::changed, this, [&]() {
		m_renderRequested = true;
	});

	return true;
}

void OverlayController::CleanupOpenGL() {
	m_frameBuffer.reset(nullptr);
	m_scene.reset(nullptr);
	m_context.reset(nullptr);
	m_surface.reset(nullptr);
}

bool OverlayController::ShowNotification(const std::string& identifier,
										 const std::string& text,
										 bool sound,
										 bool persistent) {
	IVRNotifications* notifications = VRNotifications();

	if (notifications == nullptr) {
		spdlog::error("ShowNotification: IVRNotifications interface not available");

		return false;
	}

	auto iterator = m_notifications.find(identifier);

	if (iterator != m_notifications.end()) {
		RemoveNotification(identifier);
	}

	VRNotificationId notificationId;

	EVRNotificationError notificationError;

	NotificationBitmap_t* notificationBitmap = nullptr;

	if (m_notificationBitmap.m_pImageData != nullptr) {
		notificationBitmap = &m_notificationBitmap;
	}

	std::string formattedText = text;

	if (formattedText.length() > k_unNotificationTextMaxSize) {
		formattedText = formattedText.substr(0, k_unNotificationTextMaxSize - 3) + "...";
	}

	notificationError = notifications->CreateNotification(
								m_overlayHandle,
								USER_VALUE,
								(persistent ?
								 EVRNotificationType_Persistent
								 : EVRNotificationType_Transient),
								formattedText.c_str(),
								EVRNotificationStyle_Application,
								notificationBitmap,
								&notificationId);

	if (notificationError != VRNotificationError_OK) {
		spdlog::error(std::string("ShowNotification: IVRNotifications: Error ")
					  + std::to_string(notificationError));

		return false;
	}

	if (sound && m_notificationSound.isLoaded()) {
		if (m_notificationSound.isPlaying()) {
			m_notificationSound.stop();
		}

		m_notificationSound.play();
	}

	m_notifications.insert(std::pair<std::string, VRNotificationId>(
								   identifier,
								   notificationId));

	return true;
}

bool OverlayController::RemoveNotification(const std::string& identifier) {
	IVRNotifications* notifications = VRNotifications();

	if (notifications == nullptr) {
		spdlog::error("RemoveNotification: IVRNotifications interface not available");

		return false;
	}

	auto iterator = m_notifications.find(identifier);

	if (iterator == m_notifications.end()) {
		return true;
	}

	EVRNotificationError notificationError;

	notificationError = notifications->RemoveNotification(iterator->second);

	if (notificationError != VRNotificationError_OK
			&& notificationError != VRNotificationError_InvalidNotificationId) {
		spdlog::error(std::string("RemoveNotification: IVRNotifications: Error ")
					  + std::to_string(notificationError));

		return false;
	}

	m_notifications.erase(iterator);

	return true;
}

bool OverlayController::ShowKeyboard(uint8_t identifier,
									 uint32_t maxLen,
									 const char* initialText,
									 const char* description,
									 bool singleLine,
									 bool password) {
	IVROverlay* overlay = VROverlay();

	if (overlay == nullptr) {
		spdlog::error("ShowKeyboard: IVROverlay interface not available");

		return false;
	}

	IVRCompositor* compositor = VRCompositor();

	if (compositor == nullptr) {
		spdlog::error("ShowKeyboard: IVRCompositor interface not available");

		return false;
	}

	EGamepadTextInputMode inputMode = (password ?
									   k_EGamepadTextInputModePassword
									   : k_EGamepadTextInputModeNormal);

	EGamepadTextInputLineMode lineMode = (singleLine ?
										  k_EGamepadTextInputLineModeSingleLine
										  : k_EGamepadTextInputLineModeMultipleLines);

	VROverlayError overlayError;

	overlayError = overlay->ShowKeyboardForOverlay(m_overlayHandle,
												   inputMode,
												   lineMode,
												   KeyboardFlag_Modal,
												   description,
												   maxLen,
												   initialText,
												   (USER_VALUE + identifier));

	if (overlayError != VROverlayError_None) {
		spdlog::error(std::string("ShowKeyboard: IVROverlay: ")
					  + overlay->GetOverlayErrorNameFromEnum(overlayError));

		return false;
	}

	return true;
}

bool OverlayController::HideKeyboard() {
	IVROverlay* overlay = VROverlay();

	if (overlay == nullptr) {
		spdlog::error("HideKeyboard: IVROverlay interface not available");

		return false;
	}

	overlay->HideKeyboard();

	return true;
}

bool OverlayController::SetupOverlay(std::string* error) {
	IVROverlay* overlay = VROverlay();

	if (overlay == nullptr) {
		if (error != nullptr) {
			*error = "IVROverlay interface not available";
		}

		return false;
	}

	VROverlayError overlayError;

	overlayError = overlay->CreateDashboardOverlay(
						   MANIFEST_KEY,
						   OVERLAY_NAME,
						   &m_overlayHandle,
						   &m_overlayThumbnailHandle);

	if (overlayError != VROverlayError_None) {
		if (error != nullptr) {
			*error = std::string("IVROverlay: ")
					 + overlay->GetOverlayErrorNameFromEnum(overlayError);
		}

		return false;
	}

	m_overlayVisible = false;

	overlay->SetOverlayWidthInMeters(m_overlayHandle, 3.0f);
	overlay->SetOverlayInputMethod(m_overlayHandle, VROverlayInputMethod_Mouse);
	overlay->SetOverlayFlag(m_overlayHandle, VROverlayFlags_SendVRSmoothScrollEvents, true);

	QString overlayThumbnailPath = QDir::cleanPath(QDir(qApp->applicationDirPath())
												   .absoluteFilePath(OVERLAY_THUMB_PATH));

	if (QFile::exists(overlayThumbnailPath)) {
		overlayThumbnailPath = QDir::toNativeSeparators(overlayThumbnailPath);

		overlay->SetOverlayFromFile(m_overlayThumbnailHandle,
									overlayThumbnailPath.toStdString().c_str());
	}

	return true;
}

void OverlayController::LoadNotificationAssets() {
	m_notificationSound.setSource(QUrl("qrc:///sounds/notification.wav"));
	m_notificationSound.setLoopCount(0);
	m_notificationSound.setVolume(0.4);

	QImage notificationImage(":/images/notification.png");

	notificationImage = notificationImage.convertToFormat(QImage::Format_RGBA8888);

	qsizetype size = notificationImage.sizeInBytes();

	m_notificationBitmap.m_nWidth = notificationImage.width();
	m_notificationBitmap.m_nHeight = notificationImage.height();
	m_notificationBitmap.m_nBytesPerPixel = 4;
	m_notificationBitmap.m_pImageData = new char[size];

	const char* data = reinterpret_cast<const char*>(notificationImage.constBits());

	for (int i = 0; i < size; ++i) {
		reinterpret_cast<char*>(m_notificationBitmap.m_pImageData)[i] = *(data + i);
	}
}

bool OverlayController::SetupWidget(std::string* error) {
	IVROverlay* overlay = VROverlay();

	if (overlay == nullptr) {
		if (error != nullptr) {
			*error = "IVROverlay interface not available";
		}

		return false;
	}

	m_widget->move(0, 0);
	m_scene->addWidget(m_widget);

	QOpenGLFramebufferObjectFormat bufferFormat;

	bufferFormat.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
	bufferFormat.setTextureTarget(GL_TEXTURE_2D);

	m_frameBuffer.reset(new QOpenGLFramebufferObject(m_widget->width(),
													 m_widget->height(),
													 bufferFormat));

	HmdVector2_t widgetSize = {
		static_cast<float>(m_widget->width()),
		static_cast<float>(m_widget->height())
	};

	overlay->SetOverlayMouseScale(m_overlayHandle, &widgetSize);

	return true;
}

void OverlayController::RenderOverlay() {
	IVROverlay* overlay = VROverlay();

	if (overlay == nullptr
			|| m_overlayHandle == k_ulOverlayHandleInvalid
			|| m_overlayThumbnailHandle == k_ulOverlayHandleInvalid) {
		return;
	}

	if (!overlay->IsDashboardVisible()
			|| (!overlay->IsOverlayVisible(m_overlayHandle)
				&& !overlay->IsOverlayVisible(m_overlayThumbnailHandle))) {
		return;
	}

	m_renderRequested = false;

	m_context->makeCurrent(m_surface.get());
	m_frameBuffer->bind();

	QOpenGLPaintDevice device(m_frameBuffer->size());
	QPainter painter(&device);

	m_scene->render(&painter);
	m_frameBuffer->release();

	GLuint textureId = m_frameBuffer->texture();

	if (textureId != 0) {
		Texture_t texture = {
			reinterpret_cast<void*>(textureId),
			TextureType_OpenGL,
			ColorSpace_Auto
		};

		overlay->SetOverlayTexture(m_overlayHandle, &texture);
	}

	m_context->functions()->glFlush();
}

void OverlayController::PollEvents() {
	IVROverlay* overlay = VROverlay();

	if (overlay == nullptr) {
		return;
	}

	VREvent_t event;

	if (m_overlayHandle != k_ulOverlayHandleInvalid) {
		while (overlay->PollNextOverlayEvent(m_overlayHandle, &event, sizeof(event))) {
			switch (event.eventType) {
				case VREvent_MouseMove: {
					QPointF newPosition(static_cast<qreal>(event.data.mouse.x),
										static_cast<qreal>(event.data.mouse.y));

					QPoint newPositionInt = newPosition.toPoint();
					QPoint lastPositionInt = m_lastPosition.toPoint();

					QGraphicsSceneMouseEvent mouseEvent(QEvent::GraphicsSceneMouseMove);

					mouseEvent.setWidget(nullptr);
					mouseEvent.setPos(newPosition);
					mouseEvent.setScenePos(newPositionInt);
					mouseEvent.setScreenPos(newPositionInt);
					mouseEvent.setLastPos(m_lastPosition);
					mouseEvent.setLastScenePos(lastPositionInt);
					mouseEvent.setLastScreenPos(lastPositionInt);
					mouseEvent.setButtons(m_lastButtons);
					mouseEvent.setButton(Qt::NoButton);
					mouseEvent.setModifiers(Qt::NoModifier);
					mouseEvent.setAccepted(false);

					m_lastPosition = newPosition;

					qApp->sendEvent(m_scene.get(), &mouseEvent);

					break;
				}

				case VREvent_MouseButtonDown: {
					QPointF newPosition(static_cast<qreal>(event.data.mouse.x),
										static_cast<qreal>(event.data.mouse.y));

					QPoint newPositionInt = newPosition.toPoint();
					QPoint lastPositionInt = m_lastPosition.toPoint();

					Qt::MouseButton button = (event.data.mouse.button == VRMouseButton_Right) ?
											 Qt::RightButton
											 : Qt::LeftButton;

					m_lastButtons |= button;

					QGraphicsSceneMouseEvent mouseEvent(QEvent::GraphicsSceneMousePress);

					mouseEvent.setWidget(nullptr);
					mouseEvent.setPos(newPosition);
					mouseEvent.setScenePos(newPositionInt);
					mouseEvent.setScreenPos(newPositionInt);
					mouseEvent.setButtonDownPos(button, newPosition);
					mouseEvent.setButtonDownScenePos(button, newPositionInt);
					mouseEvent.setButtonDownScreenPos(button, newPositionInt);
					mouseEvent.setLastPos(m_lastPosition);
					mouseEvent.setLastScenePos(lastPositionInt);
					mouseEvent.setLastScreenPos(lastPositionInt);
					mouseEvent.setButtons(m_lastButtons);
					mouseEvent.setButton(button);
					mouseEvent.setModifiers(Qt::NoModifier);
					mouseEvent.setAccepted(false);

					m_lastPosition = newPosition;

					qApp->sendEvent(m_scene.get(), &mouseEvent);

					break;
				}

				case VREvent_MouseButtonUp: {
					QPointF newPosition(static_cast<qreal>(event.data.mouse.x),
										static_cast<qreal>(event.data.mouse.y));

					QPoint newPositionInt = newPosition.toPoint();
					QPoint lastPositionInt = m_lastPosition.toPoint();

					Qt::MouseButton button = (event.data.mouse.button == VRMouseButton_Right) ?
											 Qt::RightButton
											 : Qt::LeftButton;

					m_lastButtons &= ~button;

					QGraphicsSceneMouseEvent mouseEvent(QEvent::GraphicsSceneMouseRelease);

					mouseEvent.setWidget(nullptr);
					mouseEvent.setPos(newPosition);
					mouseEvent.setScenePos(newPositionInt);
					mouseEvent.setScreenPos(newPositionInt);
					mouseEvent.setLastPos(m_lastPosition);
					mouseEvent.setLastScenePos(lastPositionInt);
					mouseEvent.setLastScreenPos(lastPositionInt);
					mouseEvent.setButtons(m_lastButtons);
					mouseEvent.setButton(button);
					mouseEvent.setModifiers(Qt::NoModifier);
					mouseEvent.setAccepted(false);

					m_lastPosition = newPosition;

					qApp->sendEvent(m_scene.get(), &mouseEvent);

					break;
				}

				case VREvent_ScrollSmooth: {
					QPoint scrollDelta(static_cast<int>(event.data.scroll.xdelta * SCROLL_SPEED),
									   static_cast<int>(event.data.scroll.ydelta * SCROLL_SPEED));

					QPoint lastPositionInt = m_lastPosition.toPoint();

					QGraphicsSceneWheelEvent wheelEvent(QEvent::GraphicsSceneWheel);

					wheelEvent.setWidget(nullptr);
					wheelEvent.setPos(m_lastPosition);
					wheelEvent.setScenePos(lastPositionInt);
					wheelEvent.setScreenPos(lastPositionInt);

					if (scrollDelta.y() != 0) {
						wheelEvent.setDelta(scrollDelta.y());
						wheelEvent.setOrientation(Qt::Vertical);
					} else {
						wheelEvent.setDelta(scrollDelta.x());
						wheelEvent.setOrientation(Qt::Horizontal);
					}

					wheelEvent.setButtons(m_lastButtons);
					wheelEvent.setModifiers(Qt::NoModifier);
					wheelEvent.setAccepted(false);

					qApp->sendEvent(m_scene.get(), &wheelEvent);

					break;
				}

				case VREvent_Notification_BeginInteraction: {
					if (event.data.notification.ulUserValue == USER_VALUE) {
						auto iterator = m_notifications.begin();

						while (iterator != m_notifications.end()) {
							if (iterator->second == event.data.notification.notificationId) {
								emit NotificationOpened(iterator->first);

								m_notifications.erase(iterator);

								break;
							}

							++iterator;
						}
					}

					break;
				}

				case VREvent_Notification_Destroyed: {
					if (event.data.notification.ulUserValue == USER_VALUE) {
						auto iterator = m_notifications.begin();

						while (iterator != m_notifications.end()) {
							if (iterator->second == event.data.notification.notificationId) {
								m_notifications.erase(iterator);

								break;
							}

							++iterator;
						}
					}

					break;
				}

				case VREvent_KeyboardDone: {
					m_keyboardVisible = false;

					if (event.data.keyboard.uUserValue >= USER_VALUE
							&& event.data.keyboard.uUserValue <= USER_VALUE + 255) {
						IVROverlay* overlay = VROverlay();

						if (overlay == nullptr) {
							spdlog::error("KeyboardDone: IVROverlay interface not available");

							break;
						}

						uint8_t identifier = static_cast<uint8_t>(event.data.keyboard.uUserValue
																  - USER_VALUE);

						char buffer[256];

						uint32_t inputLen = overlay->GetKeyboardText(buffer, sizeof(buffer));

						emit KeyboardData(identifier, std::string(buffer, inputLen));
					}

					break;
				}

				case VREvent_KeyboardClosed:
					m_keyboardVisible = false;
					break;

				case VREvent_OverlayShown:
					m_renderRequested = true;

					if (!m_overlayVisible) {
						m_overlayVisible = true;

						emit OverlayShown();
					}

					break;

				case VREvent_OverlayHidden:
					if (m_overlayVisible) {
						m_overlayVisible = false;

						emit OverlayHidden();
					}

					break;

				case VREvent_Quit:
				case VREvent_DriverRequestedQuit:
					qApp->quit();
					break;
			}
		}
	}

	if (m_overlayThumbnailHandle != k_ulOverlayHandleInvalid) {
		while (overlay->PollNextOverlayEvent(m_overlayThumbnailHandle, &event, sizeof(event))) {
			switch (event.eventType) {
				case VREvent_OverlayShown:
					m_renderRequested = true;
					break;
			}
		}
	}
}

bool OverlayController::InitVRRuntime(EVRApplicationType applicationType,
									  std::string* error) {
	HmdError hmdError = VRInitError_None;

	VR_Init(&hmdError, applicationType);

	if (hmdError != VRInitError_None) {
		if (error != nullptr) {
			*error = std::string("Failed to initialize OpenVR runtime: ")
					 + VR_GetVRInitErrorAsEnglishDescription(hmdError);
		}

		return false;
	}

	spdlog::info("OpenVR runtime initialized");

	return true;
}

void OverlayController::ShutdownVRRuntime() {
	VR_Shutdown();

	spdlog::info("VR runtime shutdown");
}
