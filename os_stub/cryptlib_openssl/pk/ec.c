/**
    Copyright Notice:
    Copyright 2021 DMTF. All rights reserved.
    License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/libspdm/blob/main/LICENSE.md
**/

/** @file
  Elliptic Curve Wrapper Implementation.

  RFC 8422 - Elliptic Curve Cryptography (ECC) Cipher Suites
  FIPS 186-4 - Digital signature Standard (DSS)
**/

#include "internal_crypt_lib.h"
#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/objects.h>

/**
  Allocates and Initializes one Elliptic Curve context for subsequent use
  with the NID.

  @param nid cipher NID

  @return  Pointer to the Elliptic Curve context that has been initialized.
           If the allocations fails, ec_new_by_nid() returns NULL.

**/
void *ec_new_by_nid(IN uintn nid)
{
	EC_KEY *ec_key;
	EC_GROUP *ec_group;
	boolean ret_val;
	int32 openssl_nid;

	ec_key = EC_KEY_new();
	if (ec_key == NULL) {
		return NULL;
	}
	switch (nid) {
	case CRYPTO_NID_SECP256R1:
		openssl_nid = NID_X9_62_prime256v1;
		break;
	case CRYPTO_NID_SECP384R1:
		openssl_nid = NID_secp384r1;
		break;
	case CRYPTO_NID_SECP521R1:
		openssl_nid = NID_secp521r1;
		break;
	default:
		return NULL;
	}

	ec_group = EC_GROUP_new_by_curve_name(openssl_nid);
	if (ec_group == NULL) {
		return NULL;
	}
	ret_val = (boolean)EC_KEY_set_group(ec_key, ec_group);
	EC_GROUP_free(ec_group);
	if (!ret_val) {
		return NULL;
	}
	return (void *)ec_key;
}

/**
  Release the specified EC context.

  @param[in]  ec_context  Pointer to the EC context to be released.

**/
void ec_free(IN void *ec_context)
{
	EC_KEY_free((EC_KEY *)ec_context);
}

/**
  Sets the public key component into the established EC context.

  For P-256, the public_size is 64. first 32-byte is X, second 32-byte is Y.
  For P-384, the public_size is 96. first 48-byte is X, second 48-byte is Y.
  For P-521, the public_size is 132. first 66-byte is X, second 66-byte is Y.

  @param[in, out]  ec_context      Pointer to EC context being set.
  @param[in]       public         Pointer to the buffer to receive generated public X,Y.
  @param[in]       public_size     The size of public buffer in bytes.

  @retval  TRUE   EC public key component was set successfully.
  @retval  FALSE  Invalid EC public key component.

**/
boolean ec_set_pub_key(IN OUT void *ec_context, IN uint8 *public_key,
		       IN uintn public_key_size)
{
	EC_KEY *ec_key;
	const EC_GROUP *ec_group;
	boolean ret_val;
	BIGNUM *bn_x;
	BIGNUM *bn_y;
	EC_POINT *ec_point;
	int32 openssl_nid;
	uintn half_size;

	if (ec_context == NULL || public_key == NULL) {
		return FALSE;
	}

	ec_key = (EC_KEY *)ec_context;
	openssl_nid = EC_GROUP_get_curve_name(EC_KEY_get0_group(ec_key));
	switch (openssl_nid) {
	case NID_X9_62_prime256v1:
		half_size = 32;
		break;
	case NID_secp384r1:
		half_size = 48;
		break;
	case NID_secp521r1:
		half_size = 66;
		break;
	default:
		return FALSE;
	}
	if (public_key_size != half_size * 2) {
		return FALSE;
	}

	ec_group = EC_KEY_get0_group(ec_key);
	ec_point = NULL;

	bn_x = BN_bin2bn(public_key, (uint32)half_size, NULL);
	bn_y = BN_bin2bn(public_key + half_size, (uint32)half_size, NULL);
	if (bn_x == NULL || bn_y == NULL) {
		ret_val = FALSE;
		goto done;
	}
	ec_point = EC_POINT_new(ec_group);
	if (ec_point == NULL) {
		ret_val = FALSE;
		goto done;
	}

	ret_val = (boolean)EC_POINT_set_affine_coordinates(ec_group, ec_point,
							   bn_x, bn_y, NULL);
	if (!ret_val) {
		goto done;
	}

	ret_val = (boolean)EC_KEY_set_public_key(ec_key, ec_point);
	if (!ret_val) {
		goto done;
	}

	ret_val = TRUE;

done:
	if (bn_x != NULL) {
		BN_free(bn_x);
	}
	if (bn_y != NULL) {
		BN_free(bn_y);
	}
	if (ec_point != NULL) {
		EC_POINT_free(ec_point);
	}
	return ret_val;
}

