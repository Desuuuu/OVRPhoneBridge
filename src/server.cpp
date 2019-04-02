#include <QTimer>
#include <QJsonDocument>

#include <spdlog/spdlog.h>

#include "server.h"
#include "common.h"

Server::Server(const QString& encryptionKey,
			   quint16 port,
			   const QHostAddress& address,
			   QObject* parent)
	: QObject(parent),
	  m_crypto(nullptr),
	  m_server(nullptr),
	  m_client(nullptr),
	  m_featureNotifications(false),
	  m_featureSMS(false),
	  m_appVersion(QString::null),
	  m_deviceName(QString::null),
	  m_osType(QString::null),
	  m_osVersion(QString::null) {
	m_crypto = new Crypto(encryptionKey);

	m_server = new QTcpServer(this);

	connect(m_server, &QTcpServer::newConnection, this, &Server::NewConnection);

	if (!m_server->listen(address, port)) {
		throw std::runtime_error("Listen failed");
	}

	QTimer* banTimer = new QTimer(this);

	connect(banTimer, &QTimer::timeout, this, &Server::ClearBans);

	banTimer->setInterval(30000);
	banTimer->start();
}

Server::~Server() {
	if (m_crypto != nullptr) {
		delete m_crypto;
	}
}

void Server::Stop() {
	if (m_server != nullptr) {
		m_server->close();
	}

	while (!m_clients.isEmpty()) {
		QTcpSocket* socket = m_clients.takeFirst();

		if (socket != nullptr) {
			disconnect(socket, &QTcpSocket::readyRead, this, &Server::SocketReadyRead);
			disconnect(socket, &QTcpSocket::disconnected, this, &Server::SocketDisconnected);

			socket->abort();
			socket->deleteLater();
		}
	}

	if (m_client != nullptr) {
		m_client = nullptr;

		emit ConnectedChange(false);
	}
}

QString Server::GetAddress() const {
	return m_server->serverAddress().toString();
}

quint16 Server::GetPort() const {
	return m_server->serverPort();
}

bool Server::IsConnected() const {
	return m_client != nullptr;
}

QString Server::GetClientAddress() const {
	if (m_client == nullptr) {
		return "";
	}

	bool ok;
	QHostAddress ipv4(m_client->peerAddress().toIPv4Address(&ok));

	if (ok) {
		return ipv4.toString();
	}

	return m_client->peerAddress().toString();
}

quint16 Server::GetClientPort() const {
	if (m_client == nullptr) {
		return 0;
	}

	return m_client->peerPort();
}

bool Server::HasNotifications() const {
	if (m_client == nullptr) {
		return false;
	}

	return m_featureNotifications;
}

bool Server::HasSMS() const {
	if (m_client == nullptr) {
		return false;
	}

	return m_featureSMS;
}

QString Server::GetClientAppVersion() const {
	if (m_client == nullptr) {
		return QString::null;
	}

	return m_appVersion;
}

QString Server::GetClientDeviceName() const {
	if (m_client == nullptr) {
		return QString::null;
	}

	return m_deviceName;
}

QString Server::GetClientOSType() const {
	if (m_client == nullptr) {
		return QString::null;
	}

	return m_osType;
}

QString Server::GetClientOSVersion() const {
	if (m_client == nullptr) {
		return QString::null;
	}

	return m_osVersion;
}

void Server::SendMessageToClient(const QJsonObject& json) {
	if (m_client == nullptr) {
		return;
	}

	std::string error;

	if (!SendMessageToSocket(m_client, json, &error)) {
		error.insert(0, "Failed to send message: ");

		spdlog::error(error);
	}
}

void Server::KickClient() {
	if (m_client == nullptr) {
		return;
	}

	m_banList.insert(m_client->peerAddress(), QDateTime::currentSecsSinceEpoch());

	m_client->close();
}

