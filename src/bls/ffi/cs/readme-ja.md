# BLS署名のC#バインディング

# 必要環境

* Visual Studio 2017(x64) or later
* C# 7.2 or later
* .NET Framework 4.5.2 or later

# DLLのビルド方法

Visual Studio 2017の64bit用コマンドプロンプトを開いて
```
md work
cd work
git clone https://github.com/herumi/cybozulib_ext
git clone https://github.com/herumi/mcl
git clone https://github.com/herumi/bls
cd bls
mklib dll
```
`bls/bin/*.dll`が作成される。

# サンプルのビルド方法

bls/ffi/cs/bls.slnを開いて実行する。

# クラスとAPI

## API

* `Init(int curveType = BN254);`
    * ライブラリを曲線curveTypeで初期化する。
    * curveType = BN254 or BLS12_381
* `SecretKey ShareSecretKey(in SecretKey[] msk, in Id id);`
    * マスター秘密鍵の列mskに対するidの秘密鍵を生成(共有)する。
* `SecretKey RecoverSecretKey(in SecretKey[] secVec, in Id[] idVec);`
    * 秘密鍵secVecとID idVecのペアから秘密鍵を復元する。
* `PublicKey SharePublicKey(in PublicKey[] mpk, in Id id);`
    * マスター公開鍵の列mpkに対するidの公開鍵を生成(共有)する。
* `PublicKey RecoverPublicKey(in PublicKey[] pubVec, in Id[] idVec);`
    * 公開鍵pubVecとID idVecのペアから公開鍵を復元する。
* `Signature RecoverSign(in Signature[] sigVec, in Id[] idVec);`
    * 署名sigVecとID idVecのペアから署名を復元する。

## Id

識別子クラス

* `byte[] Serialize();`
    * Idをシリアライズする。
* `void Deserialize(byte[] buf);`
    * バイト列bufからIdをデシリアライズする。
* `bool IsEqual(in Id rhs);`
    * 同値判定。
* `void SetDecStr(string s);`
    * 10進数文字列を設定する。
* `void SetHexStr(string s);`
    * 16進数文字列を設定する。
* `void SetInt(int x);`
    * 整数xを設定する。
* `string GetDecStr();`
    * 10進数表記を取得する。
* `string GetHexStr();`
    * 16進数表記を取得する。

## SecretKey

* `byte[] Serialize();`
    * Idをシリアライズする。
* `void Deserialize(byte[] buf);`
    * バイト列bufからSecretKeyをデシリアライズする。
* `bool IsEqual(in SecretKey rhs);`
    * 同値判定。
* `void SetHexStr(string s);`
    * 16進数文字列を設定する。
* `string GetHexStr();`
    * 16進数表記を取得する。
* `void Add(in SecretKey rhs);`
    * 秘密鍵rhsを加算する。
* `void SetByCSPRNG();`
    * 暗号学的乱数で設定する。
* `void SetHashOf(string s);`
    * 文字列sのハッシュ値を設定する。
* `PublicKey GetPublicKey();`
    * 対応する公開鍵を取得する。
* `Signature Sign(string m);`
    * 文字列mの署名を生成する。
* `Signature GetPop();`
    * 自身の秘密鍵による署名(Proof Of Posession)を生成する。

## PublicKey

* `byte[] Serialize();`
    * PublicKeyをシリアライズする。
* `void Deserialize(byte[] buf);`
    * バイト列bufからPublicKeyをデシリアライズする。
* `bool IsEqual(in PublicKey rhs);`
    * 同値判定。
* `void Add(in PublicKey rhs);`
    * 公開鍵rhsを加算する。
* `void SetHexStr(string s);`
    * 16進数文字列を設定する。
* `string GetHexStr();`
    * 16進数表記を取得する。
* `bool Verify(in Signature sig, string m);`
    * 文字列mに対する署名sigの正当性を確認する。
* `bool VerifyPop(in Signature pop);`
    * PoPの正当性を確認する。

## Signature

* `byte[] Serialize();`
    * Signatureをシリアライズする。
* `void Deserialize(byte[] buf);`
    * バイト列bufからSignatureをデシリアライズする。
* `bool IsEqual(in Signature rhs);`
    * 同値判定。
* `void Add(in Signature rhs);`
    * 署名rhsを加算する。
* `void SetHexStr(string s);`
    * 16進数文字列を設定する。
* `string GetHexStr();`
    * 16進数表記を取得する。

## 使い方

### 最小サンプル

```
using static BLS;

Init(BN254); // ライブラリ初期化
SecretKey sec;
sec.SetByCSPRNG(); // 秘密鍵の初期化
PublicKey pub = sec.GetPublicKey(); // 公開鍵の取得
string m = "abc";
Signature sig = sec.Sign(m); // 署名の作成
if (pub.Verify(sig, m))) {
  // 署名の確認
}
```

### 集約署名
```
Init(BN254); // ライブラリ初期化
const int n = 10;
const string m = "abc";
SecretKey[] secVec = new SecretKey[n];
PublicKey[] pubVec = new PublicKey[n];
Signature[] popVec = new Signature[n];
Signature[] sigVec = new Signature[n];

for (int i = 0; i < n; i++) {
  secVec[i].SetByCSPRNG(); // 秘密鍵の初期化
  pubVec[i] = secVec[i].GetPublicKey(); // 公開鍵の取得
  popVec[i] = secVec[i].GetPop(); // 所有(PoP)の証明
  sigVec[i] = secVec[i].Sign(m); // 署名
}

SecretKey secAgg;
PublicKey pubAgg;
Signature sigAgg;
for (int i = 0; i < n; i++) {
  // PoPの確認
  if (pubVec[i].VerifyPop(popVec[i]))) {
    // エラー
    return;
  }
  pubAgg.Add(pubVec[i]); // 公開鍵の集約
  sigAgg.Add(sigVec[i]); // 署名の集約
}
if (pubAgg.Verify(sigAgg, m)) {
  // 署名の確認
}
```

# ライセンス

modified new BSD License
http://opensource.org/licenses/BSD-3-Clause

# 著者

(C)2019 光成滋生 MITSUNARI Shigeo(herumi@nifty.com) All rights reserved.
本コンテンツの著作権、および本コンテンツ中に出てくる商標権、団体名、ロゴ、製品、
サービスなどはそれぞれ、各権利保有者に帰属します
