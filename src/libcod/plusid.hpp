// Plus ID: an ECDSA P-256 key pair that a client proves at connect by signing
// SHA-256("cod2plus-id-v1|<challenge>|<server ip:port>"). The client sends
// pk (compressed point, 66 lowercase hex), ps (r||s, 128 lowercase hex) and
// pa (the ip:port it signed); the ID is "P-" and the base32 of the first 10
// bytes of SHA-256(pk) in four groups of four. No engine headers and no
// state: the OpenCoD2-Plus client vendors this file to sign.
// See docs/architecture/plus-id.md.
#ifndef PLUSID_HPP
#define PLUSID_HPP

#include <stdio.h>
#include <string.h>

#include "mbedtls/ctr_drbg.h"
#include "mbedtls/ecdsa.h"
#include "mbedtls/entropy.h"
#include "mbedtls/sha256.h"

#define PLUSID_PK_HEX 66
#define PLUSID_PS_HEX 128

inline bool PlusId_FromPublicKey(const unsigned char pk[33], char out[22])
{
	static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
	unsigned char hash[32];

	if ( mbedtls_sha256(pk, 33, hash, 0) != 0 )
		return false;

	// 10 bytes are 80 bits, 16 base32 characters with no padding
	char *p = out;
	*p++ = 'P';
	for ( int i = 0; i < 16; i++ )
	{
		int bit = i * 5, value = ( ( hash[bit / 8] << 8 ) | ( bit / 8 + 1 < 10 ? hash[bit / 8 + 1] : 0 ) ) >> ( 11 - bit % 8 );

		if ( i % 4 == 0 )
			*p++ = '-';
		*p++ = alphabet[value & 31];
	}
	*p = '\0';
	return true;
}

inline void PlusId_SignedText(int challenge, const char *serverAdr, char *out, size_t size)
{
	snprintf(out, size, "cod2plus-id-v1|%d|%s", challenge, serverAdr);
}

// lowercase only and exactly 2 * size characters
inline bool PlusId_FromHex(const char *hex, unsigned char *out, size_t size)
{
	if ( strlen(hex) != size * 2 )
		return false;

	for ( size_t i = 0; i < size * 2; i++ )
	{
		char c = hex[i];
		int value = c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : -1;

		if ( value < 0 )
			return false;
		out[i / 2] = i % 2 ? ( out[i / 2] << 4 ) | value : value;
	}
	return true;
}

inline void PlusId_ToHex(const unsigned char *in, size_t size, char *out)
{
	for ( size_t i = 0; i < size; i++ )
		snprintf(out + i * 2, 3, "%02x", in[i]);
}

inline bool PlusId_TextHash(int challenge, const char *serverAdr, unsigned char hash[32])
{
	char text[128];

	PlusId_SignedText(challenge, serverAdr, text, sizeof(text));
	return mbedtls_sha256((const unsigned char *)text, strlen(text), hash, 0) == 0;
}

// an ECDSA P-256 signature (r||s) of a SHA-256 hash by a compressed key;
// the shared ban lists are signed the same way (plusid_lists.cpp)
inline bool PlusId_VerifyDigest(const unsigned char pk[33], const unsigned char hash[32], const unsigned char rs[64])
{
	mbedtls_ecp_group grp;
	mbedtls_ecp_point q;
	mbedtls_mpi r, s;
	bool ok;

	mbedtls_ecp_group_init(&grp);
	mbedtls_ecp_point_init(&q);
	mbedtls_mpi_init(&r);
	mbedtls_mpi_init(&s);

	// reading a compressed point derives y, so a point not on the curve fails here
	ok = mbedtls_ecp_group_load(&grp, MBEDTLS_ECP_DP_SECP256R1) == 0
	     && ( pk[0] == 0x02 || pk[0] == 0x03 )
	     && mbedtls_ecp_point_read_binary(&grp, &q, pk, 33) == 0
	     && mbedtls_ecp_check_pubkey(&grp, &q) == 0
	     && mbedtls_mpi_read_binary(&r, rs, 32) == 0
	     && mbedtls_mpi_read_binary(&s, rs + 32, 32) == 0
	     && mbedtls_ecdsa_verify(&grp, hash, 32, &q, &r, &s) == 0;

	mbedtls_mpi_free(&s);
	mbedtls_mpi_free(&r);
	mbedtls_ecp_point_free(&q);
	mbedtls_ecp_group_free(&grp);
	return ok;
}

