#include <ctime>
#include <cstring>
#include <algorithm>
#include <sodium/utils.h>
#include <sodium/randombytes.h>
#include <sodium/crypto_pwhash.h>

#include "crypto.h"
#include "common.h"

Crypto::Crypto(const QString& encryptionKey) {
	std::size_t keyLen;

	if (sodium_base642bin(
					m_key,
					sizeof(m_key),
					encryptionKey.toUtf8().constData(),
					static_cast<std::size_t>(encryptionKey.length()),
					"\n\r ",
					&keyLen,
					nullptr,
					sodium_base64_VARIANT_ORIGINAL) != 0) {
		throw std::runtime_error("Invalid key");
	}

	if (keyLen != crypto_aead_xchacha20poly1305_ietf_KEYBYTES) {
		throw std::runtime_error("Invalid key length");
	}
}

QString Crypto::Encrypt(const QString& plainText) const {
	QByteArray input = plainText.toUtf8();

	size_t inputLen = static_cast<std::size_t>(input.length());

	size_t adLen = 8;
	size_t nonceLen = crypto_aead_xchacha20poly1305_IETF_NPUBBYTES;
	size_t cipherLen = (inputLen + crypto_aead_xchacha20poly1305_ietf_ABYTES);
	size_t encodedLen = sodium_base64_encoded_len(
								(adLen + nonceLen + cipherLen),
								sodium_base64_VARIANT_ORIGINAL);

	unsigned char* buffer = new unsigned char[adLen + nonceLen + cipherLen + encodedLen];

	writeUInt64BE(buffer, GetCurrentTime());

	randombytes_buf((buffer + adLen), crypto_aead_xchacha20poly1305_IETF_NPUBBYTES);

	uint64_t realCipherLen;

	if (crypto_aead_xchacha20poly1305_ietf_encrypt(
					(buffer + adLen + nonceLen),
					&realCipherLen,
					reinterpret_cast<const unsigned char*>(input.constData()),
					inputLen,
					buffer,
					adLen,
					nullptr,
					(buffer + adLen),
					m_key) != 0) {
		delete[] buffer;

		throw std::runtime_error("Encryption failed");
	}

	char* encoded = reinterpret_cast<char*>(buffer + adLen + nonceLen + cipherLen);

	QString encodedStr(sodium_bin2base64(
							   encoded,
							   encodedLen,
							   buffer,
							   (adLen + nonceLen + static_cast<size_t>(realCipherLen)),
							   sodium_base64_VARIANT_ORIGINAL));

	delete[] buffer;

	return encodedStr;
}

QString Crypto::Decrypt(const QString& encryptedText) const {
	QString input(encryptedText);

	input.replace('\n', "");
	input.replace('\r', "");
	input.replace(' ', "");

	if (input.length() % 4 != 0) {
		throw std::runtime_error("Invalid input");
	}

	size_t decodedLen = static_cast<size_t>(input.length() / 4 * 3);

	if (input.endsWith("==")) {
		decodedLen -= 2;
	} else if (input.endsWith("=")) {
		decodedLen--;
	}

	size_t adLen = 8;
	size_t nonceLen = crypto_aead_xchacha20poly1305_IETF_NPUBBYTES;

	if (decodedLen <= adLen + nonceLen + crypto_aead_xchacha20poly1305_ietf_ABYTES) {
		throw std::runtime_error("Invalid input");
	}

	size_t cipherLen = decodedLen - adLen - nonceLen;
	size_t decipherLen = cipherLen - crypto_aead_xchacha20poly1305_ietf_ABYTES;

	unsigned char* buffer = new unsigned char[decodedLen + decipherLen];

	size_t realDecodedLen;

	if (sodium_base642bin(
					buffer,
					decodedLen,
					input.toUtf8().constData(),
					static_cast<std::size_t>(input.length()),
					"\n\r ",
					&realDecodedLen,
					nullptr,
					sodium_base64_VARIANT_ORIGINAL) != 0) {
		delete[] buffer;

		throw std::runtime_error("Decoding failed");
	}

	uint64_t realDecipherLen;

	if (crypto_aead_xchacha20poly1305_ietf_decrypt(
					(buffer + decodedLen),
					&realDecipherLen,
					nullptr,
					(buffer + adLen + nonceLen),
					(realDecodedLen - adLen - nonceLen),
					buffer,
					adLen,
					(buffer + adLen),
					m_key) != 0) {
		delete[] buffer;

		throw std::runtime_error("Decryption failed");
	}

	uint64_t timestamp = readUInt64BE(buffer);
	uint64_t currentTime = GetCurrentTime();

	if (timestamp > currentTime + TIMESTAMP_LEEWAY
			|| timestamp < currentTime - TIMESTAMP_LEEWAY) {
		throw std::runtime_error("Expired message");
	}

	return QString::fromUtf8(reinterpret_cast<char*>(buffer + decodedLen),
							 static_cast<int>(realDecipherLen));
}

