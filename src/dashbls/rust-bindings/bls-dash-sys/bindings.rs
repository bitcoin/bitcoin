pub type G1Element = *mut ::std::os::raw::c_void;
pub type G2Element = *mut ::std::os::raw::c_void;
pub type PrivateKey = *mut ::std::os::raw::c_void;
pub type CoreMPL = *mut ::std::os::raw::c_void;
pub type BasicSchemeMPL = CoreMPL;
pub type AugSchemeMPL = CoreMPL;
pub type PopSchemeMPL = CoreMPL;
pub type LegacySchemeMPL = CoreMPL;
pub type BIP32ExtendedPrivateKey = *mut ::std::os::raw::c_void;
pub type BIP32ExtendedPublicKey = *mut ::std::os::raw::c_void;
pub type BIP32ChainCode = *mut ::std::os::raw::c_void;

extern "C" {
    pub fn G1ElementSize() -> ::std::os::raw::c_int;

    pub fn G1ElementFromBytes(
        data: *const ::std::os::raw::c_void,
        legacy: bool,
        didErr: *mut bool,
    ) -> G1Element;

    pub fn G1ElementGenerator() -> G1Element;

    pub fn G1ElementIsValid(el: G1Element) -> bool;

    pub fn G1ElementGetFingerprint(el: G1Element, legacy: bool) -> u32;

    pub fn G1ElementIsEqual(el1: G1Element, el2: G1Element) -> bool;

    pub fn G1ElementAdd(el1: G1Element, el2: G1Element) -> G1Element;

    pub fn G1ElementMul(el: G1Element, sk: PrivateKey) -> G1Element;

    pub fn G1ElementNegate(el: G1Element) -> G1Element;

    pub fn G1ElementCopy(el: G1Element) -> G1Element;

    pub fn G1ElementSerialize(el: G1Element, legacy: bool) -> *mut ::std::os::raw::c_void;

    pub fn G1ElementFree(el: G1Element);

    pub fn G2ElementSize() -> ::std::os::raw::c_int;

    pub fn G2ElementFromBytes(
        data: *const ::std::os::raw::c_void,
        legacy: bool,
        didErr: *mut bool,
    ) -> G2Element;

    pub fn G2ElementGenerator() -> G2Element;

    pub fn G2ElementIsValid(el: G2Element) -> bool;

    pub fn G2ElementIsEqual(el1: G2Element, el2: G2Element) -> bool;

    pub fn G2ElementAdd(el1: G2Element, el2: G2Element) -> G2Element;

    pub fn G2ElementMul(el: G2Element, sk: PrivateKey) -> G2Element;

    pub fn G2ElementNegate(el: G2Element) -> G2Element;

    pub fn G2ElementCopy(el: G2Element) -> G2Element;

    pub fn G2ElementSerialize(el: G2Element, legacy: bool) -> *mut ::std::os::raw::c_void;

    pub fn G2ElementFree(el: G2Element);

    pub fn PrivateKeyFromBytes(
        data: *const ::std::os::raw::c_void,
        modOrder: bool,
        didErr: *mut bool,
    ) -> PrivateKey;

    pub fn PrivateKeyFromSeedBIP32(data: *const ::std::os::raw::c_void, len: usize) -> PrivateKey;

    pub fn PrivateKeyAggregate(sks: *mut *mut ::std::os::raw::c_void, len: usize) -> PrivateKey;

    pub fn PrivateKeyGetG1Element(sk: PrivateKey, didErr: *mut bool) -> G1Element;

    pub fn PrivateKeyGetG2Element(sk: PrivateKey, didErr: *mut bool) -> G2Element;

    pub fn PrivateKeyGetG2Power(sk: PrivateKey, el: G2Element) -> G2Element;

    pub fn PrivateKeyIsEqual(sk1: PrivateKey, sk2: PrivateKey) -> bool;

    pub fn PrivateKeySerialize(sk: PrivateKey) -> *mut ::std::os::raw::c_void;

    pub fn PrivateKeyFree(sk: PrivateKey);

    pub fn PrivateKeySizeBytes() -> usize;

    pub fn SecFree(p: *mut ::std::os::raw::c_void);

    pub fn AllocPtrArray(len: usize) -> *mut *mut ::std::os::raw::c_void;

    pub fn SetPtrArray(
        arrPtr: *mut *mut ::std::os::raw::c_void,
        elemPtr: *mut ::std::os::raw::c_void,
        index: ::std::os::raw::c_int,
    );

    pub fn FreePtrArray(inPtr: *mut *mut ::std::os::raw::c_void);

    pub fn GetPtrAtIndex(
        arrPtr: *mut *mut ::std::os::raw::c_void,
        index: ::std::os::raw::c_int,
    ) -> *mut ::std::os::raw::c_void;

    pub fn SecAllocBytes(len: usize) -> *mut u8;

    pub fn GetAddressAtIndex(
        ptr: *mut u8,
        index: ::std::os::raw::c_int,
    ) -> *mut ::std::os::raw::c_void;

    pub fn GetLastErrorMsg() -> *const ::std::os::raw::c_char;

    pub fn CoreMPLKeyGen(
        scheme: CoreMPL,
        seed: *const ::std::os::raw::c_void,
        seedLen: usize,
        didErr: *mut bool,
    ) -> PrivateKey;

    pub fn CoreMPLSkToG1(scheme: CoreMPL, sk: PrivateKey) -> G1Element;

    pub fn CoreMPLSign(
        scheme: CoreMPL,
        sk: PrivateKey,
        msg: *const ::std::os::raw::c_void,
        msgLen: usize,
    ) -> G2Element;

    pub fn CoreMPLVerify(
        scheme: BasicSchemeMPL,
        pk: G1Element,
        msg: *const ::std::os::raw::c_void,
        msgLen: usize,
        sig: G2Element,
    ) -> bool;

    pub fn CoreMPLVerifySecure(
        scheme: CoreMPL,
        pks: *mut *mut ::std::os::raw::c_void,
        pksLen: usize,
        sig: G2Element,
        msg: *const ::std::os::raw::c_void,
        msgLen: usize,
    ) -> bool;

    pub fn CoreMPLAggregatePubKeys(
        scheme: CoreMPL,
        pubKeys: *mut *mut ::std::os::raw::c_void,
        pkLen: usize,
    ) -> G1Element;

    pub fn CoreMPLAggregateSigs(
        scheme: CoreMPL,
        sigs: *mut *mut ::std::os::raw::c_void,
        sigLen: usize,
    ) -> G2Element;

    pub fn CoreMPLDeriveChildSk(scheme: CoreMPL, sk: PrivateKey, index: u32) -> PrivateKey;

    pub fn CoreMPLDeriveChildSkUnhardened(
        scheme: CoreMPL,
        sk: PrivateKey,
        index: u32,
    ) -> PrivateKey;

    pub fn CoreMPLDeriveChildPkUnhardened(scheme: CoreMPL, sk: G1Element, index: u32) -> G1Element;

    pub fn CoreMPLAggregateVerify(
        scheme: CoreMPL,
        pks: *mut *mut ::std::os::raw::c_void,
        pkLen: usize,
        msgs: *mut *mut ::std::os::raw::c_void,
        msgLens: *const ::std::os::raw::c_void,
        msgLen: usize,
        sig: G2Element,
    ) -> bool;

    pub fn NewBasicSchemeMPL() -> BasicSchemeMPL;

    pub fn BasicSchemeMPLAggregateVerify(
        scheme: BasicSchemeMPL,
        pks: *mut *mut ::std::os::raw::c_void,
        pksLen: usize,
        msgs: *mut *mut ::std::os::raw::c_void,
        msgsLens: *const ::std::os::raw::c_void,
        msgsLen: usize,
        sig: G2Element,
    ) -> bool;

    pub fn BasicSchemeMPLFree(scheme: BasicSchemeMPL);

    pub fn NewAugSchemeMPL() -> AugSchemeMPL;

    pub fn AugSchemeMPLSign(
        scheme: AugSchemeMPL,
        sk: PrivateKey,
        msg: *const ::std::os::raw::c_void,
        msgLen: usize,
    ) -> G2Element;

    pub fn AugSchemeMPLSignPrepend(
        scheme: AugSchemeMPL,
        sk: PrivateKey,
        msg: *const ::std::os::raw::c_void,
        msgLen: usize,
        prepPk: G1Element,
    ) -> G2Element;

    pub fn AugSchemeMPLVerify(
        scheme: AugSchemeMPL,
        pk: G1Element,
        msg: *const ::std::os::raw::c_void,
        msgLen: usize,
        sig: G2Element,
    ) -> bool;

    pub fn AugSchemeMPLAggregateVerify(
        scheme: AugSchemeMPL,
        pks: *mut *mut ::std::os::raw::c_void,
        pksLen: usize,
        msgs: *mut *mut ::std::os::raw::c_void,
        msgsLens: *const ::std::os::raw::c_void,
        msgsLen: usize,
        sig: G2Element,
    ) -> bool;

    pub fn AugSchemeMPLFree(scheme: AugSchemeMPL);

    pub fn NewPopSchemeMPL() -> PopSchemeMPL;

    pub fn PopSchemeMPLPopProve(scheme: PopSchemeMPL, sk: PrivateKey) -> G2Element;

    pub fn PopSchemeMPLPopVerify(scheme: PopSchemeMPL, pk: G1Element, sig: G2Element) -> bool;

    pub fn PopSchemeMPLFastAggregateVerify(
        scheme: PopSchemeMPL,
        pks: *mut *mut ::std::os::raw::c_void,
        pksLen: usize,
        msgs: *const ::std::os::raw::c_void,
        msgsLen: usize,
        sig: G2Element,
    ) -> bool;

    pub fn PopSchemeMPLFree(scheme: PopSchemeMPL);

    pub fn NewLegacySchemeMPL() -> LegacySchemeMPL;

    pub fn LegacySchemeMPLSign(
        scheme: LegacySchemeMPL,
        sk: PrivateKey,
        msg: *const ::std::os::raw::c_void,
        msgLen: usize,
    ) -> G2Element;

    pub fn LegacySchemeMPLSignPrepend(
        scheme: LegacySchemeMPL,
        sk: PrivateKey,
        msg: *const ::std::os::raw::c_void,
        msgLen: usize,
        prepPk: G1Element,
    ) -> G2Element;

    pub fn LegacySchemeMPLVerify(
        scheme: LegacySchemeMPL,
        pk: G1Element,
        msg: *const ::std::os::raw::c_void,
        msgLen: usize,
        sig: G2Element,
    ) -> bool;

    pub fn LegacySchemeMPLVerifySecure(
        scheme: LegacySchemeMPL,
        pks: *mut *mut ::std::os::raw::c_void,
        pksLen: usize,
        sig: G2Element,
        msg: *const ::std::os::raw::c_void,
        msgLen: usize,
    ) -> bool;

    pub fn LegacySchemeMPLAggregateVerify(
        scheme: LegacySchemeMPL,
        pks: *mut *mut ::std::os::raw::c_void,
        pksLen: usize,
        msgs: *mut *mut ::std::os::raw::c_void,
        msgsLens: *const ::std::os::raw::c_void,
        msgsLen: usize,
        sig: G2Element,
    ) -> bool;

    pub fn LegacySchemeMPLFree(scheme: LegacySchemeMPL);

    pub fn ThresholdPrivateKeyShare(
        sks: *mut *mut ::std::os::raw::c_void,
        sksLen: usize,
        hash: *const ::std::os::raw::c_void,
        didErr: *mut bool,
    ) -> PrivateKey;

    pub fn ThresholdPrivateKeyRecover(
        sks: *mut *mut ::std::os::raw::c_void,
        sksLen: usize,
        hashes: *mut *mut ::std::os::raw::c_void,
        hashesLen: usize,
        didErr: *mut bool,
    ) -> PrivateKey;

    pub fn ThresholdPublicKeyShare(
        pks: *mut *mut ::std::os::raw::c_void,
        pksLen: usize,
        hash: *const ::std::os::raw::c_void,
        didErr: *mut bool,
    ) -> G1Element;

    pub fn ThresholdPublicKeyRecover(
        pks: *mut *mut ::std::os::raw::c_void,
        pksLen: usize,
        hashes: *mut *mut ::std::os::raw::c_void,
        hashesLen: usize,
        didErr: *mut bool,
    ) -> G1Element;

    pub fn ThresholdSignatureShare(
        sigs: *mut *mut ::std::os::raw::c_void,
        sigsLen: usize,
        hash: *const ::std::os::raw::c_void,
        didErr: *mut bool,
    ) -> G2Element;

    pub fn ThresholdSignatureRecover(
        sigs: *mut *mut ::std::os::raw::c_void,
        sigsLen: usize,
        hashes: *mut *mut ::std::os::raw::c_void,
        hashesLen: usize,
        didErr: *mut bool,
    ) -> G2Element;

    pub fn ThresholdSign(sk: PrivateKey, hash: *const ::std::os::raw::c_void) -> G2Element;

    pub fn ThresholdVerify(
        pk: G1Element,
        hash: *const ::std::os::raw::c_void,
        sig: G2Element,
    ) -> bool;

    pub fn BIP32ChainCodeSerialize(cc: BIP32ChainCode) -> *mut ::std::os::raw::c_void;

    pub fn BIP32ChainCodeIsEqual(cc1: BIP32ChainCode, cc2: BIP32ChainCode) -> bool;

    pub fn BIP32ChainCodeFree(cc: BIP32ChainCode);

    pub fn BIP32ExtendedPublicKeyFromBytes(
        data: *const ::std::os::raw::c_void,
        legacy: bool,
        didErr: *mut bool,
    ) -> BIP32ExtendedPublicKey;

    pub fn BIP32ExtendedPublicKeyPublicChild(
        pk: BIP32ExtendedPublicKey,
        index: u32,
        legacy: bool,
    ) -> BIP32ExtendedPublicKey;

    pub fn BIP32ExtendedPublicKeyGetChainCode(pk: BIP32ExtendedPublicKey) -> BIP32ChainCode;

    pub fn BIP32ExtendedPublicKeySerialize(
        pk: BIP32ExtendedPublicKey,
        legacy: bool,
    ) -> *mut ::std::os::raw::c_void;

    pub fn BIP32ExtendedPublicKeyIsEqual(
        pk1: BIP32ExtendedPublicKey,
        pk2: BIP32ExtendedPublicKey,
    ) -> bool;

    pub fn BIP32ExtendedPublicKeyGetPublicKey(
        pk: BIP32ExtendedPublicKey,
    ) -> *mut ::std::os::raw::c_void;

    pub fn BIP32ExtendedPublicKeyFree(pk: BIP32ExtendedPublicKey);

    pub fn BIP32ExtendedPrivateKeyFromBytes(
        data: *const ::std::os::raw::c_void,
        didErr: *mut bool,
    ) -> BIP32ExtendedPrivateKey;

    pub fn BIP32ExtendedPrivateKeyFromSeed(
        data: *const ::std::os::raw::c_void,
        len: usize,
        didErr: *mut bool,
    ) -> BIP32ExtendedPrivateKey;

    pub fn BIP32ExtendedPrivateKeyPrivateChild(
        sk: BIP32ExtendedPrivateKey,
        index: u32,
        legacy: bool,
    ) -> BIP32ExtendedPrivateKey;

    pub fn BIP32ExtendedPrivateKeyPublicChild(
        sk: BIP32ExtendedPrivateKey,
        index: u32,
    ) -> BIP32ExtendedPublicKey;

    pub fn BIP32ExtendedPrivateKeyGetChainCode(sk: BIP32ExtendedPrivateKey) -> BIP32ChainCode;

    pub fn BIP32ExtendedPrivateKeySerialize(
        sk: BIP32ExtendedPrivateKey,
    ) -> *mut ::std::os::raw::c_void;

    pub fn BIP32ExtendedPrivateKeyIsEqual(
        sk1: BIP32ExtendedPrivateKey,
        sk2: BIP32ExtendedPrivateKey,
    ) -> bool;

    pub fn BIP32ExtendedPrivateKeyGetPrivateKey(
        sk: BIP32ExtendedPrivateKey,
    ) -> *mut ::std::os::raw::c_void;

    pub fn BIP32ExtendedPrivateKeyGetExtendedPublicKey(
        sk: BIP32ExtendedPrivateKey,
        legacy: bool,
        didErr: *mut bool,
    ) -> BIP32ExtendedPublicKey;

    pub fn BIP32ExtendedPrivateKeyGetPublicKey(
        sk: BIP32ExtendedPrivateKey,
        didErr: *mut bool,
    ) -> *mut ::std::os::raw::c_void;

    pub fn BIP32ExtendedPrivateKeyFree(sk: BIP32ExtendedPrivateKey);
}
