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

		static QString DerivePassword(const QString& password);

	private:
		unsigned char m_key[crypto_aead_xchacha20poly1305_ietf_KEYBYTES];

		static uint64_t GetCurrentTime();
		static void TransformSalt(char* dest,
								  std::size_t destLen,
								  const char* src,
								  std::size_t srcLen);
		static void WriteUInt64BE(unsigned char* dest, const uint64_t& src);
		static uint64_t ReadUInt64BE(const unsigned char* src);
};

class PasswordTooShortException : public std::runtime_error {
	public:
		PasswordTooShortException(const std::size_t& minLen);

		std::size_t GetMinLength() const;

	private:
		std::size_t m_minLen;
};

#endif /* CRYPTO_H */