void Server::NewConnection() {
	while (m_server->hasPendingConnections()) {
		QTcpSocket* socket = m_server->nextPendingConnection();

		if (socket != nullptr) {
			if (m_banList.contains(socket->peerAddress())) {
				socket->abort();
				socket->deleteLater();
				return;
			}

			socket->setSocketOption(QTcpSocket::KeepAliveOption, 1);

			m_clients.append(socket);

			connect(socket, &QTcpSocket::readyRead, this, &Server::SocketReadyRead);
			connect(socket, &QTcpSocket::disconnected, this, &Server::SocketDisconnected);
		}
	}
}

void Server::SocketReadyRead() {
	QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());

	if (socket != nullptr) {
		while (socket->canReadLine()) {
			QString data = QString(socket->readLine());

			if (data.length() > 0) {
				ProcessMessage(socket, data);
			}
		}
	}
}

void Server::SocketDisconnected() {
	QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());

	if (socket != nullptr) {
		disconnect(socket, &QTcpSocket::readyRead, this, &Server::SocketReadyRead);
		disconnect(socket, &QTcpSocket::disconnected, this, &Server::SocketDisconnected);

		m_clients.removeOne(socket);

		if (socket == m_client) {
			m_client = nullptr;

			emit ConnectedChange(false);
		}

		socket->deleteLater();
	}
}

void Server::ClearBans() {
	qint64 timestamp = QDateTime::currentSecsSinceEpoch();

	QMutableHashIterator<QHostAddress, qint64> iterator(m_banList);

	while (iterator.hasNext()) {
		iterator.next();

		if (iterator.value() + KICK_BANTIME <= timestamp) {
			iterator.remove();
		}
	}
}

bool Server::SendMessageToSocket(QTcpSocket* socket, const QJsonObject& json, std::string* error) {
	QJsonDocument jsonDocument(json);

	if (jsonDocument.isNull()) {
		if (error != nullptr) {
			*error = std::string("Invalid JSON object");
		}

		return false;
	}

	try {
		QString encrypted = m_crypto->Encrypt(jsonDocument.toJson());

		socket->write(encrypted.toUtf8());
		socket->write("\n");
	} catch (const std::runtime_error& ex) {
		if (error != nullptr) {
			*error = ex.what();
		}

		return false;
	}

	return true;
}

void Server::ProcessMessage(QTcpSocket* socket, const QString& data) {
	try {
		QString decrypted = m_crypto->Decrypt(data);

		QJsonParseError error;

		QJsonDocument jsonDocument = QJsonDocument::fromJson(decrypted.toUtf8(), &error);

		if (jsonDocument.isNull()) {
			spdlog::error(error.errorString().toStdString());
			return;
		}

		if (!jsonDocument.isObject()) {
			spdlog::error("Invalid message");
			return;
		}

		QJsonObject json = jsonDocument.object();

		if (!json.contains("type")) {
			spdlog::error("Missing type property");
			return;
		}

		QString type = json.value("type").toString();

		if (type.isEmpty()) {
			spdlog::error("Missing type property");
			return;
		}

		if (m_client != nullptr && socket != m_client) {
			if (type == "handshake") {
				QJsonObject response;

				response.insert("type", "handshake");
				response.insert("success", false);

				SendMessageToSocket(socket, response);

				QTimer::singleShot(10000, socket, [&]() {
					if (socket->isOpen()) {
						socket->close();
					}
				});
				return;
			}

			socket->close();
			return;
		}

		if (type == "handshake") {
			QJsonObject features = json.value("features").toObject();

			m_featureNotifications = features.value("notifications").toBool(false);
			m_featureSMS = features.value("sms").toBool(false);

			m_appVersion = json.value("app_version").toString();
			m_deviceName = json.value("device_name").toString();
			m_osType = json.value("os_type").toString();
			m_osVersion = json.value("os_version").toString();

			if (m_client == nullptr) {
				m_client = socket;
			}

			QJsonObject response;

			response.insert("type", "handshake");
			response.insert("success", true);

			SendMessageToSocket(socket, response);

			QTimer::singleShot(1000, this, [&]() {
				if (m_client != nullptr) {
					emit ConnectedChange(true);
				}
			});

			return;
		}

		emit MessageReceived(type, json);
	} catch (const std::runtime_error& ex) {
		spdlog::error(ex.what());
	}
}
