#include <QTimer>
#include <QDateTime>
#include <QJsonObject>
#include <QHostAddress>
#include <QJsonDocument>

#include <spdlog/spdlog.h>

#include "client.h"

Client::Client(const QString* serverPublicKey,
			   const QString* serverSecretKey,
			   QTcpSocket* socket,
			   QObject* parent)
	: QObject(parent),
	  m_serverPublicKey(serverPublicKey),
	  m_serverSecretKey(serverSecretKey),
	  m_socket(socket),
	  m_connectTime(QDateTime::currentSecsSinceEpoch()),
	  m_handshakeDone(false),
	  m_crypto(nullptr),
	  m_appVersion(QString::null),
	  m_deviceName(QString::null),
	  m_osType(QString::null),
	  m_osVersion(QString::null),
	  m_notifications(false),
	  m_sms(false) {
	connect(m_socket, &QTcpSocket::readyRead, this, &Client::SocketReadyRead);
	connect(m_socket, &QTcpSocket::disconnected, this, &Client::SocketDisconnected);
}

Client::~Client() {
	if (m_crypto != nullptr) {
		delete m_crypto;
	}

	if (m_socket != nullptr) {
		disconnect(m_socket, &QTcpSocket::readyRead, this, &Client::SocketReadyRead);
		disconnect(m_socket, &QTcpSocket::disconnected, this, &Client::SocketDisconnected);

		m_socket->abort();
		m_socket->deleteLater();
	}
}

QHostAddress Client::GetAddress() const {
	if (m_socket == nullptr) {
		return QHostAddress::Null;
	}

	return m_socket->peerAddress();
}

QString Client::GetAddressString() const {
	if (m_socket == nullptr) {
		return QString::null;
	}

	bool ok;
	QHostAddress ipv4(m_socket->peerAddress().toIPv4Address(&ok));

	if (ok) {
		return ipv4.toString();
	}

	return m_socket->peerAddress().toString();
}

quint16 Client::GetPort() const {
	if (m_socket == nullptr) {
		return 0;
	}

	return m_socket->peerPort();
}

qint64 Client::GetConnectTime() const {
	return m_connectTime;
}

bool Client::IsHandshakeDone() const {
	return m_handshakeDone;
}

Crypto* Client::GetCrypto() const {
	return m_crypto;
}

QString Client::GetAppVersion() const {
	return m_appVersion;
}

QString Client::GetDeviceName() const {
	return m_deviceName;
}

QString Client::GetIdentifier() const {
	if (m_crypto == nullptr) {
		return QString::null;
	}

	return m_crypto->GetClientIdentifier();
}

QString Client::GetOSType() const {
	return m_osType;
}

QString Client::GetOSVersion() const {
	return m_osVersion;
}

bool Client::HasNotifications() const {
	return m_notifications;
}

bool Client::HasSMS() const {
	return m_sms;
}

void Client::Kick() {
	if (m_socket != nullptr) {
		m_socket->close();
	}
}

void Client::SendJsonMessage(const QJsonObject& message) {
	if (m_socket != nullptr) {
		QJsonDocument document(message);

		if (document.isNull()) {
			spdlog::error("SendJsonMessage: Invalid JSON object");

			return;
		}

		if (m_crypto == nullptr) {
			spdlog::error("SendJsonMessage: Encryption not available");

			return;
		}

		QString finalMessage = m_crypto->Encrypt(document.toJson());

		m_socket->write(finalMessage.toUtf8());
		m_socket->write("\n");
	}
}

void Client::AnswerHandshake(bool allow) {
	if (m_handshakeDone && !allow) {
		return;
	}

	QJsonObject response;

	response.insert("type", "handshake");
	response.insert("success", allow);

	SendJsonMessage(response);

	m_handshakeDone = allow;

	if (!allow) {
		QTimer::singleShot(10000, this, [&]() {
			Kick();
		});
	}
}

void Client::SocketReadyRead() {
	while (m_socket != nullptr && m_socket->canReadLine()) {
		QString data = QString(m_socket->readLine());

		if (data.length() > 0) {
			ProcessMessage(data);
		}
	}
}

void Client::SocketDisconnected() {
	if (m_socket != nullptr) {
		disconnect(m_socket, &QTcpSocket::readyRead, this, &Client::SocketReadyRead);
		disconnect(m_socket, &QTcpSocket::disconnected, this, &Client::SocketDisconnected);

		m_socket->deleteLater();
		m_socket = nullptr;
	}

	emit Disconnected();
}

void Client::HandshakePhase1(const QString& publicKey) {
	if (m_handshakeDone || m_socket == nullptr || m_crypto != nullptr) {
		return;
	}

	try {
		m_crypto = new Crypto(*m_serverPublicKey, *m_serverSecretKey, publicKey);
	} catch (const std::runtime_error& ex) {
		spdlog::warn(std::string("HandshakePhase1: ") + ex.what());

		Kick();
		return;
	}

	m_socket->write("@@");
	m_socket->write(m_crypto->GetEncryptedPublicKey().toUtf8());
	m_socket->write("\n");
}

void Client::HandshakePhase2(const QJsonObject& json) {
	m_appVersion = json.value("app_version").toString();
	m_deviceName = json.value("device_name").toString();
	m_osType = json.value("os_type").toString();
	m_osVersion = json.value("os_version").toString();

	QJsonObject features = json.value("features").toObject();

	m_notifications = features.value("notifications").toBool(false);
	m_sms = features.value("sms").toBool(false);

	if (m_handshakeDone) {
		AnswerHandshake(true);
	} else {
		emit HandshakePending();
	}
}

void Client::ProcessMessage(const QString& message) {
	if (message.startsWith("@@")) {
		HandshakePhase1(message.mid(2));
		return;
	}

	QJsonDocument document;

	try {
		document = QJsonDocument::fromJson(m_crypto->Decrypt(message).toUtf8());
	} catch (const std::runtime_error& ex) {
		spdlog::warn(std::string("ProcessMessage: ") + ex.what());

		Kick();
		return;
	}

	if (document.isNull() || !document.isObject()) {
		spdlog::warn("ProcessMessage: Invalid message");

		Kick();
		return;
	}

	QJsonObject json = document.object();

	if (!json.contains("type")) {
		spdlog::warn("ProcessMessage: Missing 'type' property");

		Kick();
		return;
	}

	QString type = json.value("type").toString();

	if (type.isEmpty()) {
		spdlog::warn("ProcessMessage: Missing 'type' property");

		Kick();
		return;
	}

	if (type == "handshake") {
		HandshakePhase2(json);
		return;
	}

	if (!m_handshakeDone) {
		spdlog::warn("ProcessMessage: No handshake");

		Kick();
		return;
	}

	emit MessageReceived(type, json);
}
