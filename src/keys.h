/* Copyright (C) 2012-2019 IBM Corp.
 * This program is Licensed under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *   http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License. See accompanying LICENSE file.
 */

#ifndef HELIB_KEYS_H
#define HELIB_KEYS_H
/**
 * @file keys.h
 * @brief - Declaration of public key
 * @brief - Declaration of secret key
 *
 * Copyright IBM Corporation 2019 All rights reserved.
 */

#include "keySwitching.h"

namespace helib {

#define FHE_KSS_UNKNOWN (0)
// unknown KS strategy

#define FHE_KSS_FULL    (1)
// all KS matrices

#define FHE_KSS_BSGS    (2)
// baby step/giant step strategy

#define FHE_KSS_MIN     (3)
// minimal strategy (for g_i, and for g_i^{-ord_i} for bad dims)

void writePubKeyBinary(std::ostream& str, const FHEPubKey& pk);
void readPubKeyBinary(std::istream& str, FHEPubKey& pk);

/**
 * @class FHEPubKey
 * @brief The public key
 ********************************************************************/
class FHEPubKey { // The public key
    const FHEcontext& context; // The context

private:

    //! @var Ctxt pubEncrKey
    //! The public encryption key is an encryption of 0,
    //! relative to the first secret key
    Ctxt pubEncrKey;

    std::vector<double> skBounds;
    // High-probability bounds on L-infty norm of secret keys

    std::vector<KeySwitch> keySwitching; // The key-switching matrices

    // The keySwitchMap structure contains pointers to key-switching matrices
    // for re-linearizing automorphisms. The entry keySwitchMap[i][n] contains
    // the index j such that keySwitching[j] is the first matrix one needs to
    // use when re-linearizing s_i(X^n).
    std::vector< std::vector<long> > keySwitchMap;

    NTL::Vec<long> KS_strategy; // NTL Vec's support I/O, which is more convenient

    // bootstrapping data

    long recryptKeyID; // index of the bootstrapping key
    Ctxt recryptEkey;  // the key itself, encrypted under key #0

public:
    //! This constructor thorws run-time error if activeContext=NULL
    FHEPubKey();

    explicit FHEPubKey(const FHEcontext& _context);

    //! Copy constructor
    FHEPubKey(const FHEPubKey& other);

    //! Clear all public-key data
    void clear();

    bool operator==(const FHEPubKey& other) const;
    bool operator!=(const FHEPubKey& other) const;

    // Access methods
    const FHEcontext& getContext() const;
    long getPtxtSpace() const;
    bool keyExists(long keyID) const;

    //! @brief The size of the secret key
    double getSKeyBound(long keyID=0) const;

    ///@{
    //! @name Find key-switching matrices
    const std::vector<KeySwitch>& keySWlist() const;

    //! @brief Find a key-switching matrix by its indexes.
    //! If no such matrix exists it returns a dummy matrix with toKeyID==-1.
    const KeySwitch& getKeySWmatrix(const SKHandle& from, long toID=0) const;
    const KeySwitch& getKeySWmatrix(long fromSPower, long fromXPower, long fromID=0, long toID=0) const;

    bool haveKeySWmatrix(const SKHandle& from, long toID=0) const;

    bool haveKeySWmatrix(long fromSPower, long fromXPower, long fromID=0, long toID=0) const;

    //! @brief Is there a matrix from this key to *any* base key?
    const KeySwitch& getAnyKeySWmatrix(const SKHandle& from) const;
    bool haveAnyKeySWmatrix(const SKHandle& from) const;

    //!@brief Get the next matrix to use for multi-hop automorphism
    //! See Section 3.2.2 in the design document
    const KeySwitch& getNextKSWmatrix(long fromXPower, long fromID=0) const;

    ///@}

    //! @brief Is it possible to re-linearize the automorphism X -> X^k
    //! See Section 3.2.2 in the design document (KeySwitchMap)
    bool isReachable(long k, long keyID=0) const;

    //! @brief Compute the reachability graph of key-switching matrices
    //! See Section 3.2.2 in the design document (KeySwitchMap)
    void setKeySwitchMap(long keyId=0);  // Computes the keySwitchMap pointers

    //! @brief get KS strategy for dimension dim
    //! dim == -1 is Frobenius
    long getKSStrategy(long dim) const;

    //! @brief set KS strategy for dimension dim
    //! dim == -1 is Frobenius
    void setKSStrategy(long dim, int val);

    /**
     * Encrypts plaintext, result returned in the ciphertext argument. When
     * called with highNoise=true, returns a ciphertext with noise level
     * approximately q/8. For BGV, ptxtSpace is the intended plaintext
     *     space, which cannot be co-prime with pubEncrKey.ptxtSpace.
     *     The returned value is the plaintext-space for the resulting
     *     ciphertext, which is GCD(ptxtSpace, pubEncrKey.ptxtSpace).
     * For CKKS, ptxtSpace is a bound on the size of the complex plaintext
     *     elements that are encoded in ptxt (before scaling), it is assumed
     *     that they are scaled by context.alMod.encodeScalingFactor(). The
     *     returned value is the same as the argument ptxtSpace.
     **/
    long Encrypt(Ctxt &ciphertxt,
                 const NTL::ZZX& plaintxt, long ptxtSpace, bool highNoise) const;
    long Encrypt(Ctxt &ciphertxt,
                 const zzX& plaintxt, long ptxtSpace, bool highNoise) const;