QString Crypto::DerivePassword(const QString& password) {
	if (password.length() < ENCRYPTION_KEY_MIN_LENGTH) {
		throw PasswordTooShortException(ENCRYPTION_KEY_MIN_LENGTH);
	}

	QByteArray input = password.toUtf8();

	char salt[16];

	TransformSalt(salt, sizeof(salt), ENCRYPTION_KEY_SALT, strlen(ENCRYPTION_KEY_SALT));

	unsigned char key[crypto_aead_xchacha20poly1305_ietf_KEYBYTES];

	if (crypto_pwhash(
					key,
					sizeof(key),
					input.constData(),
					static_cast<std::size_t>(input.length()),
					reinterpret_cast<unsigned char*>(salt),
					4,
					67108864,
					crypto_pwhash_ALG_ARGON2I13) != 0) {
		throw std::runtime_error("Hashing failed");
	}

	char encoded[sodium_base64_ENCODED_LEN(sizeof(key), sodium_base64_VARIANT_ORIGINAL)];

	return QString(sodium_bin2base64(
						   encoded,
						   sizeof(encoded),
						   key,
						   sizeof(key),
						   sodium_base64_VARIANT_ORIGINAL));
}

uint64_t Crypto::GetCurrentTime() {
	std::time_t timestamp = std::time(nullptr);

	return static_cast<uint64_t>(timestamp);
}

void Crypto::TransformSalt(char* dest, std::size_t destLen, const char* src, std::size_t srcLen) {
	if (srcLen > 0) {
		std::size_t i = 0;
		std::size_t j = 0;

		while (i < destLen) {
			dest[i++] = src[j++];

			if (j == srcLen) {
				j = 0;
			}
		}
	}
}

void Crypto::writeUInt64BE(unsigned char* dest, const uint64_t& src) {
	dest[0] = static_cast<unsigned char>((src >> 56) & 0xff);
	dest[1] = static_cast<unsigned char>((src >> 48) & 0xff);
	dest[2] = static_cast<unsigned char>((src >> 40) & 0xff);
	dest[3] = static_cast<unsigned char>((src >> 32) & 0xff);
	dest[4] = static_cast<unsigned char>((src >> 24) & 0xff);
	dest[5] = static_cast<unsigned char>((src >> 16) & 0xff);
	dest[6] = static_cast<unsigned char>((src >> 8) & 0xff);
	dest[7] = static_cast<unsigned char>(src & 0xff);
}

uint64_t Crypto::readUInt64BE(const unsigned char* src) {
	return (static_cast<uint64_t>(src[0]) << 56
			| static_cast<uint64_t>(src[1]) << 48
			| static_cast<uint64_t>(src[2]) << 40
			| static_cast<uint64_t>(src[3]) << 32
			| static_cast<uint64_t>(src[4]) << 24
			| static_cast<uint64_t>(src[5]) << 16
			| static_cast<uint64_t>(src[6]) << 8
			| static_cast<uint64_t>(src[7]));
}

PasswordTooShortException::PasswordTooShortException(const std::size_t& minLen)
	: std::runtime_error("Password too short") {
	m_minLen = minLen;
}

std::size_t PasswordTooShortException::GetMinLength() const {
	return m_minLen;
}
