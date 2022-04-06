# L2準同型暗号ライブラリshe

# 概要
she(somewhat homomorphic encryption)はペアリングベースのL2準同型暗号と呼ばれる公開鍵暗号ライブラリである。
L2準同型暗号とは暗号文同士の加算を複数回、乗算を一度だけできる性質を表す。

特に2個の整数値ベクトルx = (x_i), y = (y_i)の各要素が暗号化された状態で、その2個のベクトルの内積を暗号化したまま計算できる。

ΣEnc(x_i) Enc(y_i) = Enc(Σx_i y_i).

# 特長
* ペアリングベースの最新アルゴリズムを実装
    * [Efficient Two-level Homomorphic Encryption in Prime-order Bilinear Groups and A Fast Implementation in WebAssembly : ASIA CCS2018](http://asiaccs2018.org/?page_id=632)
* C++版はWindows(x64), Linux(x64, ARM64), OSX(x64)に対応
* JavaScript(WebAssembly 以降JSと記す)版はChrome, Firefox, Edge, Safari(Android, iPhone含む), Node.jsに対応

# クラスと主な機能

## 主なクラス
* 秘密鍵クラス SecretKey
* 公開鍵クラス PublicKey
* 暗号文クラス CipherTextG1, CipherTextG2, CipherTextGT
* ゼロ知識証明クラス ZkpBin, ZkpEq, ZkpBinEq

## 暗号化と復号方法
* 秘密鍵から公開鍵を作成する
* 公開鍵を用いて整数から暗号文を作る
* 秘密鍵を用いて暗号文を復号する

## 暗号文同士の計算
* 同じ暗号文クラス同士は加算・減算できる
* CipherTextG1とCipherTextG2を乗算するとCipherTextGTになる

## 復号の重要な注意点
* このsheは復号時に小さな離散対数問題(DLP)を解く必要がある
* DLPのテーブルサイズをs、暗号文をEnc(m)とすると復号時間はm/sに比例する
* テーブルサイズの設定は`setRangeForDLP(s)`を使う
    * `m/s`の最大値は`setTryNum(tryNum)`で行う

## ゼロ知識証明クラス
* mを暗号するときに同時にゼロ知識証明を生成する
* 暗号文と生成されたゼロ知識証明と公開鍵でmに関する制約条件を検証できる

# JS版

## Node.jsでの読み込み

```
>npm install she-wasm
>node
>const she = require('she-wasm')
```

## ブラウザでの読み込み
[she-wasm](https://github.com/herumi/she-wasm/)のshe.js, she\_c.js, she\_c.wasmファイルを同じディレクトリに置いてshe.jsを読み込む
```
// HTML
<script src="she.js"></script>
```

## JS版サンプル

```
// システムの初期化
she.init().then(() => {
  const sec = new she.SecretKey()
  // 秘密鍵の初期化
  sec.setByCSPRNG()

  // 秘密鍵secから公開鍵pubを作成
  const pub = sec.getPublicKey()

  const m1 = 1
  const m2 = 2
  const m3 = 3
  const m4 = -1

  // 平文m1とm2をCipherTextG1として暗号化
  const c11 = pub.encG1(m1)
  const c12 = pub.encG1(m2)

  // 平文m3とm4をCipherTextG2として暗号化
  const c21 = pub.encG2(m3)
  const c22 = pub.encG2(m4)

  // c11とc12, c21とc22をそれぞれ加算
  const c1 = she.add(c11, c12)
  const c2 = she.add(c21, c22)

  // c1とc2を乗算するとCipherTextGT型になる
  const ct = she.mul(c1, c2)

  // 暗号文ctを復号する
  console.log(`(${m1} + ${m2}) * (${m3} + ${m4}) = ${sec.dec(ct)}`)
})
```

# C++版サンプル
ライブラリのビルドは[mcl](https://github.com/herumi/mcl/#installation-requirements)を参照
```
#include <mcl/she.hpp>
int main()
    try
{
    using namespace mcl::she;
    // システのム初期化
    init();

    SecretKey sec;

    // 秘密鍵の初期化
    sec.setByCSPRNG();

    // 秘密鍵secから公開鍵pubを作成
    PublicKey pub;
    sec.getPublicKey(pub);

    int m1 = 1;
    int m2 = 2;
    int m3 = 3;
    int m4 = -1;

    // 平文m1とm2をCipherTextG1として暗号化
    CipherTextG1 c11, c12;
    pub.enc(c11, m1);
    pub.enc(c12, m2);

    // 平文m3とm4をCipherTextG2として暗号化
    CipherTextG2 c21, c22;
    pub.enc(c21, m3);
    pub.enc(c22, m4);

    // c11とc12, c21とc22をそれぞれ加算
    CipherTextG1 c1;
    CipherTextG2 c2;
    CipherTextG1::add(c1, c11, c12);
    CipherTextG2::add(c2, c21, c22);

    // c1とc2を乗算するとCipherTextGT型になる
    CipherTextGT ct;
    CipherTextGT::mul(ct, c1, c2);

    // 暗号文ctを復号する
    printf("(%d + %d) * (%d + %d) = %d\n", m1, m2, m3, m4, (int)sec.dec(ct));
} catch (std::exception& e) {
    printf("ERR %s\n", e.what());
    return 1;
}

```

# クラス共通メソッド

## シリアライズ(C++)

* `setStr(const std::string& str, int ioMode = 0)`
    * ioModeに従ってstrで設定する

* `getStr(std::string& str, int ioMode = 0) const`
* `std::string getStr(int ioMode = 0) const`
    * ioModeに従ってstrを取得する
* `size_t serialize(void *buf, size_t maxBufSize) const`
    * maxBufSize確保されたbufにシリアライズする
    * bufに書き込まれたbyte長が返る
    * エラーの場合は0が返る
* `size_t deserialize(const void *buf, size_t bufSize)`
    * bufから最大bufSizeまで値を読み込みデリシアライズする
    * 読み込まれたbyte長が返る
    * エラーの場合は0が返る

## シリアライズ(JS)

* `deserialize(s)`
    * Uint8Array型sでデシリアライズ
* `serialize()`
    * シリアライズしてUint8Arrayの値を返す
* `deserializeHexStr(s)`
    * 16進数文字列sでデシリアライズ
* `serializeToHexStr()`
    * 16進数文字列sでシリアライズ

## ioMode

* 2 ; 2進数
* 10 ; 10進数
* 16 ; 16進数
* IoPrefix ; 2または16とorの値を設定すると0bまたは0xがつく
* IoEcAffine ; (G1, G2のみ)アフィン座標
* IoEcProj ; (G1, G2のみ)射影座標
* IoSerialize ; serialize()/deserialize()と同じ

## 注意
* C++の名前空間は`mcl::she`
* 以下CTはCipherTextG1, CipherTextG2, CipherTextGTのいずれかを表す
* JS版の平文は32ビット整数の範囲に制限される

## SecretKeyクラス

* `void setByCSPRNG()`(C++)
* `void setByCSPRNG()`(JS)
    * 疑似乱数で秘密鍵を初期化する

* `int64_t dec(const CT& c) const`(C++)
* `int dec(CT c)`(JS)
    * 暗号文cを復号する
* `int64_t decViaGT(const CipherTextG1& c) const`(C++)
* `int64_t decViaGT(const CipherTextG2& c) const`(C++)
* `int decViaGT(CT c)`(JS)
    * 暗号文をGT経由で復号する
* `bool isZero(const CT& c) const`(C++)
* `bool isZero(CT c)`(JS)
    * cの復号結果が0ならばtrue
    * decしてから0と比較するよりも高速

## PublicKey, PrecomputedPublicKeyクラス
PrecomputedPublicKeyはPublicKeyの高速版

* `void PrecomputedPublicKey::init(const PublicKey& pub)`(C++)
* `void PrecomputedPublicKey::init(pub)`(JS)
    * 公開鍵pubでPrecomputedPublicKeyを初期化する


* `PrecomputedPublicKey::destroy()`(JS)
    * JavaScriptではPrecomputedPublicKeyが不要になったらこのメソッドを呼ぶ必要がある
    * そうしないとメモリリークする

以下はPK = PublicKey or PrecomputedPublicKey

* `void PK::enc(CT& c, int64_t m) const`(C++)
* `CipherTextG1 PK::encG1(m)`(JS)
* `CipherTextG2 PK::encG2(m)`(JS)
* `CipherTextGT PK::encGT(m)`(JS)
    * mを暗号化してcにセットする(またはその値を返す)

* `void PK::reRand(CT& c) const`(C++)
* `CT PK::reRand(CT c)`(JS)
    * cを再ランダム化する
    * 再ランダム化された暗号文と元の暗号文は同じ平文を暗号化したものかどうか判定できない

* `void convert(CipherTextGT& cm, const CT& ca) const`
* `CipherTextGT convert(CT ca)`
   * 暗号文ca(CipherTextG1かCipherTextG2)をCipherTextGTに変換する

## CipherTextクラス

* `void CT::add(CT& z, const CT& x const CT& y)`(C++)
* `CT she.add(CT x, CT y)`(JS)
    * 暗号文xと暗号文yを足してzにセットする(またはその値を返す)
* `void CT::sub(CT& z, const CT& x const CT& y)`(C++)
* `CT she.sub(CT x, CT y)`(JS)
    * 暗号文xから暗号文yを引いてzにセットする(またはその値を返す)
* `void CT::neg(CT& y, const CT& x)`(C++)
* `CT she.neg(CT x)`(JS)
    * 暗号文xの符号反転をyにセットする(またはその値を返す)
* `void CT::mul(CT& z, const CT& x, int y)`(C++)
* `CT she.mulInt(CT x, int y)`(JS)
    * 暗号文xを整数倍yしてzにセットする(またはその値を返す)

* `void CipherTextGT::mul(CipherTextGT& z, const CipherTextG1& x, const CipherTextG2& y)`(C++)
* `CipherTextGT she.mul(CipherTextG1 x, CipherTextG2 y)`(JS)
    * 暗号文xと暗号文yを掛けてzにセットする(またはその値を返す)

* `void CipherTextGT::mulML(CipherTextGT& z, const CipherTextG1& x, const CipherTextG2& y)`(C++)
    * 暗号文xと暗号文yを掛けて(Millerループだけして)zにセットする(またはその値を返す)
* `CipherTextGT::finalExp(CipherText& , const CipherTextG1& x, const CipherTextG2& y)`(C++)
    * mul(a, b) = finalExp(mulML(a, b))
    * add(mul(a, b), mul(c, d)) = finalExp(add(mulML(a, b), mulML(c, d)))
    * すなわち積和演算はmulMLしたものを足してから最後に一度finalExpするのがよい

## ゼロ知識証明クラス

### 概要
* ZkpBin 暗号文encGi(m)(i = 1, 2, T)についてm = 0または1であることを復号せずに検証できる
* ZkpEq 暗号文encG1(m1), encG2(m2)についてm1 = m2であることを検証できる
* ZkpBinEq 暗号文encG1(m1), encG2(m2)についてm1 = m2 = 0または1であることを検証できる

### API
- SK = SecretKey
- PK = PublicKey or PrecomputedPublicKey
- AUX = AuxiliaryForZkpDecGT

* `void PK::encWithZkpBin(CipherTextG1& c, Zkp& zkp, int m) const`(C++)
* `void PK::encWithZkpBin(CipherTextG2& c, Zkp& zkp, int m) const`(C++)
* `[CipherTextG1, ZkpBin] PK::encWithZkpBinG1(m)`(JS)
* `[CipherTextG2, ZkpBin] PK::encWithZkpBinG2(m)`(JS)
    * m(=0 or 1)を暗号化して暗号文cとゼロ知識証明zkpをセットする(または[c, zkp]を返す)
    * mが0でも1でもなければ例外
* `void PK::encWithZkpEq(CipherTextG1& c1, CipherTextG2& c2, ZkpEq& zkp, const INT& m) const`(C++)
* `[CipherTextG1, CipherTextG2, ZkpEq] PK::encWithZkpEq(m)`(JS)
    * mを暗号化して暗号文c1, c2とゼロ知識証明zkpをセットする(または[c1, c2, zkp]を返す)
* `void PK::encWithZkpBinEq(CipherTextG1& c1, CipherTextG2& c2, ZkpBinEq& zkp, int m) const`(C++)
* `[CipherTextG1, CipherTextG2, ZkpEqBin] PK::encWithZkpBinEq(m)`(JS)
    * m(=0 or 1)を暗号化して暗号文c1, c2とゼロ知識証明zkpをセットする(または[c1, c2, zkp]を返す)
    * mが0でも1でもなければ例外
* `SK::decWithZkp(DecZkpDec& zkp, const CipherTextG1& c, const PublicKey& pub) const`(C++)
* `[m, ZkpDecG1] SK::decWithZkpDec(c, pub)`(JS)
  * CipherTextG1暗号文`c`を復号して`m`と`zkp`を返す. `zkp`は`dec(c) = m`の証明
  * `pub`は計算コストを減らすために利用する
* `SK::decWithZkpDec(ZkpDecGT& zkp, const CipherTextGT& c, const AuxiliaryForZkpDecGT& aux) const`(C++)
* `[m, ZkpDecGT] SK::decWithZkpDecGT(c, aux)`(JS)
  * CipherTextGT暗号文`c`を復号して`m`と`zkp`を返す. `zkp`は`dec(c) = m`の証明
  * `aux = pub.getAuxiliaryForZkpDecGT()`. auxは計算コストを減らすために利用する

## グローバル関数

* `void init(const CurveParam& cp, size_t hashSize = 1024, size_t tryNum = 2048)`(C++)
* `void init(curveType = she.BN254, hashSize = 1024, tryNum = 2048)`(JS)
    * hashSize * 8 byteの大きさの復号用テーブルとtryNumを元に初期化する
    * 復号可能な平文mの範囲は|m| <= hashSize * tryNum
* `she.loadTableForGTDLP(Uint8Array a)`(JS)
    * 復号用テーブルを読み込む
    * 現在は`https://herumi.github.io/she-dlp-table/she-dlp-0-20-gt.bin`のみがある
* `void useDecG1ViaGT(bool use)`(C++/JS)
* `void useDecG2ViaGT(bool use)`(C++/JS)
    * CipherTextG1, CipherTextG2の復号をCipherTextGT経由で行う
    * 大きな値を復号するときはDLP用の巨大なテーブルをそれぞれに持つよりもGTに集約した方が効率がよい

# ライセンス

このライブラリは[修正BSDライセンス](https://github.com/herumi/mcl/blob/master/COPYRIGHT)で提供されます

# 開発者

光成滋生 MITSUNARI Shigeo(herumi@nifty.com)
