/*
 * Wrapper functions for mbedtls libcrypto
 * Copyright (c) 2004-2017, Jouni Malinen <j@w1.fi>
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */
/****************************************************************************
 * Notice of Export Control Law
 * ===============================================
 * Huawei LiteOS may be subject to applicable export control laws and regulations,
 * which might include those applicable to Huawei LiteOS of U.S. and the country in
 * which you are located.
 * Import, export and usage of Huawei LiteOS in any manner by you shall be in
 * compliance with such applicable export control laws and regulations.
 ****************************************************************************/

#include "crypto_mbedtls.h"
#include "securec.h"
#include "common.h"
#include "crypto.h"
#include "mbedtls/ecp.h"
#include "mbedtls/bignum.h"
#include "mbedtls/dhm.h"
#include "mbedtls/md.h"
#include "mbedtls/cmac.h"
#include "mbedtls/aes.h"
#include "mbedtls/pkcs5.h"
#include "mbedtls/entropy_poll.h"
#include "hi_watchdog.h"

#if defined(MBEDTLS_NIST_KW_C)
#include "mbedtls/nist_kw.h"
#endif
#include "utils/const_time.h"

static int get_trng(void *p_rng, unsigned char *buf, size_t len)
{
	(void)p_rng;
	size_t olen;
	return mbedtls_hardware_poll(NULL, buf, len, &olen) ? -1 : 0;
}

int crypto_get_random(void *buf, size_t len)
{
	size_t olen;
	return mbedtls_hardware_poll(NULL, buf, len, &olen) ? -1 : 0;
}

static int mbedtls_digest_vector(const mbedtls_md_info_t *md_info, size_t num_elem,
                                 const u8 *addr[], const size_t *len, u8 *mac)
{
	mbedtls_md_context_t ctx;
	size_t i;
	int ret;

	if (md_info == NULL || addr == NULL || len == NULL || mac == NULL)
		return MBEDTLS_ERR_MD_BAD_INPUT_DATA;

	mbedtls_md_init(&ctx);

	if ((ret = mbedtls_md_setup(&ctx, md_info, 1)) != 0)
		goto cleanup;

	if ((ret = mbedtls_md_starts(&ctx)) != 0)
		goto cleanup;

	for (i = 0; i < num_elem; i++) {
		if ((ret = mbedtls_md_update(&ctx, addr[i], len[i])) != 0)
			goto cleanup;
	}

	if ((ret = mbedtls_md_finish(&ctx, mac)) != 0)
		goto cleanup;

cleanup:
	mbedtls_md_free(&ctx);

	return ret;
}

#ifndef CONFIG_FIPS
int md4_vector(size_t num_elem, const u8 *addr[], const size_t *len, u8 *mac)
{
	return mbedtls_digest_vector(mbedtls_md_info_from_type(MBEDTLS_MD_MD4), num_elem, addr, len, mac);
}
#endif /* CONFIG_FIPS */

#ifndef CONFIG_FIPS
int md5_vector(size_t num_elem, const u8 *addr[], const size_t *len, u8 *mac)
{
	return mbedtls_digest_vector(mbedtls_md_info_from_type(MBEDTLS_MD_MD5), num_elem, addr, len, mac);
}
#endif /* CONFIG_FIPS */

int sha1_vector(size_t num_elem, const u8 *addr[], const size_t *len, u8 *mac)
{
	return mbedtls_digest_vector(mbedtls_md_info_from_type(MBEDTLS_MD_SHA1), num_elem, addr, len, mac);
}

#ifndef NO_SHA256_WRAPPER
int sha256_vector(size_t num_elem, const u8 *addr[], const size_t *len, u8 *mac)
{
	return mbedtls_digest_vector(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), num_elem, addr, len, mac);
}
#endif

#ifndef NO_SHA384_WRAPPER
int sha384_vector(size_t num_elem, const u8 *addr[], const size_t *len, u8 *mac)
{
	return mbedtls_digest_vector(mbedtls_md_info_from_type(MBEDTLS_MD_SHA384), num_elem, addr, len, mac);
}
#endif

#ifndef NO_SHA512_WRAPPER
int sha512_vector(size_t num_elem, const u8 *addr[], const size_t *len, u8 *mac)
{
	return mbedtls_digest_vector(mbedtls_md_info_from_type(MBEDTLS_MD_SHA512), num_elem, addr, len, mac);
}
#endif