/**
  Gets the public key component from the established EC context.

  For P-256, the public_size is 64. first 32-byte is X, second 32-byte is Y.
  For P-384, the public_size is 96. first 48-byte is X, second 48-byte is Y.
  For P-521, the public_size is 132. first 66-byte is X, second 66-byte is Y.

  @param[in, out]  ec_context      Pointer to EC context being set.
  @param[out]      public         Pointer to the buffer to receive generated public X,Y.
  @param[in, out]  public_size     On input, the size of public buffer in bytes.
                                  On output, the size of data returned in public buffer in bytes.

  @retval  TRUE   EC key component was retrieved successfully.
  @retval  FALSE  Invalid EC key component.

**/
boolean ec_get_pub_key(IN OUT void *ec_context, OUT uint8 *public_key,
		       IN OUT uintn *public_key_size)
{
	EC_KEY *ec_key;
	const EC_GROUP *ec_group;
	boolean ret_val;
	const EC_POINT *ec_point;
	BIGNUM *bn_x;
	BIGNUM *bn_y;
	int32 openssl_nid;
	uintn half_size;
	intn x_size;
	intn y_size;

	if (ec_context == NULL || public_key_size == NULL) {
		return FALSE;
	}

	if (public_key == NULL && *public_key_size != 0) {
		return FALSE;
	}

	ec_key = (EC_KEY *)ec_context;

	openssl_nid = EC_GROUP_get_curve_name(EC_KEY_get0_group(ec_key));
	switch (openssl_nid) {
	case NID_X9_62_prime256v1:
		half_size = 32;
		break;
	case NID_secp384r1:
		half_size = 48;
		break;
	case NID_secp521r1:
		half_size = 66;
		break;
	default:
		return FALSE;
	}
	if (*public_key_size < half_size * 2) {
		*public_key_size = half_size * 2;
		return FALSE;
	}
	*public_key_size = half_size * 2;

	ec_group = EC_KEY_get0_group(ec_key);
	ec_point = EC_KEY_get0_public_key(ec_key);
	if (ec_point == NULL) {
		return FALSE;
	}

	bn_x = BN_new();
	bn_y = BN_new();
	if (bn_x == NULL || bn_y == NULL) {
		ret_val = FALSE;
		goto done;
	}

	ret_val = (boolean)EC_POINT_get_affine_coordinates(ec_group, ec_point,
							   bn_x, bn_y, NULL);
	if (!ret_val) {
		goto done;
	}

	x_size = BN_num_bytes(bn_x);
	y_size = BN_num_bytes(bn_y);
	if (x_size <= 0 || y_size <= 0) {
		ret_val = FALSE;
		goto done;
	}
	ASSERT((uintn)x_size <= half_size && (uintn)y_size <= half_size);

	if (public_key != NULL) {
		zero_mem(public_key, *public_key_size);
		BN_bn2bin(bn_x, &public_key[0 + half_size - x_size]);
		BN_bn2bin(bn_y, &public_key[half_size + half_size - y_size]);
	}
	ret_val = TRUE;

done:
	if (bn_x != NULL) {
		BN_free(bn_x);
	}
	if (bn_y != NULL) {
		BN_free(bn_y);
	}
	return ret_val;
}

