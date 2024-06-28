export declare class AugSchemeMPL {
  static skToG1(sk: PrivateKey): G1Element;
  static keyGen(msg: Uint8Array): PrivateKey;
  static sign(sk: PrivateKey, msg: Uint8Array): G2Element;
  static signPrepend(sk: PrivateKey, msg: Uint8Array, prependPk: G1Element): G2Element;
  static verify(pk: G1Element, msg: Uint8Array, sig: G2Element): boolean;
  static aggregate(g2Elements: G2Element[]): G2Element;
  static aggregateVerify(pks: G1Element[], msgs: Uint8Array[], sig: G2Element): boolean;
  static deriveChildSk(sk: PrivateKey, index: number): PrivateKey;
  static deriveChildSkUnhardened(sk: PrivateKey, index: number): PrivateKey;
  static deriveChildPkUnhardened(pk: G1Element, index: number): G1Element;
}

export declare class BasicSchemeMPL {
  static skToG1(sk: PrivateKey): G1Element;
  static keyGen(msg: Uint8Array): PrivateKey;
  static sign(sk: PrivateKey, msg: Uint8Array): G2Element;
  static verify(pk: G1Element, msg: Uint8Array, sig: G2Element): boolean;
  static aggregate(g2Elements: G2Element[]): G2Element;
  static aggregateVerify(pks: G1Element[], msgs: Uint8Array[], sig: G2Element): boolean;
  static deriveChildSk(sk: PrivateKey, index: number): PrivateKey;
  static deriveChildSkUnhardened(sk: PrivateKey, index: number): PrivateKey;
  static deriveChildPkUnhardened(pk: G1Element, index: number): G1Element;
  static verifySecure(pk: G1Element, sig: G2Element, msg: Uint8Array): boolean;
}

export declare class PopSchemeMPL {
  static skToG1(sk: PrivateKey): G1Element;
  static keyGen(msg: Uint8Array): PrivateKey;
  static sign(sk: PrivateKey, msg: Uint8Array): G2Element;
  static verify(pk: G1Element, msg: Uint8Array, sig: G2Element): boolean;
  static aggregate(g2Elements: G2Element[]): G2Element;
  static aggregateVerify(pks: G1Element[], msgs: Uint8Array[], sig: G2Element): boolean;
  static deriveChildSk(sk: PrivateKey, index: number): PrivateKey;
  static deriveChildSkUnhardened(sk: PrivateKey, index: number): PrivateKey;
  static deriveChildPkUnhardened(pk: G1Element, index: number): G1Element;
  static popProve(sk: PrivateKey): G2Element;
  static popVerify(pk: G1Element, signatureProof: G2Element): boolean;
  static fastAggregateVerify(pks: G1Element[], msg: Uint8Array, sig: G2Element): boolean;
}

export declare class G1Element {
  static SIZE: number;
  static fromBytes(bytes: Uint8Array): G1Element;
  static generator(): G2Element;
  serialize(): Uint8Array;
  negate(): G1Element;
  deepcopy(): G1Element;
  getFingerprint(): number;
  add(el: G1Element): G1Element;
  mul(bn: Bignum): G1Element;
  equalTo(el: G1Element): boolean;
  delete(): void;
}

export declare class G2Element {
  static SIZE: number;
  static fromBytes(bytes: Uint8Array): G2Element;
  static fromG2(sk: G2Element): G2Element;
  static aggregateSigs(sigs: G2Element[]): G2Element;
  static generator(): G2Element;
  serialize(): Uint8Array;
  negate(): G2Element;
  deepcopy(): G2Element;
  add(el: G2Element): G2Element;
  mul(bn: Bignum): G2Element;
  equalTo(el: G2Element): boolean;
  delete(): void;
}

export declare class PrivateKey {
  static PRIVATE_KEY_SIZE: number;
  static fromBytes(bytes: Uint8Array, modOrder: boolean): PrivateKey;
  static aggregate(pks: PrivateKey[]): PrivateKey;
  deepcopy(): PrivateKey;
  serialize(): Uint8Array;
  getG1(): G1Element;
  getG2(): G2Element;
  mulG1(el: G1Element): G1Element;
  mulG2(el: G2Element): G2Element;
  equalTo(key: PrivateKey): boolean;
  delete(): void;
}

export declare class Bignum {
  static fromString(s: string, radix: number): Bignum;
  toString(radix: number): string;
  delete(): void;
}

export declare class Util {
  static hash256(msg: Uint8Array): Uint8Array;
  static hexStr(msg: Uint8Array): string;
}

export interface ModuleInstance {
  AugSchemeMPL: typeof AugSchemeMPL;
  BasicSchemeMPL: typeof BasicSchemeMPL;
  PopSchemeMPL: typeof PopSchemeMPL;
  G1Element: typeof G1Element;
  G2Element: typeof G2Element;
  PrivateKey: typeof PrivateKey;
  Bignum: typeof Bignum;
  Util: typeof Util;
}

export default function createModule(options?: {}): Promise<ModuleInstance>;