int mbedtls_hmac_vector(const mbedtls_md_info_t *md_info,
                        const u8 *key, size_t keylen, size_t num_elem,
                        const u8 *addr[], const size_t *len, u8 *mac)
{
	mbedtls_md_context_t ctx;
	size_t i;
	int ret;

	if (md_info == NULL || key == NULL || addr == NULL || len == NULL || mac == NULL)
		return MBEDTLS_ERR_MD_BAD_INPUT_DATA;

	mbedtls_md_init(&ctx);

	if ((ret = mbedtls_md_setup(&ctx, md_info, 1)) != 0)
		goto cleanup;

	if ((ret = mbedtls_md_hmac_starts(&ctx, key, keylen)) != 0)
		goto cleanup;

	for (i = 0; i < num_elem; i++) {
		if ((ret = mbedtls_md_hmac_update(&ctx, addr[i], len[i])) != 0)
			goto cleanup;
	}

	if ((ret = mbedtls_md_hmac_finish(&ctx, mac)) != 0)
		goto cleanup;

cleanup:
	mbedtls_md_free(&ctx);

	return ret;
}

#ifndef CONFIG_FIPS
int hmac_md5_vector(const u8 *key, size_t key_len, size_t num_elem,
	const u8 *addr[], const size_t *len, u8 *mac)
{
	return mbedtls_hmac_vector(mbedtls_md_info_from_type(MBEDTLS_MD_MD5), key, key_len, num_elem, addr, len, mac);
}
int hmac_md5(const u8 *key, size_t key_len, const u8 *data, size_t data_len, u8 *mac)
{
	return hmac_md5_vector(key, key_len, 1, &data, &data_len, mac);
}
#endif /* CONFIG_FIPS */

int pbkdf2_sha1(const char *passphrase, const u8 *ssid, size_t ssid_len, int iterations, u8 *buf, size_t buflen)
{
	mbedtls_md_context_t sha1_ctx;
	const mbedtls_md_info_t *info_sha1 = NULL;
	int ret;
	if ((passphrase == NULL) || (ssid == NULL) || (buf == NULL) ||
		(ssid_len <= 0) || (buflen <= 0))
		return -1;
	mbedtls_md_init(&sha1_ctx);
	info_sha1 = mbedtls_md_info_from_type(MBEDTLS_MD_SHA1);
	if (info_sha1 == NULL) {
		ret = -1;
		goto cleanup;
	}
	if ((ret = mbedtls_md_setup(&sha1_ctx, info_sha1, 1)) != 0) {
		ret = -1;
		goto cleanup;
	}

	if (mbedtls_pkcs5_pbkdf2_hmac(&sha1_ctx, (const unsigned char *)passphrase, os_strlen(passphrase),
		ssid, ssid_len, iterations, buflen, buf) != 0) {
		ret =  -1;
		goto cleanup;
	}
cleanup:
	mbedtls_md_free(&sha1_ctx);
	return ret;
}

int hmac_sha1_vector(const u8 *key, size_t key_len, size_t num_elem,
	const u8 *addr[], const size_t *len, u8 *mac)
{
	return mbedtls_hmac_vector(mbedtls_md_info_from_type(MBEDTLS_MD_SHA1), key ,key_len, num_elem, addr, len, mac);
}
int hmac_sha1(const u8 *key, size_t key_len, const u8 *data, size_t data_len, u8 *mac)
{
	return hmac_sha1_vector(key, key_len, 1, &data, &data_len, mac);
}

#ifdef CONFIG_SHA256
int hmac_sha256_vector(const u8 *key, size_t key_len, size_t num_elem,
	const u8 *addr[], const size_t *len, u8 *mac)
{
	return mbedtls_hmac_vector(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), key ,key_len, num_elem, addr, len, mac);
}
int hmac_sha256(const u8 *key, size_t key_len, const u8 *data, size_t data_len, u8 *mac)
{
	return hmac_sha256_vector(key, key_len, 1, &data, &data_len, mac);
}
#endif /* CONFIG_SHA256 */

#ifdef CONFIG_SHA384
int hmac_sha384_vector(const u8 *key, size_t key_len, size_t num_elem,
	const u8 *addr[], const size_t *len, u8 *mac)
{
	return mbedtls_hmac_vector(mbedtls_md_info_from_type(MBEDTLS_MD_SHA384), key ,key_len, num_elem, addr, len, mac);
}

int hmac_sha384(const u8 *key, size_t key_len, const u8 *data, size_t data_len, u8 *mac)
{
	return hmac_sha384_vector(key, key_len, 1, &data, &data_len, mac);
}
#endif /* CONFIG_SHA384 */

#ifdef CONFIG_SHA512
int hmac_sha512_vector(const u8 *key, size_t key_len, size_t num_elem,
	const u8 *addr[], const size_t *len, u8 *mac)
{
	return mbedtls_hmac_vector(mbedtls_md_info_from_type(MBEDTLS_MD_SHA512), key ,key_len, num_elem, addr, len, mac);
}

int hmac_sha512(const u8 *key, size_t key_len, const u8 *data, size_t data_len, u8 *mac)
{
	return hmac_sha512_vector(key, key_len, 1, &data, &data_len, mac);
}
#endif /* CONFIG_SHA512 */