/**
  Validates key components of EC context.
  NOTE: This function performs integrity checks on all the EC key material, so
        the EC key structure must contain all the private key data.

  If ec_context is NULL, then return FALSE.

  @param[in]  ec_context  Pointer to EC context to check.

  @retval  TRUE   EC key components are valid.
  @retval  FALSE  EC key components are not valid.

**/
boolean ec_check_key(IN void *ec_context)
{
	EC_KEY *ec_key;
	boolean ret_val;

	if (ec_context == NULL) {
		return FALSE;
	}

	ec_key = (EC_KEY *)ec_context;

	ret_val = (boolean)EC_KEY_check_key(ec_key);
	if (!ret_val) {
		return FALSE;
	}

	return TRUE;
}

/**
  Generates EC key and returns EC public key (X, Y).

  This function generates random secret, and computes the public key (X, Y), which is
  returned via parameter public, public_size.
  X is the first half of public with size being public_size / 2,
  Y is the second half of public with size being public_size / 2.
  EC context is updated accordingly.
  If the public buffer is too small to hold the public X, Y, FALSE is returned and
  public_size is set to the required buffer size to obtain the public X, Y.

  For P-256, the public_size is 64. first 32-byte is X, second 32-byte is Y.
  For P-384, the public_size is 96. first 48-byte is X, second 48-byte is Y.
  For P-521, the public_size is 132. first 66-byte is X, second 66-byte is Y.

  If ec_context is NULL, then return FALSE.
  If public_size is NULL, then return FALSE.
  If public_size is large enough but public is NULL, then return FALSE.

  @param[in, out]  ec_context      Pointer to the EC context.
  @param[out]      public         Pointer to the buffer to receive generated public X,Y.
  @param[in, out]  public_size     On input, the size of public buffer in bytes.
                                  On output, the size of data returned in public buffer in bytes.

  @retval TRUE   EC public X,Y generation succeeded.
  @retval FALSE  EC public X,Y generation failed.
  @retval FALSE  public_size is not large enough.

**/
boolean ec_generate_key(IN OUT void *ec_context, OUT uint8 *public,
			IN OUT uintn *public_size)
{
	EC_KEY *ec_key;
	const EC_GROUP *ec_group;
	boolean ret_val;
	const EC_POINT *ec_point;
	BIGNUM *bn_x;
	BIGNUM *bn_y;
	int32 openssl_nid;
	uintn half_size;
	intn x_size;
	intn y_size;

	if (ec_context == NULL || public_size == NULL) {
		return FALSE;
	}

	if (public == NULL && *public_size != 0) {
		return FALSE;
	}

	ec_key = (EC_KEY *)ec_context;
	ret_val = (boolean)EC_KEY_generate_key(ec_key);
	if (!ret_val) {
		return FALSE;
	}
	openssl_nid = EC_GROUP_get_curve_name(EC_KEY_get0_group(ec_key));
	switch (openssl_nid) {
	case NID_X9_62_prime256v1:
		half_size = 32;
		break;
	case NID_secp384r1:
		half_size = 48;
		break;
	case NID_secp521r1:
		half_size = 66;
		break;
	default:
		return FALSE;
	}
	if (*public_size < half_size * 2) {
		*public_size = half_size * 2;
		return FALSE;
	}
	*public_size = half_size * 2;

	ec_group = EC_KEY_get0_group(ec_key);
	ec_point = EC_KEY_get0_public_key(ec_key);
	if (ec_point == NULL) {
		return FALSE;
	}

	bn_x = BN_new();
	bn_y = BN_new();
	if (bn_x == NULL || bn_y == NULL) {
		ret_val = FALSE;
		goto done;
	}

	ret_val = (boolean)EC_POINT_get_affine_coordinates(ec_group, ec_point,
							   bn_x, bn_y, NULL);
	if (!ret_val) {
		goto done;
	}

	x_size = BN_num_bytes(bn_x);
	y_size = BN_num_bytes(bn_y);
	if (x_size <= 0 || y_size <= 0) {
		ret_val = FALSE;
		goto done;
	}
	ASSERT((uintn)x_size <= half_size && (uintn)y_size <= half_size);

	if (public != NULL) {
		zero_mem(public, *public_size);
		BN_bn2bin(bn_x, &public[0 + half_size - x_size]);
		BN_bn2bin(bn_y, &public[half_size + half_size - y_size]);
	}
	ret_val = TRUE;

done:
	if (bn_x != NULL) {
		BN_free(bn_x);
	}
	if (bn_y != NULL) {
		BN_free(bn_y);
	}
	return ret_val;
}

