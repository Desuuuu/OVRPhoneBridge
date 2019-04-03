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

#include "vr_overlay_controller.h"
#include "common.h"

VROverlayController::VROverlayController(QWidget* widget)
	: QObject(),
	  m_context(nullptr),
	  m_surface(nullptr),
	  m_scene(nullptr),
	  m_frameBuffer(nullptr),
	  m_overlayHandle(vr::k_ulOverlayHandleInvalid),
	  m_overlayThumbnailHandle(vr::k_ulOverlayHandleInvalid),
	  m_widget(widget),
	  m_lastPosition(0, 0),
	  m_lastButtons(Qt::NoButton),
	  m_renderRequested(false),
	  m_overlayVisible(false) {
	if (!vr::VR_IsHmdPresent()) {
		throw std::runtime_error("HMD not found");
	}

	std::string error;

	if (!InitOpenGL(&error)) {
		throw std::runtime_error(error);
	}

	if (!InitVRRuntime(vr::VRApplication_Overlay, &error)) {
		throw std::runtime_error(error);
	}

	if (!vr::VR_IsInterfaceVersionValid(vr::IVROverlay_Version)
			|| !vr::VR_IsInterfaceVersionValid(vr::IVRNotifications_Version)) {
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

	connect(eventTimer, &QTimer::timeout, this, &VROverlayController::PollEvents);

	eventTimer->setInterval(20);
	eventTimer->start();
}

VROverlayController::~VROverlayController() {
	ShutdownVRRuntime();

	if (m_notificationBitmap.m_pImageData != nullptr) {
		delete[] reinterpret_cast<char*>(m_notificationBitmap.m_pImageData);
		m_notificationBitmap.m_pImageData = nullptr;
	}

	CleanupOpenGL();
}

bool VROverlayController::InitOpenGL(std::string* error) {
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

void VROverlayController::CleanupOpenGL() {
	m_frameBuffer.reset(nullptr);
	m_scene.reset(nullptr);
	m_context.reset(nullptr);
	m_surface.reset(nullptr);
}

bool VROverlayController::ShowNotification(const std::string& identifier,
										   const std::string& text,
										   bool sound,
										   bool persistent) {
	vr::IVRNotifications* notifications = vr::VRNotifications();

	if (notifications == nullptr) {
		spdlog::error("ShowNotification: IVRNotifications interface not available");

		return false;
	}

	auto iterator = m_notifications.find(identifier);

	if (iterator != m_notifications.end()) {
		RemoveNotification(identifier);
	}

	vr::VRNotificationId notificationId;

	vr::EVRNotificationError notificationError;

	vr::NotificationBitmap_t* notificationBitmap = nullptr;

	if (m_notificationBitmap.m_pImageData != nullptr) {
		notificationBitmap = &m_notificationBitmap;
	}

	std::string formattedText = text;

	if (formattedText.length() > vr::k_unNotificationTextMaxSize) {
		formattedText = formattedText.substr(0, vr::k_unNotificationTextMaxSize - 3) + "...";
	}

	notificationError = notifications->CreateNotification(
								m_overlayHandle,
								VR_CONTROLLER_USERVALUE,
								(persistent ?
								 vr::EVRNotificationType_Persistent
								 : vr::EVRNotificationType_Transient),
								formattedText.c_str(),
								vr::EVRNotificationStyle_Application,
								notificationBitmap,
								&notificationId);

	if (notificationError != vr::VRNotificationError_OK) {
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

	m_notifications.insert(std::pair<std::string, vr::VRNotificationId>(
								   identifier,
								   notificationId));

	return true;
}

bool VROverlayController::RemoveNotification(const std::string& identifier) {
	vr::IVRNotifications* notifications = vr::VRNotifications();

	if (notifications == nullptr) {
		spdlog::error("RemoveNotification: IVRNotifications interface not available");

		return false;
	}

	auto iterator = m_notifications.find(identifier);

	if (iterator == m_notifications.end()) {
		return true;
	}

	vr::EVRNotificationError notificationError;

	notificationError = notifications->RemoveNotification(iterator->second);

	if (notificationError != vr::VRNotificationError_OK
			&& notificationError != vr::VRNotificationError_InvalidNotificationId) {
		spdlog::error(std::string("RemoveNotification: IVRNotifications: Error ")
					  + std::to_string(notificationError));

		return false;
	}

	m_notifications.erase(iterator);

	return true;
}

bool VROverlayController::ShowKeyboard(uint8_t identifier,
									   uint32_t maxLen,
									   const char* initialText,
									   const char* description,
									   bool singleLine,
									   bool password) {
	vr::IVROverlay* overlay = vr::VROverlay();

	if (overlay == nullptr) {
		spdlog::error("ShowKeyboard: IVROverlay interface not available");

		return false;
	}

	vr::EGamepadTextInputMode inputMode = (password ?
										   vr::k_EGamepadTextInputModePassword
										   : vr::k_EGamepadTextInputModeNormal);

	vr::EGamepadTextInputLineMode lineMode = (singleLine ?
											  vr::k_EGamepadTextInputLineModeSingleLine
											  : vr::k_EGamepadTextInputLineModeMultipleLines);

	vr::VROverlayError overlayError;

	overlayError = overlay->ShowKeyboardForOverlay(m_overlayHandle,
												   inputMode,
												   lineMode,
												   description,
												   maxLen,
												   initialText,
												   false,
												   (VR_CONTROLLER_USERVALUE + identifier));

	if (overlayError != vr::VROverlayError_None) {
		spdlog::error(std::string("ShowKeyboard: IVROverlay: ")
					  + overlay->GetOverlayErrorNameFromEnum(overlayError));

		return false;
	}

	return true;
}

bool VROverlayController::HideKeyboard() {
	vr::IVROverlay* overlay = vr::VROverlay();

	if (overlay == nullptr) {
		spdlog::error("HideKeyboard: IVROverlay interface not available");

		return false;
	}

	overlay->HideKeyboard();

	return true;
}

bool VROverlayController::SetupOverlay(std::string* error) {
	vr::IVROverlay* overlay = vr::VROverlay();

	if (overlay == nullptr) {
		if (error != nullptr) {
			*error = "IVROverlay interface not available";
		}

		return false;
	}

	vr::VROverlayError overlayError;

	overlayError = overlay->CreateDashboardOverlay(
						   MANIFEST_KEY,
						   OVERLAY_NAME,
						   &m_overlayHandle,
						   &m_overlayThumbnailHandle);

	if (overlayError != vr::VROverlayError_None) {
		if (error != nullptr) {
			*error = std::string("IVROverlay: ")
					 + overlay->GetOverlayErrorNameFromEnum(overlayError);
		}

		return false;
	}

	m_overlayVisible = false;

	overlay->SetOverlayWidthInMeters(m_overlayHandle, 2.35f);
	overlay->SetOverlayInputMethod(m_overlayHandle, vr::VROverlayInputMethod_Mouse);
	overlay->SetOverlayFlag(m_overlayHandle, vr::VROverlayFlags_SendVRSmoothScrollEvents, true);

	QString overlayThumbnailPath = QDir::cleanPath(QDir(qApp->applicationDirPath())
												   .absoluteFilePath(OVERLAY_THUMB_PATH));

	if (QFile::exists(overlayThumbnailPath)) {
		overlayThumbnailPath = QDir::toNativeSeparators(overlayThumbnailPath);

		overlay->SetOverlayFromFile(m_overlayThumbnailHandle,
									overlayThumbnailPath.toStdString().c_str());
	}

	return true;
}

void VROverlayController::LoadNotificationAssets() {
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

bool VROverlayController::SetupWidget(std::string* error) {
	vr::IVROverlay* overlay = vr::VROverlay();

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

	vr::HmdVector2_t widgetSize = {
		static_cast<float>(m_widget->width()),
		static_cast<float>(m_widget->height())
	};

	overlay->SetOverlayMouseScale(m_overlayHandle, &widgetSize);

	return true;
}

void VROverlayController::RenderOverlay() {
	vr::IVROverlay* overlay = vr::VROverlay();

	if (overlay == nullptr
			|| m_overlayHandle == vr::k_ulOverlayHandleInvalid
			|| m_overlayThumbnailHandle == vr::k_ulOverlayHandleInvalid) {
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
		vr::Texture_t texture = {
			reinterpret_cast<void*>(textureId),
			vr::TextureType_OpenGL,
			vr::ColorSpace_Auto
		};

		overlay->SetOverlayTexture(m_overlayHandle, &texture);
	}

	m_context->functions()->glFlush();
}

void VROverlayController::PollEvents() {
	vr::IVROverlay* overlay = vr::VROverlay();

	if (overlay == nullptr) {
		return;
	}

	vr::VREvent_t event;

	if (m_overlayHandle != vr::k_ulOverlayHandleInvalid) {
		while (overlay->PollNextOverlayEvent(m_overlayHandle, &event, sizeof(event))) {
			switch (event.eventType) {
				case vr::VREvent_MouseMove: {
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

				case vr::VREvent_MouseButtonDown: {
					QPointF newPosition(static_cast<qreal>(event.data.mouse.x),
										static_cast<qreal>(event.data.mouse.y));

					QPoint newPositionInt = newPosition.toPoint();
					QPoint lastPositionInt = m_lastPosition.toPoint();

					Qt::MouseButton button = (event.data.mouse.button == vr::VRMouseButton_Right) ?
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

				case vr::VREvent_MouseButtonUp: {
					QPointF newPosition(static_cast<qreal>(event.data.mouse.x),
										static_cast<qreal>(event.data.mouse.y));

					QPoint newPositionInt = newPosition.toPoint();
					QPoint lastPositionInt = m_lastPosition.toPoint();

					Qt::MouseButton button = (event.data.mouse.button == vr::VRMouseButton_Right) ?
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

				case vr::VREvent_ScrollSmooth: {
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

				case vr::VREvent_Notification_BeginInteraction: {
					if (event.data.notification.ulUserValue == VR_CONTROLLER_USERVALUE) {
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

				case vr::VREvent_Notification_Destroyed: {
					if (event.data.notification.ulUserValue == VR_CONTROLLER_USERVALUE) {
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

				case vr::VREvent_KeyboardDone: {
					if (event.data.keyboard.uUserValue >= VR_CONTROLLER_USERVALUE
							&& event.data.keyboard.uUserValue <= VR_CONTROLLER_USERVALUE + 255) {
						vr::IVROverlay* overlay = vr::VROverlay();

						if (overlay == nullptr) {
							spdlog::error("KeyboardDone: IVROverlay interface not available");

							break;
						}

						uint8_t identifier = static_cast<uint8_t>(event.data.keyboard.uUserValue
																  - VR_CONTROLLER_USERVALUE);

						char buffer[256];

						uint32_t inputLen = overlay->GetKeyboardText(buffer, sizeof(buffer));

						emit KeyboardData(identifier, std::string(buffer, inputLen));
					}

					break;
				}

				case vr::VREvent_OverlayShown:
					m_renderRequested = true;

					if (!m_overlayVisible) {
						m_overlayVisible = true;

						emit OverlayShown();
					}

					break;

				case vr::VREvent_OverlayHidden:
					if (m_overlayVisible) {
						m_overlayVisible = false;

						emit OverlayHidden();
					}

					break;

				case vr::VREvent_Quit:
				case vr::VREvent_DriverRequestedQuit:
					qApp->quit();
					break;
			}
		}
	}

	if (m_overlayThumbnailHandle != vr::k_ulOverlayHandleInvalid) {
		while (overlay->PollNextOverlayEvent(m_overlayThumbnailHandle, &event, sizeof(event))) {
			switch (event.eventType) {
				case vr::VREvent_OverlayShown:
					m_renderRequested = true;
					break;
			}
		}
	}
}

bool VROverlayController::InitVRRuntime(vr::EVRApplicationType applicationType,
										std::string* error) {
	vr::HmdError hmdError = vr::VRInitError_None;

	vr::VR_Init(&hmdError, applicationType);

	if (hmdError != vr::VRInitError_None) {
		if (error != nullptr) {
			*error = std::string("Failed to initialize OpenVR runtime: ")
					 + vr::VR_GetVRInitErrorAsEnglishDescription(hmdError);
		}

		return false;
	}

	spdlog::info("OpenVR runtime initialized");

	return true;
}

void VROverlayController::ShutdownVRRuntime() {
	vr::VR_Shutdown();

	spdlog::info("VR runtime shutdown");
}