#ifdef CONFIG_OPENSSL_CMAC
int omac1_aes_vector(const u8 *key, size_t key_len, size_t num_elem, const u8 *addr[], const size_t *len, u8 *mac)
{
	mbedtls_cipher_context_t ctx;
	size_t i;
	int ret;
	mbedtls_cipher_info_t *cipher_info = NULL;

	if (len == NULL || key == NULL || addr == NULL || mac == NULL)
		return(MBEDTLS_ERR_CIPHER_BAD_INPUT_DATA);

	if (key_len == AES_256_ALT_BLOCK_SIZE)
		cipher_info = mbedtls_cipher_info_from_type(MBEDTLS_CIPHER_AES_256_CBC);
	else if (key_len == AES_128_ALT_BLOCK_SIZE)
		cipher_info = mbedtls_cipher_info_from_type(MBEDTLS_CIPHER_AES_128_CBC);
	else
		goto cleanup;

	mbedtls_cipher_init(&ctx);

	if ((ret = mbedtls_cipher_setup(&ctx, cipher_info)) != 0)
		goto cleanup;

	ret = mbedtls_cipher_cmac_starts(&ctx, key, key_len);
	if (ret != 0)
		goto cleanup;

	for (i = 0; i < num_elem; i++) {
		if ((ret = mbedtls_cipher_cmac_update(&ctx, addr[i], len[i])) != 0)
			goto cleanup;
	}

	ret = mbedtls_cipher_cmac_finish(&ctx, mac);

cleanup:
	mbedtls_cipher_free(&ctx);

	return ret;

}

int omac1_aes_128_vector(const u8 *key, size_t num_elem, const u8 *addr[], const size_t *len, u8 *mac)
{
	return omac1_aes_vector(key, AES_128_ALT_BLOCK_SIZE, num_elem, addr, len, mac);
}

int omac1_aes_128(const u8 *key, const u8 *data, size_t data_len, u8 *mac)
{
	return omac1_aes_128_vector(key, 1, &data, &data_len, mac);
}

int omac1_aes_256(const u8 *key, const u8 *data, size_t data_len, u8 *mac)
{
	return omac1_aes_vector(key, AES_256_ALT_BLOCK_SIZE, 1, &data, &data_len, mac);
}
#endif /* CONFIG_OPENSSL_CMAC */

static void get_group5_prime(mbedtls_mpi *p)
{
	static const unsigned char RFC3526_PRIME_1536[] = {
		0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xC9,0x0F,0xDA,0xA2,
		0x21,0x68,0xC2,0x34,0xC4,0xC6,0x62,0x8B,0x80,0xDC,0x1C,0xD1,
		0x29,0x02,0x4E,0x08,0x8A,0x67,0xCC,0x74,0x02,0x0B,0xBE,0xA6,
		0x3B,0x13,0x9B,0x22,0x51,0x4A,0x08,0x79,0x8E,0x34,0x04,0xDD,
		0xEF,0x95,0x19,0xB3,0xCD,0x3A,0x43,0x1B,0x30,0x2B,0x0A,0x6D,
		0xF2,0x5F,0x14,0x37,0x4F,0xE1,0x35,0x6D,0x6D,0x51,0xC2,0x45,
		0xE4,0x85,0xB5,0x76,0x62,0x5E,0x7E,0xC6,0xF4,0x4C,0x42,0xE9,
		0xA6,0x37,0xED,0x6B,0x0B,0xFF,0x5C,0xB6,0xF4,0x06,0xB7,0xED,
		0xEE,0x38,0x6B,0xFB,0x5A,0x89,0x9F,0xA5,0xAE,0x9F,0x24,0x11,
		0x7C,0x4B,0x1F,0xE6,0x49,0x28,0x66,0x51,0xEC,0xE4,0x5B,0x3D,
		0xC2,0x00,0x7C,0xB8,0xA1,0x63,0xBF,0x05,0x98,0xDA,0x48,0x36,
		0x1C,0x55,0xD3,0x9A,0x69,0x16,0x3F,0xA8,0xFD,0x24,0xCF,0x5F,
		0x83,0x65,0x5D,0x23,0xDC,0xA3,0xAD,0x96,0x1C,0x62,0xF3,0x56,
		0x20,0x85,0x52,0xBB,0x9E,0xD5,0x29,0x07,0x70,0x96,0x96,0x6D,
		0x67,0x0C,0x35,0x4E,0x4A,0xBC,0x98,0x04,0xF1,0x74,0x6C,0x08,
		0xCA,0x23,0x73,0x27,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	};
	mbedtls_mpi_init(p);
	mbedtls_mpi_read_binary(p, RFC3526_PRIME_1536, sizeof(RFC3526_PRIME_1536));
	return;
}

void * aes_encrypt_init(const u8 *key, size_t len)
{
	mbedtls_aes_context *ctx = NULL;
	ctx = os_zalloc(sizeof(mbedtls_aes_context));
	if (ctx == NULL)
		return NULL;

	mbedtls_aes_setkey_enc(ctx, key, (len * 8));
	return ctx;
}