/**
  Computes exchanged common key.

  Given peer's public key (X, Y), this function computes the exchanged common key,
  based on its own context including value of curve parameter and random secret.
  X is the first half of peer_public with size being peer_public_size / 2,
  Y is the second half of peer_public with size being peer_public_size / 2.

  If ec_context is NULL, then return FALSE.
  If peer_public is NULL, then return FALSE.
  If peer_public_size is 0, then return FALSE.
  If key is NULL, then return FALSE.
  If key_size is not large enough, then return FALSE.

  For P-256, the peer_public_size is 64. first 32-byte is X, second 32-byte is Y. The key_size is 32.
  For P-384, the peer_public_size is 96. first 48-byte is X, second 48-byte is Y. The key_size is 48.
  For P-521, the peer_public_size is 132. first 66-byte is X, second 66-byte is Y. The key_size is 66.

  @param[in, out]  ec_context          Pointer to the EC context.
  @param[in]       peer_public         Pointer to the peer's public X,Y.
  @param[in]       peer_public_size     size of peer's public X,Y in bytes.
  @param[out]      key                Pointer to the buffer to receive generated key.
  @param[in, out]  key_size            On input, the size of key buffer in bytes.
                                      On output, the size of data returned in key buffer in bytes.

  @retval TRUE   EC exchanged key generation succeeded.
  @retval FALSE  EC exchanged key generation failed.
  @retval FALSE  key_size is not large enough.

**/
boolean ec_compute_key(IN OUT void *ec_context, IN const uint8 *peer_public,
		       IN uintn peer_public_size, OUT uint8 *key,
		       IN OUT uintn *key_size)
{
	EC_KEY *ec_key;
	const EC_GROUP *ec_group;
	boolean ret_val;
	BIGNUM *bn_x;
	BIGNUM *bn_y;
	EC_POINT *ec_point;
	int32 openssl_nid;
	uintn half_size;
	intn size;

	if (ec_context == NULL || peer_public == NULL || key_size == NULL ||
	    key == NULL) {
		return FALSE;
	}

	if (peer_public_size > INT_MAX) {
		return FALSE;
	}

	ec_key = (EC_KEY *)ec_context;
	openssl_nid = EC_GROUP_get_curve_name(EC_KEY_get0_group(ec_key));
	switch (openssl_nid) {
	case NID_X9_62_prime256v1:
		half_size = 32;
		break;
	case NID_secp384r1:
		half_size = 48;
		break;
	case NID_secp521r1:
		half_size = 66;
		break;
	default:
		return FALSE;
	}
	if (peer_public_size != half_size * 2) {
		return FALSE;
	}

	ec_group = EC_KEY_get0_group(ec_key);
	ec_point = NULL;

	bn_x = BN_bin2bn(peer_public, (uint32)half_size, NULL);
	bn_y = BN_bin2bn(peer_public + half_size, (uint32)half_size, NULL);
	if (bn_x == NULL || bn_y == NULL) {
		ret_val = FALSE;
		goto done;
	}
	ec_point = EC_POINT_new(ec_group);
	if (ec_point == NULL) {
		ret_val = FALSE;
		goto done;
	}

	ret_val = (boolean)EC_POINT_set_affine_coordinates(ec_group, ec_point,
							   bn_x, bn_y, NULL);
	if (!ret_val) {
		goto done;
	}

	size = ECDH_compute_key(key, *key_size, ec_point, ec_key, NULL);
	if (size < 0) {
		ret_val = FALSE;
		goto done;
	}

	if (*key_size < (uintn)size) {
		*key_size = size;
		ret_val = FALSE;
		goto done;
	}

	*key_size = size;

	ret_val = TRUE;

done:
	if (bn_x != NULL) {
		BN_free(bn_x);
	}
	if (bn_y != NULL) {
		BN_free(bn_y);
	}
	if (ec_point != NULL) {
		EC_POINT_free(ec_point);
	}
	return ret_val;
}

