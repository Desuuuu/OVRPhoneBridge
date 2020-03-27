#ifndef CRYPTO_H
#define CRYPTO_H

#include <string>
#include <stdexcept>

#include <QString>
#include <QSettings>

#include <sodium/crypto_kx.h>

class Crypto {
	public:
		Crypto(const QString& publicKeyHex,
			   const QString& secretKeyHex,
			   const QString& clientPublicKeyHex);

		QString Encrypt(const QString& plainText) const;
		QString Decrypt(const QString& encryptedText) const;

		QString GetEncryptedPublicKey() const;
		QString GetClientIdentifier() const;

		static void GenerateKeyPair(QSettings* settings);

	private:
		unsigned char m_publicKey[crypto_kx_PUBLICKEYBYTES];
		unsigned char m_clientPublicKey[crypto_kx_PUBLICKEYBYTES];

		QString m_clientIdentifier;

		unsigned char m_sharedPublicKey[crypto_kx_SESSIONKEYBYTES];
		unsigned char m_sharedSecretKey[crypto_kx_SESSIONKEYBYTES];

		static QString GetIdentifier(const unsigned char* publicKey);
		static uint64_t GetCurrentTime();
		static void WriteUInt64BE(unsigned char* dest, const uint64_t& src);
		static uint64_t ReadUInt64BE(const unsigned char* src);
};

#endif /* CRYPTO_H */
