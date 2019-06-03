#ifndef CRYPTO_H
#define CRYPTO_H

#include <string>
#include <stdexcept>

#include <QString>

#include <sodium/crypto_aead_xchacha20poly1305.h>

class Crypto {
	public:
		Crypto(const QString& encryptionKey);

		QString Encrypt(const QString& plainText) const;
		QString Decrypt(const QString& encryptedText) const;


	private:
		unsigned char m_key[crypto_aead_xchacha20poly1305_ietf_KEYBYTES];

		static uint64_t GetCurrentTime();
    static void WriteUInt64BE(unsigned char* dest, const uint64_t& src);
    static uint64_t ReadUInt64BE(const unsigned char* src);
};

#endif /* CRYPTO_H */