int aes_encrypt(void *ctx, const u8 *plain, u8 *crypt)
{
	return mbedtls_internal_aes_encrypt(ctx, plain, crypt);
}

void aes_encrypt_deinit(void *ctx)
{
	(void)memset_s(ctx, sizeof(mbedtls_aes_context), 0x00, sizeof(mbedtls_aes_context));
	os_free(ctx);
}

void * aes_decrypt_init(const u8 *key, size_t len)
{
	mbedtls_aes_context *ctx;
	ctx = os_zalloc(sizeof(mbedtls_aes_context));
	if (ctx == NULL)
		return NULL;

	mbedtls_aes_setkey_dec(ctx, key, (len * 8));
	return ctx;
}

int aes_decrypt(void *ctx, const u8 *crypt, u8 *plain)
{
	return mbedtls_internal_aes_decrypt(ctx, crypt, plain);
}

void aes_decrypt_deinit(void *ctx)
{
	(void)memset_s(ctx, sizeof(mbedtls_aes_context), 0x00, sizeof(mbedtls_aes_context));
	os_free(ctx);
}

int aes_128_cbc_encrypt(const u8 *key, const u8 *iv, u8 *data, size_t data_len)
{
	mbedtls_aes_context ctx = { 0 };
	u8 temp_iv[16] = { 0 };  /* 16: iv length */
	if (iv == NULL)
		return -1;
	if (memcpy_s(temp_iv, sizeof(temp_iv), iv, 16) != EOK)
		return -1;

	mbedtls_aes_setkey_enc(&ctx, key, AES_128_CRYPTO_LEN);
	return mbedtls_aes_crypt_cbc(&ctx, MBEDTLS_AES_ENCRYPT, data_len, temp_iv, data, data);
}

int aes_128_cbc_decrypt(const u8 *key, const u8 *iv, u8 *data, size_t data_len)
{
	mbedtls_aes_context ctx = { 0 };
	u8 temp_iv[16] = { 0 };  /* 16: iv length */
	if (iv == NULL)
		return -1;
	if (memcpy_s(temp_iv, sizeof(temp_iv), iv, 16) != EOK)
		return -1;

	mbedtls_aes_setkey_dec(&ctx, key, AES_128_CRYPTO_LEN);
	return mbedtls_aes_crypt_cbc(&ctx, MBEDTLS_AES_DECRYPT, data_len, temp_iv, data, data);
}

void * dh5_init(struct wpabuf **priv, struct wpabuf **publ)
{
	mbedtls_dhm_context *dh = NULL;
	struct wpabuf *pubkey = NULL;
	struct wpabuf *privkey = NULL;
	size_t publen, privlen;
	unsigned char *export = NULL;
	size_t exportlen;
	if ((priv == NULL) || (publ == NULL))
		return NULL;
	if (*priv != NULL) {
		wpabuf_free(*priv);
		*priv = NULL;
	}
	if (*publ != NULL) {
		wpabuf_free(*publ);
		*publ = NULL;
	}
	dh = os_zalloc(sizeof(*dh));
	if (dh == NULL)
		return NULL;

	mbedtls_dhm_init(dh);
	mbedtls_mpi_init(&dh->G);
	mbedtls_mpi_lset(&dh->G, DHM_PARM_G_LEN);
	get_group5_prime(&dh->P);
	export = os_zalloc(DHM_PARM_MEM); // check result in mbedtls_dhm_make_params
	if (mbedtls_dhm_make_params(dh, (int)mbedtls_mpi_size(&dh->P), export, &exportlen, get_trng, NULL) != 0)
		goto err;

	os_free(export);
	export = NULL;
	publen = mbedtls_mpi_size((const mbedtls_mpi *)&(dh->GX));
	pubkey = wpabuf_alloc(publen);
	if (pubkey == NULL)
		goto err;

	privlen = mbedtls_mpi_size((const mbedtls_mpi *)&dh->X);
	privkey = wpabuf_alloc(privlen);
	if (privkey == NULL)
		goto err;

	mbedtls_mpi_write_binary((const mbedtls_mpi *)&dh->GX, wpabuf_put(pubkey, publen), publen);
	mbedtls_mpi_write_binary((const mbedtls_mpi *)&dh->X, wpabuf_put(privkey, privlen), privlen);
	*priv = privkey;
	*publ = pubkey;
	return dh;
err:
	wpabuf_clear_free(pubkey);
	wpabuf_clear_free(privkey);
	mbedtls_dhm_free(dh);
	os_free(dh);
	return NULL;
}

