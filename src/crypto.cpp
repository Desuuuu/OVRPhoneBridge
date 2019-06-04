#include <ctime>
#include <cstring>
#include <algorithm>
#include <spdlog/spdlog.h>
#include <sodium/utils.h>
#include <sodium/randombytes.h>
#include <sodium/crypto_box.h>
#include <sodium/crypto_shorthash.h>
#include <sodium/crypto_aead_xchacha20poly1305.h>

#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QStandardPaths>

#include "crypto.h"
#include "common.h"

Crypto::Crypto(const QString& publicKeyHex,
			   const QString& secretKeyHex,
			   const QString& clientPublicKeyHex) {
	unsigned char secretKey[crypto_kx_SECRETKEYBYTES];

	size_t keyLen;

	if (sodium_hex2bin(
					m_publicKey,
					sizeof(m_publicKey),
					publicKeyHex.toUtf8().constData(),
					static_cast<std::size_t>(publicKeyHex.length()),
					nullptr,
					&keyLen,
					nullptr) != 0) {
		throw std::runtime_error("Invalid public key");
	}

	if (keyLen != crypto_kx_PUBLICKEYBYTES) {
		throw std::runtime_error("Invalid public key length");
	}

	if (sodium_hex2bin(
					secretKey,
					sizeof(secretKey),
					secretKeyHex.toUtf8().constData(),
					static_cast<std::size_t>(secretKeyHex.length()),
					nullptr,
					&keyLen,
					nullptr) != 0) {
		throw std::runtime_error("Invalid secret key");
	}

	if (keyLen != crypto_kx_SECRETKEYBYTES) {
		throw std::runtime_error("Invalid secret key length");
	}

	if (sodium_hex2bin(
					m_clientPublicKey,
					sizeof(m_clientPublicKey),
					clientPublicKeyHex.toUtf8().constData(),
					static_cast<std::size_t>(clientPublicKeyHex.length()),
					nullptr,
					&keyLen,
					nullptr) != 0) {
		throw std::runtime_error("Invalid client public key");
	}

	if (keyLen != crypto_kx_SECRETKEYBYTES) {
		throw std::runtime_error("Invalid client public key length");
	}

	if (crypto_kx_server_session_keys(
					m_sharedSecretKey,
					m_sharedPublicKey,
					m_publicKey,
					secretKey,
					m_clientPublicKey) != 0) {
		throw std::runtime_error("Invalid client public key");
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

	WriteUInt64BE(buffer, GetCurrentTime());

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
					m_sharedPublicKey) != 0) {
		delete[] buffer;

		throw std::runtime_error("Encryption failed");
	}

	QString encodedStr(sodium_bin2base64(
							   reinterpret_cast<char*>(buffer + adLen + nonceLen + cipherLen),
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
					m_sharedSecretKey) != 0) {
		delete[] buffer;

		throw std::runtime_error("Decryption failed");
	}

	uint64_t timestamp = ReadUInt64BE(buffer);
	uint64_t currentTime = GetCurrentTime();

	if (timestamp > currentTime + TIMESTAMP_LEEWAY
			|| timestamp < currentTime - TIMESTAMP_LEEWAY) {
		delete[] buffer;

		throw std::runtime_error("Expired message");
	}

	QString result = QString::fromUtf8(reinterpret_cast<char*>(buffer + decodedLen),
									   static_cast<int>(realDecipherLen));

	delete[] buffer;

	return result;
}

QString Crypto::GetEncryptedPublicKey() const {
	size_t adLen = 8;
	size_t cipherLen = (sizeof(m_publicKey) + crypto_box_SEALBYTES);
	size_t encodedLen = sodium_base64_encoded_len(
								(adLen + cipherLen),
								sodium_base64_VARIANT_ORIGINAL);

	unsigned char* buffer = new unsigned char[adLen + cipherLen + encodedLen];

	writeUInt64BE(buffer, GetCurrentTime());

	crypto_box_seal((buffer + adLen), m_publicKey, sizeof(m_publicKey), m_clientPublicKey);

	QString encodedStr(sodium_bin2base64(
							   reinterpret_cast<char*>(buffer + adLen + cipherLen),
							   encodedLen,
							   buffer,
							   (adLen + cipherLen),
							   sodium_base64_VARIANT_ORIGINAL));

	delete[] buffer;

	return encodedStr;
}

