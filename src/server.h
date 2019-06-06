#ifndef SERVER_H
#define SERVER_H

#include <QList>
#include <QObject>
#include <QString>
#include <QTcpServer>
#include <QJsonObject>

#include "client.h"

class Server : public QObject {
		Q_OBJECT

	public:
		Server(const QString& publicKey,
			   const QString& secretKey,
			   quint16 port = 0,
			   const QHostAddress& address = QHostAddress::Any,
			   QObject* parent = nullptr);
		~Server();

		void Stop();

		bool IsConnected() const;
		const Client* GetClient() const;

	public slots:
		void SendMessageToClient(const QJsonObject& json);
		void KickClient();

	private slots:
		void NewConnection();
		void ClearBans();
		void KickInactiveClients();

		void ClientHandshakePending();
		void ClientDisconnected();
		void ClientMessageReceived(const QString& type, const QJsonObject& json);

	signals:
		void ConnectedChange(bool connected);
		void MessageReceived(const QString& type, const QJsonObject& json);

	private:
		QString m_publicKey;
		QString m_secretKey;

		QTcpServer* m_server;
		Client* m_client;
		QList<Client*> m_clients;
		QHash<QHostAddress, qint64> m_banList;
};

#endif /* SERVER_H */