/**
  Carries out the EC-DSA signature.

  This function carries out the EC-DSA signature.
  If the signature buffer is too small to hold the contents of signature, FALSE
  is returned and sig_size is set to the required buffer size to obtain the signature.

  If ec_context is NULL, then return FALSE.
  If message_hash is NULL, then return FALSE.
  If hash_size need match the hash_nid. hash_nid could be SHA256, SHA384, SHA512, SHA3_256, SHA3_384, SHA3_512.
  If sig_size is large enough but signature is NULL, then return FALSE.

  For P-256, the sig_size is 64. first 32-byte is R, second 32-byte is S.
  For P-384, the sig_size is 96. first 48-byte is R, second 48-byte is S.
  For P-521, the sig_size is 132. first 66-byte is R, second 66-byte is S.

  @param[in]       ec_context    Pointer to EC context for signature generation.
  @param[in]       hash_nid      hash NID
  @param[in]       message_hash  Pointer to octet message hash to be signed.
  @param[in]       hash_size     size of the message hash in bytes.
  @param[out]      signature    Pointer to buffer to receive EC-DSA signature.
  @param[in, out]  sig_size      On input, the size of signature buffer in bytes.
                                On output, the size of data returned in signature buffer in bytes.

  @retval  TRUE   signature successfully generated in EC-DSA.
  @retval  FALSE  signature generation failed.
  @retval  FALSE  sig_size is too small.

**/
boolean ecdsa_sign(IN void *ec_context, IN uintn hash_nid,
		   IN const uint8 *message_hash, IN uintn hash_size,
		   OUT uint8 *signature, IN OUT uintn *sig_size)
{
	EC_KEY *ec_key;
	ECDSA_SIG *ecdsa_sig;
	int32 openssl_nid;
	uint8 half_size;
	BIGNUM *bn_r;
	BIGNUM *bn_s;
	intn r_size;
	intn s_size;

	if (ec_context == NULL || message_hash == NULL) {
		return FALSE;
	}

	if (signature == NULL) {
		return FALSE;
	}

	ec_key = (EC_KEY *)ec_context;
	openssl_nid = EC_GROUP_get_curve_name(EC_KEY_get0_group(ec_key));
	switch (openssl_nid) {
	case NID_X9_62_prime256v1:
		half_size = 32;
		break;
	case NID_secp384r1:
		half_size = 48;
		break;
	case NID_secp521r1:
		half_size = 66;
		break;
	default:
		return FALSE;
	}
	if (*sig_size < (uintn)(half_size * 2)) {
		*sig_size = half_size * 2;
		return FALSE;
	}
	*sig_size = half_size * 2;
	zero_mem(signature, *sig_size);

	switch (hash_nid) {
	case CRYPTO_NID_SHA256:
		if (hash_size != SHA256_DIGEST_SIZE) {
			return FALSE;
		}
		break;

	case CRYPTO_NID_SHA384:
		if (hash_size != SHA384_DIGEST_SIZE) {
			return FALSE;
		}
		break;

	case CRYPTO_NID_SHA512:
		if (hash_size != SHA512_DIGEST_SIZE) {
			return FALSE;
		}
		break;

	default:
		return FALSE;
	}

	ecdsa_sig = ECDSA_do_sign(message_hash, (uint32)hash_size,
				  (EC_KEY *)ec_context);
	if (ecdsa_sig == NULL) {
		return FALSE;
	}

	ECDSA_SIG_get0(ecdsa_sig, (const BIGNUM **)&bn_r,
		       (const BIGNUM **)&bn_s);

	r_size = BN_num_bytes(bn_r);
	s_size = BN_num_bytes(bn_s);
	if (r_size <= 0 || s_size <= 0) {
		ECDSA_SIG_free(ecdsa_sig);
		return FALSE;
	}
	ASSERT((uintn)r_size <= half_size && (uintn)s_size <= half_size);

	BN_bn2bin(bn_r, &signature[0 + half_size - r_size]);
	BN_bn2bin(bn_s, &signature[half_size + half_size - s_size]);

	ECDSA_SIG_free(ecdsa_sig);

	return TRUE;
}

