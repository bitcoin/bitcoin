// dsa.h - written and placed in the public domain by Wei Dai

//! \file dsa.h
//! \brief Classes for the DSA signature algorithm

#ifndef CRYPTOPP_DSA_H
#define CRYPTOPP_DSA_H

#include "cryptlib.h"
#include "gfpcrypt.h"

NAMESPACE_BEGIN(CryptoPP)

//! \brief DSA Signature Format
//! \details The DSA signature format used by Crypto++ is as defined by IEEE P1363.
//!   Java nad .Net use the DER format, and OpenPGP uses the OpenPGP format.
enum DSASignatureFormat {
	//! \brief Crypto++ native signature encoding format
	DSA_P1363,
	//! \brief signature encoding format used by Java and .Net
	DSA_DER,
	//! \brief OpenPGP signature encoding format
	DSA_OPENPGP
};

//! \brief Converts between signature encoding formats
//! \param buffer byte buffer for the converted signature encoding
//! \param bufferSize the length of the converted signature encoding buffer
//! \param toFormat the source signature format
//! \param signature byte buffer for the existing signature encoding
//! \param signatureLen the length of the existing signature encoding buffer
//! \param fromFormat the source signature format
//! \details This function converts between these formats, and returns length
//!   of signature in the target format. If <tt>toFormat == DSA_P1363</tt>, then
//!   <tt>bufferSize</tt> must equal <tt>publicKey.SignatureLength()</tt>
size_t DSAConvertSignatureFormat(byte *buffer, size_t bufferSize, DSASignatureFormat toFormat,
	const byte *signature, size_t signatureLen, DSASignatureFormat fromFormat);

NAMESPACE_END

#endif