void * dh5_init_fixed(const struct wpabuf *priv, const struct wpabuf *publ)
{
	mbedtls_dhm_context *dh = NULL;
	unsigned char *export = NULL;
	size_t exportlen;
	struct wpabuf *pubkey = NULL;
	struct wpabuf *privkey = NULL;
	size_t publen, privlen;
	dh = os_zalloc(sizeof(*dh));
	if (dh == NULL)
		return NULL;

	mbedtls_dhm_init(dh);
	mbedtls_mpi_init(&dh->G);
	mbedtls_mpi_lset(&dh->G, DHM_PARM_G_LEN);
	get_group5_prime(&dh->P);

	if (mbedtls_mpi_read_binary(&dh->X, wpabuf_head(priv), wpabuf_len(priv)) != 0)
		goto err;

	if (mbedtls_mpi_read_binary(&dh->GX, wpabuf_head(publ), wpabuf_len(publ)) != 0)
		goto err;

	export = os_zalloc(DHM_PARM_MEM); // check result in mbedtls_dhm_make_params
	if (mbedtls_dhm_make_params(dh, (int)mbedtls_mpi_size(&dh->P), export, &exportlen, get_trng, NULL) != 0)
		goto err;

	os_free(export);
	export = NULL;
	publen = mbedtls_mpi_size((const mbedtls_mpi *)&(dh->GX));
	pubkey = wpabuf_alloc(publen);
	if (pubkey == NULL)
		goto err;

	privlen = mbedtls_mpi_size((const mbedtls_mpi *)&dh->X);
	privkey = wpabuf_alloc(privlen);
	if (privkey == NULL)
		goto err;

	mbedtls_mpi_write_binary((const mbedtls_mpi *)&dh->GX, wpabuf_put(pubkey, publen), publen);
	mbedtls_mpi_write_binary((const mbedtls_mpi *)&dh->X, wpabuf_put(privkey, privlen), privlen);
	wpabuf_clear_free(pubkey);
	wpabuf_clear_free(privkey);
	return dh;
err:
	wpabuf_clear_free(pubkey);
	wpabuf_clear_free(privkey);
	mbedtls_dhm_free(dh);
	os_free(dh);
	return NULL;
}

struct wpabuf * dh5_derive_shared(void *ctx, const struct wpabuf *peer_public, const struct wpabuf *own_private)
{
	struct wpabuf *res = NULL;
	size_t rlen;
	mbedtls_dhm_context *dh = ctx;
	size_t keylen;
	(void)own_private;
	if (ctx == NULL)
		return NULL;
	if (mbedtls_mpi_read_binary(&dh->GY,wpabuf_head(peer_public), wpabuf_len(peer_public)) != 0)
		goto err;

	rlen = mbedtls_mpi_size((const mbedtls_mpi *)&(dh->P));
	res = wpabuf_alloc(rlen);
	if (res == NULL)
		goto err;

	if (mbedtls_dhm_calc_secret(dh, wpabuf_mhead(res), rlen, &keylen, NULL,NULL) != 0)
		goto err;

	wpabuf_put(res, keylen);
	mbedtls_mpi_free(&dh->GY);
	return res;
err:
	mbedtls_mpi_free(&dh->GY);
	wpabuf_clear_free(res);
	return NULL;
}

void dh5_free(void *ctx)
{
	mbedtls_dhm_context *dh = NULL;
	if (ctx == NULL)
		return;
	dh = ctx;
	mbedtls_dhm_free(dh);
	os_free(dh);
}


struct crypto_bignum *crypto_bignum_init(void)
{
	mbedtls_mpi *p = NULL;
	p = os_zalloc(sizeof(*p));
	if (p == NULL)
		return NULL;

	mbedtls_mpi_init(p);
	return p;
}

struct crypto_bignum *crypto_bignum_init_set(const u8 *buf, size_t len)
{
	int ret;
	mbedtls_mpi *p = NULL;
	p = crypto_bignum_init();
	if (p == NULL)
		return NULL;

	ret = mbedtls_mpi_read_binary(p, buf, len);
	if (ret != 0) {
		crypto_bignum_deinit(p, 1);
		p = NULL;
	}
	return p;
}

void crypto_bignum_deinit(struct crypto_bignum *n, int clear)
{
	(void)clear;
	if (n == NULL)
		return;

	mbedtls_mpi_free(n);
	os_free(n);
}

int crypto_bignum_to_bin(const struct crypto_bignum *a, u8 *buf, size_t buflen, size_t padlen)
{
	int ret;
	size_t num_bytes, offset;

	if (a == NULL || buf == NULL || padlen > buflen)
		return -1;

	num_bytes = mbedtls_mpi_size((const mbedtls_mpi *)a);
	if (num_bytes > buflen)
		return -1;

	if (padlen > num_bytes)
		offset = padlen - num_bytes;
	else
		offset = 0;

	if ((memset_s(buf, offset, 0, offset)) != EOK)
		return -1;
	ret = mbedtls_mpi_write_binary((const mbedtls_mpi *)a, buf + offset, num_bytes);
	if (ret)
		return -1;

	return num_bytes + offset;
}

