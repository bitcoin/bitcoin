declare class PrivateKey {
    static PRIVATE_KEY_SIZE: number;

    static fromSeed(seed: Uint8Array): PrivateKey;

    static fromBytes(bytes: Uint8Array, modOrder: boolean): PrivateKey;

    static aggregate(privateKeys: PrivateKey[], publicKeys: PublicKey[]): PrivateKey;

    static aggregateInsecure(privateKeys: PrivateKey[]): PrivateKey;

    getPublicKey(): PublicKey;

    serialize(): Uint8Array;

    sign(message: Uint8Array): Signature;

    signInsecure(message: Uint8Array): InsecureSignature;

    signPrehashed(messageHash: Uint8Array): Signature;

    delete(): void;
}

declare class InsecureSignature {
    static SIGNATURE_SIZE: number;

    static fromBytes(bytes: Uint8Array);

    static aggregate(signatures: InsecureSignature[]): InsecureSignature;

    verify(hashes: Uint8Array[], pubKeys: PublicKey[]): boolean;

    divideBy(insecureSignatures: InsecureSignature[]): InsecureSignature;

    serialize(): Uint8Array;

    delete(): void;
}

declare class Signature {
    static SIGNATURE_SIZE: number;

    static fromBytes(bytes: Uint8Array): Signature;

    static fromBytesAndAggregationInfo(bytes: Uint8Array, aggregationInfo: AggregationInfo): Signature;

    static aggregateSigs(signatures: Signature[]): Signature;

    serialize(): Uint8Array;

    verify(): boolean;

    getAggregationInfo(): AggregationInfo;

    setAggregationInfo(aggregationInfo: AggregationInfo): void;

    delete(): void;
}

declare class PublicKey {
    static PUBLIC_KEY_SIZE: number;

    static fromBytes(bytes: Uint8Array): PublicKey;

    static aggregate(publicKeys: PublicKey[]): PublicKey;

    static aggregateInsecure(publicKeys: PublicKey[]): PublicKey;

    getFingerprint(): number;

    serialize(): Uint8Array;

    delete(): void;
}

declare class AggregationInfo {
    static fromMsgHash(publicKey: PublicKey, messageHash: Uint8Array): AggregationInfo;

    static fromMsg(publicKey: PublicKey, message: Uint8Array): AggregationInfo;

    static fromBuffers(pubKeys: PublicKey[], msgHashes: Uint8Array[], exponents: Uint8Array[]): AggregationInfo;

    getPublicKeys(): PublicKey[];

    getMessageHashes(): Uint8Array[];

    getExponents(): Uint8Array[];

    delete(): void;
}

declare class ExtendedPrivateKey {
    static EXTENDED_PRIVATE_KEY_SIZE: number;

    static fromSeed(seed: Uint8Array): ExtendedPrivateKey;

    static fromBytes(bytes: Uint8Array): ExtendedPrivateKey;

    privateChild(index: number): ExtendedPrivateKey;

    publicChild(index: number): ExtendedPublicKey;

    getVersion(): number;

    getDepth(): number;

    getParentFingerprint(): number;

    getChildNumber(): number;

    getChainCode(): ChainCode;

    getPrivateKey(): PrivateKey;

    getPublicKey(): PublicKey;

    getExtendedPublicKey(): ExtendedPublicKey;

    serialize(): Uint8Array;

    delete(): void;
}

declare class ExtendedPublicKey {
    static VERSION: number;
    static EXTENDED_PUBLIC_KEY_SIZE: number;

    static fromBytes(bytes: Uint8Array): ExtendedPublicKey;

    publicChild(index: number): ExtendedPublicKey;

    getVersion(): number;

    getDepth(): number;

    getParentFingerprint(): number;

    getChildNumber(): number;

    getPublicKey(): PublicKey;

    getChainCode(): ChainCode;

    serialize(): Uint8Array;

    delete(): void;
}

declare class ChainCode {
    static CHAIN_CODE_SIZE: number;

    static fromBytes(bytes: Uint8Array);

    serialize(): Uint8Array;

    delete(): void;
}

declare class Threshold {
    static create(commitment: PublicKey[], secretFragments: PrivateKey[], threshold: number, playersCount: number): PrivateKey;

    static signWithCoefficient(sk: PrivateKey, message: Uint8Array, playerIndex: number, players: number[]): InsecureSignature;

    static aggregateUnitSigs(signatures: InsecureSignature[], message: Uint8Array, players: number[]): InsecureSignature;

    static verifySecretFragment(playerIndex: number, secretFragment: PrivateKey, commitment: PublicKey[], threshold: number): boolean;
}

interface ModuleInstance {
    then: (callback: (moduleInstance: ModuleInstance) => any) => ModuleInstance;
    PrivateKey: typeof PrivateKey;
    InsecureSignature: typeof InsecureSignature;
    Signature: typeof Signature;
    PublicKey: typeof PublicKey;
    AggregationInfo: typeof AggregationInfo;
    ExtendedPrivateKey: typeof ExtendedPrivateKey;
    ExtendedPublicKey: typeof ExtendedPublicKey;
    ChainCode: typeof ChainCode;
    Threshold: typeof Threshold;
    DHKeyExchange(privateKey: PrivateKey, publicKey: PublicKey);
    GROUP_ORDER: string;
}

declare function createModule(options?: {}): ModuleInstance;

export = createModule;