inline bool PlusId_Verify(const char *pkHex, const char *psHex, int challenge, const char *serverAdr, char idOut[22])
{
	unsigned char pk[33], ps[64], hash[32];

	return PlusId_FromHex(pkHex, pk, sizeof(pk)) && PlusId_FromHex(psHex, ps, sizeof(ps)) && PlusId_TextHash(challenge, serverAdr, hash)
	       && PlusId_VerifyDigest(pk, hash, ps) && PlusId_FromPublicKey(pk, idOut);
}

// A DRBG seeded from the system entropy, for key generation and signing
struct PlusId_Random
{
	mbedtls_entropy_context entropy;
	mbedtls_ctr_drbg_context drbg;
	bool ok;

	PlusId_Random()
	{
		static const char personal[] = "cod2plus-id";

		mbedtls_entropy_init(&entropy);
		mbedtls_ctr_drbg_init(&drbg);
		ok = mbedtls_ctr_drbg_seed(&drbg, mbedtls_entropy_func, &entropy, (const unsigned char *)personal, sizeof(personal) - 1) == 0;
	}

	~PlusId_Random()
	{
		mbedtls_ctr_drbg_free(&drbg);
		mbedtls_entropy_free(&entropy);
	}
};

inline bool PlusId_Generate(unsigned char priv[32])
{
	PlusId_Random random;
	mbedtls_ecp_group grp;
	mbedtls_mpi d;
	bool ok;

	mbedtls_ecp_group_init(&grp);
	mbedtls_mpi_init(&d);

	ok = random.ok
	     && mbedtls_ecp_group_load(&grp, MBEDTLS_ECP_DP_SECP256R1) == 0
	     && mbedtls_ecp_gen_privkey(&grp, &d, mbedtls_ctr_drbg_random, &random.drbg) == 0
	     && mbedtls_mpi_write_binary(&d, priv, 32) == 0;

	mbedtls_mpi_free(&d);
	mbedtls_ecp_group_free(&grp);
	return ok;
}

inline bool PlusId_Sign(const unsigned char priv[32], int challenge, const char *serverAdr, char pkHex[67], char psHex[129])
{
	PlusId_Random random;
	unsigned char pk[33], ps[64], hash[32];
	size_t length;
	mbedtls_ecp_group grp;
	mbedtls_ecp_point q;
	mbedtls_mpi d, r, s;
	bool ok;

	mbedtls_ecp_group_init(&grp);
	mbedtls_ecp_point_init(&q);
	mbedtls_mpi_init(&d);
	mbedtls_mpi_init(&r);
	mbedtls_mpi_init(&s);

	ok = random.ok
	     && PlusId_TextHash(challenge, serverAdr, hash)
	     && mbedtls_ecp_group_load(&grp, MBEDTLS_ECP_DP_SECP256R1) == 0
	     && mbedtls_mpi_read_binary(&d, priv, 32) == 0
	     && mbedtls_ecp_check_privkey(&grp, &d) == 0
	     && mbedtls_ecp_mul(&grp, &q, &d, &grp.G, mbedtls_ctr_drbg_random, &random.drbg) == 0
	     && mbedtls_ecp_point_write_binary(&grp, &q, MBEDTLS_ECP_PF_COMPRESSED, &length, pk, sizeof(pk)) == 0
	     && length == sizeof(pk)
	     && mbedtls_ecdsa_sign(&grp, &r, &s, &d, hash, sizeof(hash), mbedtls_ctr_drbg_random, &random.drbg) == 0
	     && mbedtls_mpi_write_binary(&r, ps, 32) == 0
	     && mbedtls_mpi_write_binary(&s, ps + 32, 32) == 0;

	if ( ok )
	{
		PlusId_ToHex(pk, sizeof(pk), pkHex);
		PlusId_ToHex(ps, sizeof(ps), psHex);
	}

	mbedtls_mpi_free(&s);
	mbedtls_mpi_free(&r);
	mbedtls_mpi_free(&d);
	mbedtls_ecp_point_free(&q);
	mbedtls_ecp_group_free(&grp);
	return ok;
}

#endif