int crypto_bignum_rand(struct crypto_bignum *r, const struct crypto_bignum *m)
{
	int count = 0;
	unsigned cmp = 0;
	size_t n_size = 0;
	if ((m == NULL) || (r == NULL))
		return -1;
	n_size = mbedtls_mpi_size(m);
	if (n_size == 0)
		return -1;
	do {
		if (mbedtls_mpi_fill_random(r, n_size, get_trng, NULL))
			return -1;

		if (mbedtls_mpi_shift_r(r, 8 * n_size - mbedtls_mpi_bitlen(m)))
			return -1;

		if (++count > 30)
			return -1;

		if (mbedtls_mpi_lt_mpi_ct(r, m, &cmp))
			return -1;
	} while (mbedtls_mpi_cmp_int(r, 1 ) < 0 || cmp != 1);

	return 0;
}

int crypto_bignum_add(const struct crypto_bignum *a, const struct crypto_bignum *b, struct crypto_bignum *c)
{
	return mbedtls_mpi_add_mpi((mbedtls_mpi *)c, (const mbedtls_mpi *)a, (const mbedtls_mpi *)b);
}

int crypto_bignum_mod(const struct crypto_bignum *a, const struct crypto_bignum *b, struct crypto_bignum *c)
{
	return mbedtls_mpi_mod_mpi((mbedtls_mpi *)c, (const mbedtls_mpi *)a, (const mbedtls_mpi *)b);
}

int crypto_bignum_exptmod(const struct crypto_bignum *a, const struct crypto_bignum *b,
                          const struct crypto_bignum *c, struct crypto_bignum *d)
{
	/* It takes 2.7 seconds for two basic boards to interact at one time.
	   If 10 basic boards interact at the same time, the watchdog cannot be feeded in time,
	   resulting in system abnormality. */
	hi_watchdog_feed();
	return mbedtls_mpi_exp_mod((mbedtls_mpi *)d, (const mbedtls_mpi *)a,
	                           (const mbedtls_mpi *)b, (const mbedtls_mpi *)c,
	                           NULL);
}

int crypto_bignum_inverse(const struct crypto_bignum *a, const struct crypto_bignum *b, struct crypto_bignum *c)
{
	return mbedtls_mpi_inv_mod((mbedtls_mpi *)c, (const mbedtls_mpi *)a, (const mbedtls_mpi *)b);
}

int crypto_bignum_sub(const struct crypto_bignum *a, const struct crypto_bignum *b, struct crypto_bignum *c)
{
	return mbedtls_mpi_sub_mpi((mbedtls_mpi *)c, (const mbedtls_mpi *)a, (const mbedtls_mpi *)b);
}

int crypto_bignum_div(const struct crypto_bignum *a, const struct crypto_bignum *b, struct crypto_bignum *c)
{
	return mbedtls_mpi_div_mpi((mbedtls_mpi *)c, NULL, (const mbedtls_mpi *)a, (const mbedtls_mpi *)b);
}

int crypto_bignum_mulmod(const struct crypto_bignum *a, const struct crypto_bignum *b,
                         const struct crypto_bignum *c, struct crypto_bignum *d)
{
	int ret;
	mbedtls_mpi mul;
	mbedtls_mpi_init(&mul);
	ret = mbedtls_mpi_mul_mpi(&mul, (const mbedtls_mpi *)a, (const mbedtls_mpi *)b);
	if (ret == 0)
		ret = mbedtls_mpi_mod_mpi((mbedtls_mpi *)d, &mul, (const mbedtls_mpi *)c);

	mbedtls_mpi_free(&mul);
	return ret;
}

int crypto_bignum_rshift(const struct crypto_bignum *a, int n, struct crypto_bignum *r)
{
	int ret;
	ret = mbedtls_mpi_copy((mbedtls_mpi *)r, (const mbedtls_mpi *)a);
	if (ret == 0)
		ret = mbedtls_mpi_shift_r((mbedtls_mpi *)r, n);

	return ret;
}

int crypto_bignum_cmp(const struct crypto_bignum *a, const struct crypto_bignum *b)
{
	return mbedtls_mpi_cmp_mpi((const mbedtls_mpi *)a, (const mbedtls_mpi *)b);
}

int crypto_bignum_bits(const struct crypto_bignum *a)
{
	return mbedtls_mpi_bitlen((const mbedtls_mpi *)a);
}

int crypto_bignum_is_zero(const struct crypto_bignum *a)
{
	return (mbedtls_mpi_cmp_int((const mbedtls_mpi *)a, 0) == 0) ? 1 : 0;
}

int crypto_bignum_is_one(const struct crypto_bignum *a)
{
	return (mbedtls_mpi_cmp_int((const mbedtls_mpi *)a, 1) == 0) ? 1 : 0;
}

int crypto_bignum_is_odd(const struct crypto_bignum *a)
{
	return ((a != NULL) && (a->p != NULL) && (a->n > 0) && (a->p[0] & 0x01)) ? 1 : 0;
}

