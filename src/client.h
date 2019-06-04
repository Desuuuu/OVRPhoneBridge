#ifndef CLIENT_H
#define CLIENT_H

#include <QObject>
#include <QTcpSocket>

#include "crypto.h"

class Client : public QObject {
		Q_OBJECT

	public:
		Client(const QString* serverPublicKey,
			   const QString* serverSecretKey,
			   QTcpSocket* socket,
			   QObject* parent = nullptr);
		~Client();

		QString GetAddress() const;
		quint16 GetPort() const;

		qint64 GetConnectTime() const;
		bool IsHandshakeDone() const;

		Crypto* GetCrypto() const;

		QString GetAppVersion() const;
		QString GetDeviceName() const;
		QString GetIdentifier() const;
		QString GetOSType() const;
		QString GetOSVersion() const;

		bool HasNotifications() const;
		bool HasSMS() const;

		void SendPublicKey();
		void SendJsonMessage(const QJsonObject& message);
		void Kick();
		void HandshakeResult(bool allow);

	private slots:
		void SocketReadyRead();
		void SocketDisconnected();

	signals:
		void HandshakePending();
		void MessageReceived(const QString& type, const QJsonObject& json);
		void Disconnected();

	private:
		const QString* m_serverPublicKey;
		const QString* m_serverSecretKey;

		QTcpSocket* m_socket;
		qint64 m_connectTime;
		bool m_handshakeDone;

		Crypto* m_crypto;

		QString m_appVersion;
		QString m_deviceName;
		QString m_osType;
		QString m_osVersion;

		bool m_notifications;
		bool m_sms;

		void HandshakePhase1(const QString& publicKey);
		void HandshakePhase2(const QJsonObject& json);
		void ProcessMessage(const QString& message);
};

#endif
