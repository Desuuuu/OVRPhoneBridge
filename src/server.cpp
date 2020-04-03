#include <QTimer>
#include <QJsonDocument>
#include <QScopedPointer>

#include <spdlog/spdlog.h>

#include "server.h"
#include "common.h"

Server::Server(const QString& publicKey,
			   const QString& secretKey,
			   quint16 port,
			   const QHostAddress& address,
			   QObject* parent)
	: QObject(parent),
	  m_publicKey(publicKey),
	  m_secretKey(secretKey),
	  m_server(nullptr),
	  m_client(nullptr) {
	m_server = new QTcpServer(this);

	connect(m_server, &QTcpServer::newConnection, this, &Server::NewConnection);

	if (!m_server->listen(address, port)) {
		throw std::runtime_error("Listen failed");
	}

	QTimer* timer = new QTimer(this);

	connect(timer, &QTimer::timeout, this, &Server::ClearBans);
	connect(timer, &QTimer::timeout, this, &Server::KickInactiveClients);

	timer->setInterval(10000);
	timer->start();
}

Server::~Server() {
}

void Server::Stop() {
	if (m_server != nullptr) {
		m_server->close();
		m_server = nullptr;
	}

	while (!m_clients.isEmpty()) {
		QPointer<Client> client(m_clients.takeFirst());

		if (client != nullptr) {
			disconnect(client, &Client::HandshakePending, this, &Server::ClientHandshakePending);
			disconnect(client, &Client::Disconnected, this, &Server::ClientDisconnected);
			disconnect(client, &Client::MessageReceived, this, &Server::ClientMessageReceived);

			client->deleteLater();
		}
	}

	if (m_client != nullptr) {
		m_client->deleteLater();
		m_client = nullptr;

		emit ConnectedChange(false);
	}
}

bool Server::IsConnected() const {
	return m_client != nullptr;
}

const Client* Server::GetClient() const {
	return m_client;
}

void Server::SendMessageToClient(const QJsonObject& json) {
	if (m_client == nullptr) {
		return;
	}

	m_client->SendJsonMessage(json);
}

void Server::KickClient() {
	if (m_client == nullptr) {
		return;
	}

	m_banList.insert(m_client->GetAddress(), QDateTime::currentSecsSinceEpoch() + KICK_BANTIME);

	m_client->Kick();
}

void Server::NewConnection() {
	while (m_server->hasPendingConnections()) {
		QScopedPointer<QTcpSocket> socket(m_server->nextPendingConnection());

		if (socket != nullptr) {
			if (m_banList.contains(socket->peerAddress())) {
				spdlog::info(std::string("Refusing connection from banned client: ")
							 + socket->peerAddress().toString().toStdString());

				socket->abort();
				return;
			}

			socket->setSocketOption(QTcpSocket::KeepAliveOption, 1);

			QPointer<Client> client(new Client(&m_publicKey, &m_secretKey, socket.take(), this));

			connect(client, &Client::HandshakePending, this, &Server::ClientHandshakePending);
			connect(client, &Client::Disconnected, this, &Server::ClientDisconnected);
			connect(client, &Client::MessageReceived, this, &Server::ClientMessageReceived);

			m_clients.append(client);
		}
	}
}

void Server::ClearBans() {
	qint64 timestamp = QDateTime::currentSecsSinceEpoch();

	QMutableHashIterator<QHostAddress, qint64> iterator(m_banList);

	while (iterator.hasNext()) {
		iterator.next();

		if (iterator.value() <= timestamp) {
			iterator.remove();
		}
	}
}
void Server::KickInactiveClients() {
	qint64 timestamp = QDateTime::currentSecsSinceEpoch();

	QMutableListIterator<QPointer<Client>> iterator(m_clients);

	while (iterator.hasNext()) {
		QPointer<Client> client(iterator.next());

		if (client == nullptr) {
			iterator.remove();
			continue;
		}

		if (client->GetAddress() == QHostAddress::Null) {
			client->deleteLater();

			iterator.remove();
			continue;
		}

		if (m_client != nullptr && client != m_client) {
			client->Kick();
			continue;
		}

		if (!client->IsHandshakeDone()
				&& client->GetConnectTime() + HANDSHAKE_WINDOW <= timestamp) {
			client->Kick();
		}
	}
}

void Server::ClientHandshakePending() {
	QPointer<Client> client(qobject_cast<Client*>(sender()));

	if (client != nullptr) {
		if (m_client != nullptr && client != m_client) {
			client->AnswerHandshake(false);
			return;
		}

		m_client = client;

		client->AnswerHandshake(true);

		QTimer::singleShot(1000, this, [&]() {
			if (m_client != nullptr) {
				emit ConnectedChange(true);
			}
		});
	}
}

void Server::ClientDisconnected() {
	QPointer<Client> client(qobject_cast<Client*>(sender()));

	if (client != nullptr) {
		disconnect(client, &Client::HandshakePending, this, &Server::ClientHandshakePending);
		disconnect(client, &Client::Disconnected, this, &Server::ClientDisconnected);
		disconnect(client, &Client::MessageReceived, this, &Server::ClientMessageReceived);

		m_clients.removeOne(client);

		if (client == m_client) {
			m_client = nullptr;

			emit ConnectedChange(false);
		}

		client->deleteLater();
	}
}

void Server::ClientMessageReceived(const QString& type, const QJsonObject& json) {
	QPointer<Client> client(qobject_cast<Client*>(sender()));

	if (client != nullptr && client == m_client) {
		emit MessageReceived(type, json);
	}
}