int crypto_bignum_legendre(const struct crypto_bignum *a, const struct crypto_bignum *p)
{
	int ret;
	int res = -2;
	unsigned int mask;
	mbedtls_mpi t;
	mbedtls_mpi exp;
	mbedtls_mpi_init(&t);
	mbedtls_mpi_init(&exp);

	/* exp = (p-1) / 2 */
	ret = mbedtls_mpi_sub_int(&exp, (const mbedtls_mpi *)p, 1);
	if (ret == 0)
		ret = mbedtls_mpi_shift_r(&exp, 1);

	if (ret == 0)
		ret = crypto_bignum_exptmod(a, (const struct crypto_bignum *)&exp,
									p, (struct crypto_bignum *)&t);

	if (ret == 0) {
		/* Return 1 if tmp == 1, 0 if tmp == 0, or -1 otherwise. Need to use
		 * constant time selection to avoid branches here. */
		res = -1;
		mask = const_time_eq(crypto_bignum_is_one((const struct crypto_bignum *)&t), 1);
		res = const_time_select_int(mask, 1, res);
		mask = const_time_eq(crypto_bignum_is_zero((const struct crypto_bignum *)&t), 1);
		res = const_time_select_int(mask, 0, res);
	}

	mbedtls_mpi_free(&exp);
	mbedtls_mpi_free(&t);
	return res;
}

struct crypto_ec *crypto_ec_init(int group)
{
	mbedtls_ecp_group_id id;

	/* convert IANA ECC group ID to mbedtls ECC group ID */
	switch (group) {
		case 19:
			id = MBEDTLS_ECP_DP_SECP256R1; /* for SAE */
			break;
		case 28:
			id = MBEDTLS_ECP_DP_BP256R1; /* for MESH */
			break;
		default:
			return NULL;
	}

	struct crypto_ec *e = os_zalloc(sizeof(struct crypto_ec));
	if (e == NULL)
		return NULL;

	mbedtls_ecp_group_init(e);
	if (mbedtls_ecp_group_load(e, id)) {
		crypto_ec_deinit(e);
		return NULL;
	}

	return e;
}

void crypto_ec_deinit(struct crypto_ec *e)
{
	if (e != NULL) {
		mbedtls_ecp_group_free(e);
		os_free(e);
	}
}

struct crypto_ec_point *crypto_ec_point_init(struct crypto_ec *e)
{
	if (e == NULL)
		return NULL;

	crypto_ec_point *p = os_zalloc(sizeof(*p));
	if (p == NULL)
		return NULL;

	mbedtls_ecp_point_init(p);
	if (mbedtls_mpi_lset(&p->Z, 1)) { // affine coordinate
		crypto_ec_point_deinit(p, 1);
		return NULL;
	}

	return p;
}

void crypto_ec_point_deinit(struct crypto_ec_point *e, int clear)
{
	(void)clear;
	if (e == NULL)
		return;

	mbedtls_ecp_point_free(e);
	os_free(e);
}

size_t crypto_ec_prime_len(struct crypto_ec *e)
{
	if (e == NULL)
		return 0;

	return mbedtls_mpi_size(&e->P);
}

size_t crypto_ec_prime_len_bits(struct crypto_ec *e)
{
	if (e == NULL)
		return 0;

	return mbedtls_mpi_bitlen(&e->P);
}

size_t crypto_ec_order_len(struct crypto_ec *e)
{
	if (e == NULL)
		return 0;

	return mbedtls_mpi_size(&e->N);
}

const struct crypto_bignum *crypto_ec_get_prime(struct crypto_ec *e)
{
	if (e == NULL)
		return NULL;

	return (const struct crypto_bignum *)&e->P;
}

const struct crypto_bignum *crypto_ec_get_order(struct crypto_ec *e)
{
	if (e == NULL)
		return NULL;

	return (const struct crypto_bignum *)&e->N;
}

int crypto_ec_point_to_bin(struct crypto_ec *e, const struct crypto_ec_point *point, u8 *x, u8 *y)
{
	int ret = -1;
	size_t len;
	if (e == NULL || point == NULL)
		return -1;

	len = mbedtls_mpi_size(&e->P);
	if (x != NULL) {
		ret = mbedtls_mpi_write_binary(&(((const mbedtls_ecp_point *)point)->X), x, len);
		if (ret)
			return ret;
	}
	if (y != NULL) {
		ret = mbedtls_mpi_write_binary(&(((const mbedtls_ecp_point *)point)->Y), y, len);
		if (ret)
			return ret;
	}

	return ret;
}

struct crypto_ec_point *crypto_ec_point_from_bin(struct crypto_ec *e, const u8 *val)
{
	int ret;
	size_t len;
	if (e == NULL)
		return NULL;

	crypto_ec_point *p = crypto_ec_point_init(e);
	if (p == NULL)
		return NULL;

	len = mbedtls_mpi_size(&e->P);
	ret = mbedtls_mpi_read_binary(&p->X, val, len);
	if (ret == 0)
		ret = mbedtls_mpi_read_binary(&p->Y, val + len, len);