/**
  Verifies the EC-DSA signature.

  If ec_context is NULL, then return FALSE.
  If message_hash is NULL, then return FALSE.
  If signature is NULL, then return FALSE.
  If hash_size need match the hash_nid. hash_nid could be SHA256, SHA384, SHA512, SHA3_256, SHA3_384, SHA3_512.

  For P-256, the sig_size is 64. first 32-byte is R, second 32-byte is S.
  For P-384, the sig_size is 96. first 48-byte is R, second 48-byte is S.
  For P-521, the sig_size is 132. first 66-byte is R, second 66-byte is S.

  @param[in]  ec_context    Pointer to EC context for signature verification.
  @param[in]  hash_nid      hash NID
  @param[in]  message_hash  Pointer to octet message hash to be checked.
  @param[in]  hash_size     size of the message hash in bytes.
  @param[in]  signature    Pointer to EC-DSA signature to be verified.
  @param[in]  sig_size      size of signature in bytes.

  @retval  TRUE   Valid signature encoded in EC-DSA.
  @retval  FALSE  Invalid signature or invalid EC context.

**/
boolean ecdsa_verify(IN void *ec_context, IN uintn hash_nid,
		     IN const uint8 *message_hash, IN uintn hash_size,
		     IN const uint8 *signature, IN uintn sig_size)
{
	int32 result;
	EC_KEY *ec_key;
	ECDSA_SIG *ecdsa_sig;
	int32 openssl_nid;
	uint8 half_size;
	BIGNUM *bn_r;
	BIGNUM *bn_s;

	if (ec_context == NULL || message_hash == NULL || signature == NULL) {
		return FALSE;
	}

	if (sig_size > INT_MAX || sig_size == 0) {
		return FALSE;
	}

	ec_key = (EC_KEY *)ec_context;
	openssl_nid = EC_GROUP_get_curve_name(EC_KEY_get0_group(ec_key));
	switch (openssl_nid) {
	case NID_X9_62_prime256v1:
		half_size = 32;
		break;
	case NID_secp384r1:
		half_size = 48;
		break;
	case NID_secp521r1:
		half_size = 66;
		break;
	default:
		return FALSE;
	}
	if (sig_size != (uintn)(half_size * 2)) {
		return FALSE;
	}

	switch (hash_nid) {
	case CRYPTO_NID_SHA256:
		if (hash_size != SHA256_DIGEST_SIZE) {
			return FALSE;
		}
		break;

	case CRYPTO_NID_SHA384:
		if (hash_size != SHA384_DIGEST_SIZE) {
			return FALSE;
		}
		break;

	case CRYPTO_NID_SHA512:
		if (hash_size != SHA512_DIGEST_SIZE) {
			return FALSE;
		}
		break;

	default:
		return FALSE;
	}

	ecdsa_sig = ECDSA_SIG_new();
	if (ecdsa_sig == NULL) {
		ECDSA_SIG_free(ecdsa_sig);
		return FALSE;
	}

	bn_r = BN_bin2bn(signature, (uint32)half_size, NULL);
	bn_s = BN_bin2bn(signature + half_size, (uint32)half_size, NULL);
	if (bn_r == NULL || bn_s == NULL) {
		if (bn_r != NULL) {
			BN_free(bn_r);
		}
		if (bn_s != NULL) {
			BN_free(bn_s);
		}
		ECDSA_SIG_free(ecdsa_sig);
		return FALSE;
	}
	ECDSA_SIG_set0(ecdsa_sig, bn_r, bn_s);

	result = ECDSA_do_verify(message_hash, (uint32)hash_size, ecdsa_sig,
				 (EC_KEY *)ec_context);

	ECDSA_SIG_free(ecdsa_sig);

	return (result == 1);
}