    void CKKSencrypt(Ctxt &ciphertxt, const NTL::ZZX& plaintxt,
                     double ptxtSize=1.0, double scaling=0.0) const;
    void CKKSencrypt(Ctxt &ciphertxt, const zzX& plaintxt,
                     double ptxtSize=1.0, double scaling=0.0) const;

    // These methods are overridden by secret-key Encrypt
    virtual long Encrypt(Ctxt &ciphertxt, const NTL::ZZX& plaintxt, long ptxtSpace=0) const;
    virtual long Encrypt(Ctxt &ciphertxt, const zzX& plaintxt, long ptxtSpace=0) const;

    bool isCKKS() const;
    // NOTE: Is taking the alMod from the context the right thing to do?

    bool isBootstrappable() const;
    void reCrypt(Ctxt &ctxt); // bootstrap a ciphertext to reduce noise
    void thinReCrypt(Ctxt &ctxt);  // bootstrap a "thin" ciphertext, where
    // slots are assumed to contain constants

    friend class FHESecKey;
    friend std::ostream& operator << (std::ostream& str, const FHEPubKey& pk);
    friend std::istream& operator >> (std::istream& str, FHEPubKey& pk);
    friend void ::helib::writePubKeyBinary(std::ostream& str, const FHEPubKey& pk);
    friend void ::helib::readPubKeyBinary(std::istream& str, FHEPubKey& pk);

    // defines plaintext space for the bootstrapping encrypted secret key
    static long ePlusR(long p);

    // A hack to increase the plaintext space, you'd better
    // know what you are doing when using it.
    void hackPtxtSpace(long p2r) { pubEncrKey.ptxtSpace = p2r; }
};


void writeSecKeyBinary(std::ostream& str, const FHESecKey& sk);
void readSecKeyBinary(std::istream& str, FHESecKey& sk);
/**
 * @class FHESecKey
 * @brief The secret key
******************************************************************/
class FHESecKey: public FHEPubKey { // The secret key
public:
    std::vector<DoubleCRT> sKeys; // The secret key(s) themselves

public:
  // Disable default constructor
  FHESecKey() = delete;

    // Constructors just call the ones for the base class
    explicit
    FHESecKey(const FHEcontext& _context);

    bool operator==(const FHESecKey& other) const;
    bool operator!=(const FHESecKey& other) const;

    //! Clear all secret-key data
    void clear();

    //! We allow the calling application to choose a secret-key polynomial by
    //! itself, then insert it into the FHESecKey object, getting the index of
    //! that secret key in the sKeys list. If this is the first secret-key for
    //! this object then the procedure below also generates a corresponding
    //! public encryption key.
    //! It is assumed that the context already contains all parameters.
    long ImportSecKey(const DoubleCRT& sKey, double bound,
                      long ptxtSpace=0, long maxDegKswitch=3);

    //! Key generation: This procedure generates a single secret key,
    //! pushes it onto the sKeys list using ImportSecKey from above.
    long GenSecKey(long hwt=0, long ptxtSpace=0, long maxDegKswitch=3);

    //! Generate a key-switching matrix and store it in the public key. The i'th
    //! column of the matrix encrypts fromKey*B1*B2*...*B{i-1}*Q under toKey,
    //! relative to the largest modulus (i.e., all primes) and plaintext space p.
    //! Q is the product of special primes, and the Bi's are the products of
    //! primes in the i'th digit. The plaintext space defaults to 2^r, as defined
    //! by context.mod2r.
    void GenKeySWmatrix(long fromSPower, long fromXPower, long fromKeyIdx=0,
                        long toKeyIdx=0, long ptxtSpace=0);

    // Decryption
    void Decrypt(NTL::ZZX& plaintxt, const Ctxt &ciphertxt) const;

    //! @brief Debugging version, returns in f the polynomial
    //! before reduction modulo the ptxtSpace
    void Decrypt(NTL::ZZX& plaintxt, const Ctxt &ciphertxt, NTL::ZZX& f) const;

    //! @brief Symmetric encryption using the secret key.
    long skEncrypt(Ctxt &ctxt, const NTL::ZZX& ptxt, long ptxtSpace, long skIdx) const;
    long skEncrypt(Ctxt &ctxt, const zzX& ptxt, long ptxtSpace, long skIdx) const;

    // These methods override the public-key Encrypt methods
    long Encrypt(Ctxt &ciphertxt, const NTL::ZZX& plaintxt, long ptxtSpace=0) const override;
    long Encrypt(Ctxt &ciphertxt, const zzX& plaintxt, long ptxtSpace=0) const override;

    //! @brief Generate bootstrapping data if needed, returns index of key
    long genRecryptData();

    friend std::ostream& operator << (std::ostream& str, const FHESecKey& sk);
    friend std::istream& operator >> (std::istream& str, FHESecKey& sk);
    friend void ::helib::writeSecKeyBinary(std::ostream& str, const FHESecKey& sk);
    friend void ::helib::readSecKeyBinary(std::istream& str, FHESecKey& sk);
};

//! Choose random c0,c1 such that c0+s*c1 = p*e for a short e
//! Returns a high-probabiliy bound on the L-infty norm
//! of the canonical embedding
double RLWE(DoubleCRT& c0, DoubleCRT& c1, const DoubleCRT &s, long p,
            NTL::ZZ* prgSeed=nullptr);

//! Same as RLWE, but assumes that c1 is already chosen by the caller
double RLWE1(DoubleCRT& c0, const DoubleCRT& c1, const DoubleCRT &s, long p);

}

#endif //HELIB_KEYS_H