	if (ret) {
		mbedtls_ecp_point_free(p);
		os_free(p);
		return NULL;
	}
	return p;
}

int crypto_ec_point_add(struct crypto_ec *e, const struct crypto_ec_point *a,
                        const struct crypto_ec_point *b, struct crypto_ec_point *c)
{
	int ret;
	if (e == NULL)
		return -1;
	hi_watchdog_feed();
	crypto_bignum one;
	mbedtls_mpi_init(&one);
	ret = mbedtls_mpi_lset(&one, 1);
	if (ret == 0)
		ret = mbedtls_ecp_muladd(e, c, &one, a, &one, b);

	mbedtls_mpi_free(&one);
	return ret;
}

/*
 * Multiplication res = b * p
 */
int crypto_ec_point_mul(struct crypto_ec *e, const struct crypto_ec_point *p,
                        const struct crypto_bignum *b,
                        struct crypto_ec_point *res)
{
	hi_watchdog_feed();
	return mbedtls_ecp_mul(e, res, b, p, NULL, NULL) ? -1 : 0;
}

int crypto_ec_point_invert(struct crypto_ec *e, struct crypto_ec_point *p)
{
	if (e == NULL || p == NULL)
		return -1;

	return mbedtls_mpi_sub_mpi(&p->Y, &e->P, &p->Y);
}

// y_bit (first byte of compressed point) mod 2 odd : r = p - r
int crypto_ec_point_solve_y_coord(struct crypto_ec *e,
                                  struct crypto_ec_point *p,
                                  const struct crypto_bignum *x, int y_bit)
{
	int ret;
	mbedtls_mpi n;
	if (e == NULL || p == NULL)
		return -1;

	// Calculate square root of r over finite field P:
	// r = sqrt(x^3 + ax + b) = (x^3 + ax + b) ^ ((P + 1) / 4) (mod P)
	struct crypto_bignum *y_sqr = crypto_ec_point_compute_y_sqr(e, x);
	if (y_sqr == NULL)
		return -1;

	mbedtls_mpi_init(&n);

	ret = mbedtls_mpi_add_int(&n, &e->P, 1);
	if (ret == 0)
		ret = mbedtls_mpi_shift_r(&n, 2);

	if (ret == 0)
		ret = mbedtls_mpi_exp_mod(y_sqr, y_sqr, &n, &e->P, NULL);

	// If LSB(Y) != y_bit, then do Y = P - Y
	if ((y_bit != crypto_bignum_is_odd(y_sqr)) && (ret == 0))
		ret = mbedtls_mpi_sub_mpi(y_sqr, &e->P, y_sqr); // r = p - r

	if (ret == 0) {
		mbedtls_mpi_copy(&p->X, x);
		mbedtls_mpi_copy(&p->Y, y_sqr);
	}
	mbedtls_mpi_free(&n);
	crypto_bignum_deinit(y_sqr, 1);

	return ret;
}

struct crypto_bignum *crypto_ec_point_compute_y_sqr(struct crypto_ec *e, const struct crypto_bignum *x)
{
	int ret;
	if (e == NULL)
		return NULL;

	struct crypto_bignum *y2 = crypto_bignum_init();
	if (y2 == NULL)
		return NULL;

	ret = mbedtls_mpi_mul_mpi(y2, x, x);
	if (ret == 0)
		ret = mbedtls_mpi_mod_mpi(y2, y2, &e->P);

	if (ret == 0) {
		if (e->A.p == NULL)
			ret = mbedtls_mpi_sub_int(y2, y2, 3); // Special case where a is -3
		else
			ret = mbedtls_mpi_add_mpi(y2, y2, &e->A);

		if (ret == 0)
			ret = mbedtls_mpi_mod_mpi(y2, y2, &e->P);
	}
	if (ret == 0)
		ret = mbedtls_mpi_mul_mpi(y2, y2, x);

	if (ret == 0)
		ret = mbedtls_mpi_mod_mpi(y2, y2, &e->P);

	if (ret == 0)
		ret = mbedtls_mpi_add_mpi(y2, y2, &e->B);

	if (ret == 0)
		ret = mbedtls_mpi_mod_mpi(y2, y2, &e->P);

	if (ret) {
		crypto_bignum_deinit(y2, 1);
		return NULL;
	}

	return y2;
}

int crypto_ec_point_is_at_infinity(struct crypto_ec *e, const struct crypto_ec_point *p)
{
	(void)e;
	return mbedtls_ecp_is_zero((struct crypto_ec_point *)p);
}

int crypto_ec_point_is_on_curve(struct crypto_ec *e, const struct crypto_ec_point *p)
{
	return mbedtls_ecp_check_pubkey(e, p) ? 0 : 1;
}

int crypto_ec_point_cmp(const struct crypto_ec *e,
                        const struct crypto_ec_point *a,
                        const struct crypto_ec_point *b)
{
	(void)e;
	return mbedtls_ecp_point_cmp(a, b);
}
