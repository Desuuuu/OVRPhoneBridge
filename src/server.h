#ifndef SERVER_H
#define SERVER_H

#include <QList>
#include <QObject>
#include <QString>
#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonObject>

#include "crypto.h"

class Server : public QObject {
		Q_OBJECT

	public:
		Server(const QString& encryptionKey,
			   quint16 port = 0,
			   const QHostAddress& address = QHostAddress::Any,
			   QObject* parent = nullptr);
		~Server();

		void Stop();

		QString GetAddress() const;
		quint16 GetPort() const;

		bool IsConnected() const;
		QString GetClientAddress() const;
		quint16 GetClientPort() const;

		bool HasNotifications() const;
		bool HasSMS() const;
		QString GetClientAppVersion() const;
		QString GetClientDeviceName() const;
		QString GetClientOSType() const;
		QString GetClientOSVersion() const;

	public slots:
		void SendMessageToClient(const QJsonObject& json);
		void KickClient();

	private slots:
		void NewConnection();
		void SocketReadyRead();
		void SocketDisconnected();
		void ClearBans();

	signals:
		void ConnectedChange(bool connected);
		void MessageReceived(const QString& type, const QJsonObject& json);

	private:
		Crypto* m_crypto;
		QTcpServer* m_server;
		QList<QTcpSocket*> m_clients;
		QTcpSocket* m_client;
		QHash<QHostAddress, qint64> m_banList;

		bool m_featureNotifications;
		bool m_featureSMS;
		QString m_appVersion;
		QString m_deviceName;
		QString m_osType;
		QString m_osVersion;

		bool SendMessageToSocket(QTcpSocket* socket,
								 const QJsonObject& json,
								 std::string* error = nullptr);
		void ProcessMessage(QTcpSocket* socket, const QString& data);
};

#endif /* SERVER_H */
