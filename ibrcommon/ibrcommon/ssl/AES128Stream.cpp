/*
Copyright 2017 Google LLC.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

https://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "ibrcommon/ssl/AES128Stream.h"

#include "ibrcommon/Logger.h"

#include <cstring>
#include <netinet/in.h>
#include <openssl/rand.h>

namespace ibrcommon
{
	AES128Stream::AES128Stream(const CipherMode mode, std::ostream& output, const unsigned char key[key_size_in_bytes], const uint32_t salt)
		: CipherStream(output, mode),
		  ctx(EVP_CIPHER_CTX_new(), &EVP_CIPHER_CTX_free)
	{
		if (ctx.get() == nullptr) {
			throw ibrcommon::Exception("failed to initialize aes gcm context");
		}

		_gcm_iv.salt = htonl(salt);
		if (!RAND_bytes(_gcm_iv.initialisation_vector, iv_len)) {
			throw ibrcommon::Exception("failed to create initialization vector");
		}

		std::memcpy(_used_initialisation_vector, _gcm_iv.initialisation_vector, iv_len);

		if (EVP_EncryptInit_ex(ctx.get(), EVP_aes_128_gcm(), nullptr, key, reinterpret_cast<unsigned char*>(&_gcm_iv)) != 1) {
			throw ibrcommon::Exception("failed to set up encode cipher context");
		}
	}

	AES128Stream::AES128Stream(const CipherMode mode, std::ostream& output, const unsigned char key[key_size_in_bytes], const uint32_t salt, const unsigned char iv[iv_len])
		: CipherStream(output, mode),
		  ctx(EVP_CIPHER_CTX_new(), &EVP_CIPHER_CTX_free)
	{
		if (ctx.get() == nullptr) {
			throw ibrcommon::Exception("failed to initialize aes gcm context");
		}

		_gcm_iv.salt = htonl(salt);
		std::memcpy(_gcm_iv.initialisation_vector, iv, iv_len);
		std::memcpy(_used_initialisation_vector, iv, iv_len);

		if (EVP_DecryptInit_ex(ctx.get(), EVP_aes_128_gcm(), NULL, key, reinterpret_cast<unsigned char*>(&_gcm_iv)) != 1) {
			throw ibrcommon::Exception("failed to set up decode cipher context");
		}
	}

	void AES128Stream::getIV(unsigned char (&to_iv)[iv_len]) const {
		std::memcpy(to_iv, _used_initialisation_vector, iv_len);
	}

	void AES128Stream::getTag(unsigned char (&to_tag)[tag_len]) {
		if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_GET_TAG, tag_len, to_tag) != 1) {
			throw ibrcommon::Exception("tag generation failed");
		}
	}

	bool AES128Stream::verify(const unsigned char (&verify_tag)[tag_len])
	{
		// For GCM no extra output is ever generated.
		int outlen;
		EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_TAG, tag_len, const_cast<void*>(static_cast<const void*>(&verify_tag)));
		return (EVP_DecryptFinal_ex(ctx.get(), nullptr, &outlen) > 0);
	}

	void AES128Stream::encrypt(char *buf, const size_t size)
	{
		if (size == 0) {
			return;
		}

		std::vector<unsigned char> outbuf(size);
		int outlen;
		if (EVP_EncryptUpdate(ctx.get(), outbuf.data(), &outlen, reinterpret_cast<unsigned char *>(buf), size) != 1) {
			throw ibrcommon::Exception("block encryption failed");
		}
		if (outlen != static_cast<int>(size)) {
			// This should never happen in GCM mode.
			throw ibrcommon::Exception("cyphertext size doesn't match");
		}
		std::memcpy(buf, outbuf.data(), outlen);
	}

	void AES128Stream::encrypt_final() {
		int outlen;
		// Finalise: no output for GCM.
		if (EVP_EncryptFinal_ex(ctx.get(), nullptr, &outlen) != 1) {
			throw ibrcommon::Exception("final block encryption failed");
		}
	}

	void AES128Stream::decrypt(char *buf, const size_t size) {
		if (size == 0) {
			return;
		}

		std::vector<unsigned char> outbuf(size);
		int outlen;
		if (EVP_DecryptUpdate(ctx.get(), outbuf.data(), &outlen, reinterpret_cast<unsigned char *>(buf), size) != 1) {
			throw ibrcommon::Exception("block encryption failed");
		}
		if (outlen != static_cast<int>(size)) {
			// This should never happen in GCM mode.
			throw ibrcommon::Exception("plaintext size doesn't match");
		}
		std::memcpy(buf, outbuf.data(), outlen);
	}
}