void Crypto::GenerateKeyPair(QSettings* settings) {
	unsigned char publicKey[crypto_kx_PUBLICKEYBYTES];
	unsigned char secretKey[crypto_kx_SECRETKEYBYTES];

	crypto_kx_keypair(publicKey, secretKey);

	QString identifier = GetIdentifier(publicKey);

	size_t publicKeyHexLen = sizeof(publicKey) * 2 + 1;
	size_t secretKeyHexLen = sizeof(secretKey) * 2 + 1;

	char* keypairBuffer = new char[publicKeyHexLen + secretKeyHexLen];

	sodium_bin2hex(keypairBuffer, publicKeyHexLen, publicKey, sizeof(publicKey));
	sodium_bin2hex(keypairBuffer + publicKeyHexLen, secretKeyHexLen, secretKey, sizeof(secretKey));

	settings->setValue("public_key", QString(keypairBuffer));
	settings->setValue("secret_key", QString(keypairBuffer + publicKeyHexLen));
	settings->setValue("identifier", identifier);

	delete[] keypairBuffer;

	QDir configDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));

	QFile identifierFile(QDir::cleanPath(configDir.absoluteFilePath(IDENTIFIER_PATH)));

	if (identifierFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
		QTextStream identifierStream(&identifierFile);

		identifierStream << identifier;

		identifierFile.close();
	} else {
		spdlog::warn("Failed to write identifier file");
	}

	spdlog::info(std::string("Key pair generated with identifier: ") + identifier.toStdString());
}

QString Crypto::GetIdentifier(const unsigned char* publicKey) {
	unsigned char key[16] = {
		0x32, 0x65, 0x40, 0x4d, 0x1d, 0x30, 0x66, 0x34,
		0x29, 0x90, 0xb8, 0x91, 0x8a, 0x8f, 0x5b, 0xa1
	};

	unsigned char shortHash[crypto_shorthash_BYTES];

	crypto_shorthash(shortHash, publicKey, crypto_kx_PUBLICKEYBYTES, key);

	char shortHashHex[sizeof(shortHash) * 2 + 1];

	sodium_bin2hex(shortHashHex, sizeof(shortHashHex), shortHash, sizeof(shortHash));

	QString identifier(shortHashHex);

	if (identifier.length() % 2 != 0) {
		identifier.prepend('0');
	}

	int i = identifier.length() - 2;

	while (i > 0) {
		identifier.insert(i, ':');

		i -= 2;
	}

	return QString("[") + identifier.toUpper() + QString("]");
}

uint64_t Crypto::GetCurrentTime() {
	std::time_t timestamp = std::time(nullptr);

	return static_cast<uint64_t>(timestamp);
}

void Crypto::WriteUInt64BE(unsigned char* dest, const uint64_t& src) {
	dest[0] = static_cast<unsigned char>((src >> 56) & 0xff);
	dest[1] = static_cast<unsigned char>((src >> 48) & 0xff);
	dest[2] = static_cast<unsigned char>((src >> 40) & 0xff);
	dest[3] = static_cast<unsigned char>((src >> 32) & 0xff);
	dest[4] = static_cast<unsigned char>((src >> 24) & 0xff);
	dest[5] = static_cast<unsigned char>((src >> 16) & 0xff);
	dest[6] = static_cast<unsigned char>((src >> 8) & 0xff);
	dest[7] = static_cast<unsigned char>(src & 0xff);
}

uint64_t Crypto::ReadUInt64BE(const unsigned char* src) {
	return (static_cast<uint64_t>(src[0]) << 56
			| static_cast<uint64_t>(src[1]) << 48
			| static_cast<uint64_t>(src[2]) << 40
			| static_cast<uint64_t>(src[3]) << 32
			| static_cast<uint64_t>(src[4]) << 24
			| static_cast<uint64_t>(src[5]) << 16
			| static_cast<uint64_t>(src[6]) << 8
			| static_cast<uint64_t>(src[7]));
}

