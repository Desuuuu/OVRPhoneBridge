#include <QDir>
#include <QIcon>
#include <QFile>
#include <QString>
#include <QMessageBox>
#include <QApplication>
#include <QStandardPaths>

#include <sodium/core.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include "common.h"
#include "crypto.h"
#include "openvr/overlay_controller.h"
#include "widgets/main_widget.h"

using namespace vr;
using namespace OpenVR;

/*
 * Install the application manifest & enable auto-start
 */

bool installVRManifest(std::string manifestPath, std::string* error) {
	IVRApplications* applications = VRApplications();

	if (applications == nullptr) {
		if (error != nullptr) {
			*error = "IVRApplications interface not available";
		}

		return false;
	}

	if (applications->IsApplicationInstalled(MANIFEST_KEY)) {
		return true;
	}

	EVRApplicationError applicationError;

	applicationError = applications->AddApplicationManifest(manifestPath.c_str());

	if (applicationError != VRApplicationError_None) {
		if (error != nullptr) {
			*error = std::string("IVRApplications: ")
					 + applications->GetApplicationsErrorNameFromEnum(applicationError);
		}

		return false;
	}

	applicationError = applications->SetApplicationAutoLaunch(MANIFEST_KEY, true);

	if (applicationError != VRApplicationError_None) {
		if (error != nullptr) {
			*error = std::string("IVRApplications: ")
					 + applications->GetApplicationsErrorNameFromEnum(applicationError);
		}

		return false;
	}

	return true;
}

/*
 * Uninstall the application manifest
 */

bool uninstallVRManifest(std::string manifestPath, std::string* error) {
	IVRApplications* applications = VRApplications();

	if (applications == nullptr) {
		if (error != nullptr) {
			*error = "IVRApplications interface not available";
		}

		return false;
	}

	if (!applications->IsApplicationInstalled(MANIFEST_KEY)) {
		return true;
	}

	EVRApplicationError applicationError;

	applicationError = applications->RemoveApplicationManifest(manifestPath.c_str());

	if (applicationError != VRApplicationError_None) {
		if (error != nullptr) {
			*error = std::string("IVRApplications: ")
					 + applications->GetApplicationsErrorNameFromEnum(applicationError);
		}

		return false;
	}

	return true;
}

/*
 * Handle --install_manifest, --uninstall_manifest and --reinstall_manifest CLI arguments
 */

bool handleManifestArguments(bool& install) {
	bool installManifest = qApp->arguments().indexOf("--install_manifest") >= 0;

	bool uninstallManifest = qApp->arguments().indexOf("--uninstall_manifest") >= 0;

	if (qApp->arguments().indexOf("--reinstall_manifest") >= 0) {
		installManifest = true;
		uninstallManifest = true;
	}

	if (installManifest || uninstallManifest) {
		std::string error;

		if (!OpenVR::OverlayController::InitVRRuntime(VRApplication_Utility, &error)) {
			error.insert(0, "Failed to initialize OpenVR runtime: ");

			throw std::runtime_error(error);
		}

		if (!VR_IsInterfaceVersionValid(IVRApplications_Version)) {
			OverlayController::ShutdownVRRuntime();

			throw std::runtime_error("Incompatible OpenVR version");
		}

		QString manifestPath = QDir::cleanPath(QDir(qApp->applicationDirPath())
											   .absoluteFilePath(MANIFEST_PATH));

		if (!QFile::exists(manifestPath)) {
			OverlayController::ShutdownVRRuntime();

			error = std::string("Application manifest not found: ") + manifestPath.toStdString();

			throw std::runtime_error(error);
		}

		manifestPath = QDir::toNativeSeparators(manifestPath);

		if (uninstallManifest && !uninstallVRManifest(manifestPath.toStdString(), &error)) {
			OverlayController::ShutdownVRRuntime();

			error.insert(0, "Failed to uninstall application manifest: ");

			throw std::runtime_error(error);
		}

		if (installManifest && !installVRManifest(manifestPath.toStdString(), &error)) {
			OverlayController::ShutdownVRRuntime();

			error.insert(0, "Failed to install application manifest: ");

			throw std::runtime_error(error);
		}

		OverlayController::ShutdownVRRuntime();

		install = installManifest;

		return true;
	}

	return false;
}

/*
 * Initialize spdlog for file logging
 */

bool initLogger(const QDir& configDir, std::string* error) {
	try {
		std::string fileName = std::string(APP_NAME) + ".log";

		QString filePath = QDir::cleanPath(configDir.absoluteFilePath(fileName.c_str()));

		auto logger = spdlog::rotating_logger_mt(APP_NAME,
												 filePath.toStdString(),
												 LOG_MAX_SIZE,
												 LOG_MAX_FILES);

		logger->set_level(spdlog::level::debug);

		spdlog::set_default_logger(logger);
		spdlog::flush_every(std::chrono::seconds(10));
	} catch (const spdlog::spdlog_ex& ex) {
		if (error != nullptr) {
			*error = ex.what();
		}

		return false;
	}

	return true;
}

/*
 * Set basic application properties
 */

void setupApplication() {
	qApp->setApplicationName(APP_NAME);
	qApp->setApplicationDisplayName(APP_NAME);
	qApp->setApplicationVersion(APP_VERSION);
	qApp->setWindowIcon(QIcon(":/images/icons/icon.png"));
}

int main(int argc, char* argv[]) {
	QApplication app(argc, argv);

	setupApplication();

	MainWidget* widget = nullptr;
	OverlayController* controller = nullptr;

	QObject::connect(&app, &QApplication::aboutToQuit, [&]() {
		if (controller != nullptr) {
			delete controller;
			controller = nullptr;
		}

		spdlog::info("Application shutdown");

		spdlog::shutdown();
	});

	bool silent = app.arguments().indexOf("--silent") >= 0;

	QDir configDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));

	if (!configDir.exists()) {
		if (!configDir.mkpath(".")) {
			if (!silent) {
				QMessageBox::critical(nullptr, APP_NAME, "Failed to create config folder");
			}

			exit(EXIT_FAILURE);
		}
	}

	if (!configDir.isReadable()) {
		if (!silent) {
			QMessageBox::critical(nullptr, APP_NAME, "Config folder unreadable");
		}

		exit(EXIT_FAILURE);
	}

	std::string error;

	if (!initLogger(configDir, &error)) {
		if (!silent) {
			error.insert(0, "Failed to initialize logger: ");

			QMessageBox::critical(nullptr, APP_NAME, error.c_str());
		}

		exit(EXIT_FAILURE);
	}

	if (sodium_init() == -1) {
		spdlog::error("Failed to initialize libsodium");

		if (!silent) {
			QMessageBox::critical(nullptr, APP_NAME, "Failed to initialize libsodium");
		}

		exit(EXIT_FAILURE);
	}

	QSettings settings(QDir::cleanPath(configDir.absoluteFilePath(SETTINGS_PATH)),
					   QSettings::IniFormat);

	if (app.arguments().indexOf("--generate_keypair") >= 0) {
		Crypto::GenerateKeyPair(&settings);

		settings.sync();

		exit(EXIT_SUCCESS);
	}

	if (!VR_IsRuntimeInstalled()) {
		spdlog::error("OpenVR runtime not installed");

		if (!silent) {
			QMessageBox::critical(nullptr, APP_NAME, "OpenVR runtime not installed");
		}

		exit(EXIT_FAILURE);
	}

	bool desktop = app.arguments().indexOf("--desktop") >= 0;

	try {
		bool install = false;

		if (handleManifestArguments(install)) {
			spdlog::info(install ?
						 "Application manifest successfully installed"
						 : "Application manifest successfully uninstalled");

			if (!silent) {
				QMessageBox::information(nullptr,
										 APP_NAME,
										 (install ?
										  "Application manifest successfully installed"
										  : "Application manifest successfully uninstalled"));
			}

			exit(EXIT_SUCCESS);
		}

		widget = new MainWidget(&settings);

		if (!desktop) {
			controller = new OverlayController(widget);
		}
	} catch (const std::runtime_error& ex) {
		spdlog::error(ex.what());

		if (!silent) {
			QMessageBox::critical(nullptr, APP_NAME, ex.what());
		}

		exit(EXIT_FAILURE);
	}

	if (desktop) {
		widget->show();
	} else {
		QObject::connect(controller,
						 &OverlayController::OverlayShown,
						 widget,
						 &MainWidget::VROverlayShown);

		QObject::connect(controller,
						 &OverlayController::NotificationOpened,
						 widget,
						 &MainWidget::VRNotificationOpened);

		QObject::connect(controller,
						 &OverlayController::KeyboardData,
						 widget,
						 &MainWidget::VRKeyboardData);

		QObject::connect(widget,
						 &MainWidget::ShowVRNotification,
						 controller,
						 &OverlayController::ShowNotification);

		QObject::connect(widget,
						 &MainWidget::RemoveVRNotification,
						 controller,
						 &OverlayController::RemoveNotification);

		QObject::connect(widget,
						 &MainWidget::ShowVRKeyboard,
						 controller,
						 &OverlayController::ShowKeyboard);
	}

	spdlog::info("Application started");

	return app.exec();
}
