package mcl

/*
#cgo bn256 CFLAGS:-DMCLBN_FP_UNIT_SIZE=4
#cgo bn384 CFLAGS:-DMCLBN_FP_UNIT_SIZE=6
#cgo bn384_256 CFLAGS:-DMCLBN_FP_UNIT_SIZE=6 -DMCLBN_FR_UNIT_SIZE=4
#include <mcl/bn.h>
*/
import "C"
import "fmt"
import "unsafe"

// CurveFp254BNb -- 254 bit curve
const CurveFp254BNb = C.mclBn_CurveFp254BNb

// CurveFp382_1 -- 382 bit curve 1
const CurveFp382_1 = C.mclBn_CurveFp382_1

// CurveFp382_2 -- 382 bit curve 2
const CurveFp382_2 = C.mclBn_CurveFp382_2

// BLS12_381 --
const BLS12_381 = C.MCL_BLS12_381

// IoSerializeHexStr --
const IoSerializeHexStr = C.MCLBN_IO_SERIALIZE_HEX_STR

// IO_EC_AFFINE --
const IO_EC_AFFINE = C.MCLBN_IO_EC_AFFINE

// IO_EC_PROJ --
const IO_EC_PROJ = C.MCLBN_IO_EC_PROJ

// IRTF -- for SetMapToMode
const IRTF = 5 /* MCL_MAP_TO_MODE_HASH_TO_CURVE_07 */

// GetFrUnitSize --
func GetFrUnitSize() int {
	return int(C.MCLBN_FR_UNIT_SIZE)
}

// GetFpUnitSize --
// same as GetMaxOpUnitSize()
func GetFpUnitSize() int {
	return int(C.MCLBN_FP_UNIT_SIZE)
}

// GetMaxOpUnitSize --
func GetMaxOpUnitSize() int {
	return int(C.MCLBN_FP_UNIT_SIZE)
}

// GetOpUnitSize --
// the length of Fr is GetOpUnitSize() * 8 bytes
func GetOpUnitSize() int {
	return int(C.mclBn_getOpUnitSize())
}

// GetFrByteSize -- the serialized size of Fr
func GetFrByteSize() int {
	return int(C.mclBn_getFrByteSize())
}

// GetFpByteSize -- the serialized size of Fp
func GetFpByteSize() int {
	return int(C.mclBn_getFpByteSize())
}

// GetG1ByteSize -- the serialized size of G1
func GetG1ByteSize() int {
	return GetFpByteSize()
}

// GetG2ByteSize -- the serialized size of G2
func GetG2ByteSize() int {
	return GetFpByteSize() * 2
}

// allow zero length byte
func getPointer(msg []byte) unsafe.Pointer {
	if len(msg) == 0 {
		return nil
	}
	return unsafe.Pointer(&msg[0])
}

// GetCurveOrder --
// return the order of G1
func GetCurveOrder() string {
	buf := make([]byte, 1024)
	// #nosec
	n := C.mclBn_getCurveOrder((*C.char)(unsafe.Pointer(&buf[0])), C.size_t(len(buf)))
	if n == 0 {
		panic("implementation err. size of buf is small")
	}
	return string(buf[:n])
}

// GetFieldOrder --
// return the characteristic of the field where a curve is defined
func GetFieldOrder() string {
	buf := make([]byte, 1024)
	// #nosec
	n := C.mclBn_getFieldOrder((*C.char)(unsafe.Pointer(&buf[0])), C.size_t(len(buf)))
	if n == 0 {
		panic("implementation err. size of buf is small")
	}
	return string(buf[:n])
}

func bool2Cint(b bool) C.int {
	if b {
		return 1
	} else {
		return 0
	}
}

// VerifyOrderG1 -- verify order if SetString/Deserialize are called
func VerifyOrderG1(doVerify bool) {
	// #nosec
	C.mclBn_verifyOrderG1(bool2Cint(doVerify))
}

// VerifyOrderG2 -- verify order if SetString/Deserialize are called
func VerifyOrderG2(doVerify bool) {
	// #nosec
	C.mclBn_verifyOrderG2(bool2Cint(doVerify))
}

// SetETHserialization --
func SetETHserialization(enable bool) {
	// #nosec
	C.mclBn_setETHserialization(bool2Cint(enable))
}

// SetMapToMode --
func SetMapToMode(mode int) error {
	// #nosec
	err := C.mclBn_setMapToMode((C.int)(mode))
	if err != 0 {
		return fmt.Errorf("SetMapToMode mode=%d\n", mode)
	}
	return nil
}

// Fr --
type Fr struct {
	v C.mclBnFr
}

// getPointer --
func (x *Fr) getPointer() (p *C.mclBnFr) {
	// #nosec
	return (*C.mclBnFr)(unsafe.Pointer(x))
}

// Clear --
func (x *Fr) Clear() {
	// #nosec
	C.mclBnFr_clear(x.getPointer())
}

// SetInt64 --
func (x *Fr) SetInt64(v int64) {
	// #nosec
	C.mclBnFr_setInt(x.getPointer(), C.int64_t(v))
}

// SetString --
func (x *Fr) SetString(s string, base int) error {
	buf := []byte(s)
	// #nosec
	err := C.mclBnFr_setStr(x.getPointer(), (*C.char)(getPointer(buf)), C.size_t(len(buf)), C.int(base))
	if err != 0 {
		return fmt.Errorf("err mclBnFr_setStr %x", err)
	}
	return nil
}

// Deserialize --
func (x *Fr) Deserialize(buf []byte) error {
	// #nosec
	n := C.mclBnFr_deserialize(x.getPointer(), getPointer(buf), C.size_t(len(buf)))
	if n == 0 || int(n) != len(buf) {
		return fmt.Errorf("err mclBnFr_deserialize %x", buf)
	}
	return nil
}

// SetLittleEndian --
func (x *Fr) SetLittleEndian(buf []byte) error {
	// #nosec
	err := C.mclBnFr_setLittleEndian(x.getPointer(), getPointer(buf), C.size_t(len(buf)))
	if err != 0 {
		return fmt.Errorf("err mclBnFr_setLittleEndian %x", err)
	}
	return nil
}

// SetLittleEndianMod --
func (x *Fr) SetLittleEndianMod(buf []byte) error {
	// #nosec
	err := C.mclBnFr_setLittleEndianMod(x.getPointer(), getPointer(buf), C.size_t(len(buf)))
	if err != 0 {
		return fmt.Errorf("err mclBnFr_setLittleEndianMod %x", err)
	}
	return nil
}

// SetBigEndianMod --
func (x *Fr) SetBigEndianMod(buf []byte) error {
	// #nosec
	err := C.mclBnFr_setBigEndianMod(x.getPointer(), getPointer(buf), C.size_t(len(buf)))
	if err != 0 {
		return fmt.Errorf("err mclBnFr_setBigEndianMod %x", err)
	}
	return nil
}

// IsEqual --
func (x *Fr) IsEqual(rhs *Fr) bool {
	return C.mclBnFr_isEqual(x.getPointer(), rhs.getPointer()) == 1
}

// IsZero --
func (x *Fr) IsZero() bool {
	return C.mclBnFr_isZero(x.getPointer()) == 1
}

// IsValid --
func (x *Fr) IsValid() bool {
	return C.mclBnFr_isValid(x.getPointer()) == 1
}

// IsOne --
func (x *Fr) IsOne() bool {
	return C.mclBnFr_isOne(x.getPointer()) == 1
}

// IsOdd --
func (x *Fr) IsOdd() bool {
	return C.mclBnFr_isOdd(x.getPointer()) == 1
}

// IsNegative -- true if x >= (r + 1) / 2
func (x *Fr) IsNegative() bool {
	return C.mclBnFr_isNegative(x.getPointer()) == 1
}

// SetByCSPRNG --
func (x *Fr) SetByCSPRNG() {
	err := C.mclBnFr_setByCSPRNG(x.getPointer())
	if err != 0 {
		panic("err mclBnFr_setByCSPRNG")
	}
}

// SetHashOf --
func (x *Fr) SetHashOf(buf []byte) bool {
	// #nosec
	return C.mclBnFr_setHashOf(x.getPointer(), getPointer(buf), C.size_t(len(buf))) == 0
}

// GetString --
func (x *Fr) GetString(base int) string {
	buf := make([]byte, 2048)
	// #nosec
	n := C.mclBnFr_getStr((*C.char)(unsafe.Pointer(&buf[0])), C.size_t(len(buf)), x.getPointer(), C.int(base))
	if n == 0 {
		panic("err mclBnFr_getStr")
	}
	return string(buf[:n])
}

// Serialize --
func (x *Fr) Serialize() []byte {
	buf := make([]byte, 2048)
	// #nosec
	n := C.mclBnFr_serialize(unsafe.Pointer(&buf[0]), C.size_t(len(buf)), x.getPointer())
	if n == 0 {
		panic("err mclBnFr_serialize")
	}
	return buf[:n]
}

// FrNeg --
func FrNeg(out *Fr, x *Fr) {
	C.mclBnFr_neg(out.getPointer(), x.getPointer())
}

// FrInv --
func FrInv(out *Fr, x *Fr) {
	C.mclBnFr_inv(out.getPointer(), x.getPointer())
}

// FrSqr --
func FrSqr(out *Fr, x *Fr) {
	C.mclBnFr_sqr(out.getPointer(), x.getPointer())
}

// FrAdd --
func FrAdd(out *Fr, x *Fr, y *Fr) {
	C.mclBnFr_add(out.getPointer(), x.getPointer(), y.getPointer())
}

// FrSub --
func FrSub(out *Fr, x *Fr, y *Fr) {
	C.mclBnFr_sub(out.getPointer(), x.getPointer(), y.getPointer())
}

// FrMul --
func FrMul(out *Fr, x *Fr, y *Fr) {
	C.mclBnFr_mul(out.getPointer(), x.getPointer(), y.getPointer())
}

// FrDiv --
func FrDiv(out *Fr, x *Fr, y *Fr) {
	C.mclBnFr_div(out.getPointer(), x.getPointer(), y.getPointer())
}

// FrSquareRoot --
func FrSquareRoot(out *Fr, x *Fr) bool {
	return C.mclBnFr_squareRoot(out.getPointer(), x.getPointer()) == 0
}

// Fp --
type Fp struct {
	v C.mclBnFp
}

// getPointer --
func (x *Fp) getPointer() (p *C.mclBnFp) {
	// #nosec
	return (*C.mclBnFp)(unsafe.Pointer(x))
}

// Clear --
func (x *Fp) Clear() {
	// #nosec
	C.mclBnFp_clear(x.getPointer())
}

// SetInt64 --
func (x *Fp) SetInt64(v int64) {
	// #nosec
	C.mclBnFp_setInt(x.getPointer(), C.int64_t(v))
}

// SetString --
func (x *Fp) SetString(s string, base int) error {
	buf := []byte(s)
	// #nosec
	err := C.mclBnFp_setStr(x.getPointer(), (*C.char)(getPointer(buf)), C.size_t(len(buf)), C.int(base))
	if err != 0 {
		return fmt.Errorf("err mclBnFp_setStr %x", err)
	}
	return nil
}

// Deserialize --
func (x *Fp) Deserialize(buf []byte) error {
	// #nosec
	n := C.mclBnFp_deserialize(x.getPointer(), getPointer(buf), C.size_t(len(buf)))
	if n == 0 || int(n) != len(buf) {
		return fmt.Errorf("err mclBnFp_deserialize %x", buf)
	}
	return nil
}

// SetLittleEndian --
func (x *Fp) SetLittleEndian(buf []byte) error {
	// #nosec
	err := C.mclBnFp_setLittleEndian(x.getPointer(), getPointer(buf), C.size_t(len(buf)))
	if err != 0 {
		return fmt.Errorf("err mclBnFp_setLittleEndian %x", err)
	}
	return nil
}

// SetLittleEndianMod --
func (x *Fp) SetLittleEndianMod(buf []byte) error {
	// #nosec
	err := C.mclBnFp_setLittleEndianMod(x.getPointer(), getPointer(buf), C.size_t(len(buf)))
	if err != 0 {
		return fmt.Errorf("err mclBnFp_setLittleEndianMod %x", err)
	}
	return nil
}

// SetBigEndianMod --
func (x *Fp) SetBigEndianMod(buf []byte) error {
	// #nosec
	err := C.mclBnFp_setBigEndianMod(x.getPointer(), getPointer(buf), C.size_t(len(buf)))
	if err != 0 {
		return fmt.Errorf("err mclBnFp_setBigEndianMod %x", err)
	}
	return nil
}

// IsEqual --
func (x *Fp) IsEqual(rhs *Fp) bool {
	return C.mclBnFp_isEqual(x.getPointer(), rhs.getPointer()) == 1
}

// IsZero --
func (x *Fp) IsZero() bool {
	return C.mclBnFp_isZero(x.getPointer()) == 1
}

// IsValid --
func (x *Fp) IsValid() bool {
	return C.mclBnFp_isValid(x.getPointer()) == 1
}

// IsOne --
func (x *Fp) IsOne() bool {
	return C.mclBnFp_isOne(x.getPointer()) == 1
}

// IsOdd --
func (x *Fp) IsOdd() bool {
	return C.mclBnFp_isOdd(x.getPointer()) == 1
}

// IsNegative -- true if x >= (p + 1) / 2
func (x *Fp) IsNegative() bool {
	return C.mclBnFp_isNegative(x.getPointer()) == 1
}

// SetByCSPRNG --
func (x *Fp) SetByCSPRNG() {
	err := C.mclBnFp_setByCSPRNG(x.getPointer())
	if err != 0 {
		panic("err mclBnFp_setByCSPRNG")
	}
}

// SetHashOf --
func (x *Fp) SetHashOf(buf []byte) bool {
	// #nosec
	return C.mclBnFp_setHashOf(x.getPointer(), getPointer(buf), C.size_t(len(buf))) == 0
}

// GetString --
func (x *Fp) GetString(base int) string {
	buf := make([]byte, 2048)
	// #nosec
	n := C.mclBnFp_getStr((*C.char)(unsafe.Pointer(&buf[0])), C.size_t(len(buf)), x.getPointer(), C.int(base))
	if n == 0 {
		panic("err mclBnFp_getStr")
	}
	return string(buf[:n])
}

// Serialize --
func (x *Fp) Serialize() []byte {
	buf := make([]byte, 2048)
	// #nosec
	n := C.mclBnFp_serialize(unsafe.Pointer(&buf[0]), C.size_t(len(buf)), x.getPointer())
	if n == 0 {
		panic("err mclBnFp_serialize")
	}
	return buf[:n]
}

// FpNeg --
func FpNeg(out *Fp, x *Fp) {
	C.mclBnFp_neg(out.getPointer(), x.getPointer())
}

// FpInv --
func FpInv(out *Fp, x *Fp) {
	C.mclBnFp_inv(out.getPointer(), x.getPointer())
}

// FpSqr --
func FpSqr(out *Fp, x *Fp) {
	C.mclBnFp_sqr(out.getPointer(), x.getPointer())
}

// FpAdd --
func FpAdd(out *Fp, x *Fp, y *Fp) {
	C.mclBnFp_add(out.getPointer(), x.getPointer(), y.getPointer())
}

// FpSub --
func FpSub(out *Fp, x *Fp, y *Fp) {
	C.mclBnFp_sub(out.getPointer(), x.getPointer(), y.getPointer())
}

// FpMul --
func FpMul(out *Fp, x *Fp, y *Fp) {
	C.mclBnFp_mul(out.getPointer(), x.getPointer(), y.getPointer())
}

// FpDiv --
func FpDiv(out *Fp, x *Fp, y *Fp) {
	C.mclBnFp_div(out.getPointer(), x.getPointer(), y.getPointer())
}

// FpSquareRoot --
func FpSquareRoot(out *Fp, x *Fp) bool {
	return C.mclBnFp_squareRoot(out.getPointer(), x.getPointer()) == 0
}

// Fp2 -- x = D[0] + D[1] i where i^2 = -1
type Fp2 struct {
	D [2]Fp
}

// getPointer --
func (x *Fp2) getPointer() (p *C.mclBnFp2) {
	// #nosec
	return (*C.mclBnFp2)(unsafe.Pointer(x))
}

// Clear --
func (x *Fp2) Clear() {
	// #nosec
	C.mclBnFp2_clear(x.getPointer())
}

// Deserialize --
func (x *Fp2) Deserialize(buf []byte) error {
	// #nosec
	n := C.mclBnFp2_deserialize(x.getPointer(), getPointer(buf), C.size_t(len(buf)))
	if n == 0 || int(n) != len(buf) {
		return fmt.Errorf("err mclBnFp2_deserialize %x", buf)
	}
	return nil
}

// IsEqual --
func (x *Fp2) IsEqual(rhs *Fp2) bool {
	return C.mclBnFp2_isEqual(x.getPointer(), rhs.getPointer()) == 1
}

// IsZero --
func (x *Fp2) IsZero() bool {
	return C.mclBnFp2_isZero(x.getPointer()) == 1
}

// IsOne --
func (x *Fp2) IsOne() bool {
	return C.mclBnFp2_isOne(x.getPointer()) == 1
}

// Serialize --
func (x *Fp2) Serialize() []byte {
	buf := make([]byte, 2048)
	// #nosec
	n := C.mclBnFp2_serialize(unsafe.Pointer(&buf[0]), C.size_t(len(buf)), x.getPointer())
	if n == 0 {
		panic("err mclBnFp2_serialize")
	}
	return buf[:n]
}

// Fp2Neg --
func Fp2Neg(out *Fp2, x *Fp2) {
	C.mclBnFp2_neg(out.getPointer(), x.getPointer())
}

// Fp2Inv --
func Fp2Inv(out *Fp2, x *Fp2) {
	C.mclBnFp2_inv(out.getPointer(), x.getPointer())
}

// Fp2Sqr --
func Fp2Sqr(out *Fp2, x *Fp2) {
	C.mclBnFp2_sqr(out.getPointer(), x.getPointer())
}

// Fp2Add --
func Fp2Add(out *Fp2, x *Fp2, y *Fp2) {
	C.mclBnFp2_add(out.getPointer(), x.getPointer(), y.getPointer())
}

// Fp2Sub --
func Fp2Sub(out *Fp2, x *Fp2, y *Fp2) {
	C.mclBnFp2_sub(out.getPointer(), x.getPointer(), y.getPointer())
}

// Fp2Mul --
func Fp2Mul(out *Fp2, x *Fp2, y *Fp2) {
	C.mclBnFp2_mul(out.getPointer(), x.getPointer(), y.getPointer())
}

// Fp2Div --
func Fp2Div(out *Fp2, x *Fp2, y *Fp2) {
	C.mclBnFp2_div(out.getPointer(), x.getPointer(), y.getPointer())
}

// Fp2SquareRoot --
func Fp2SquareRoot(out *Fp2, x *Fp2) bool {
	return C.mclBnFp2_squareRoot(out.getPointer(), x.getPointer()) == 0
}

// G1 --
type G1 struct {
	X Fp
	Y Fp
	Z Fp
}

// getPointer --
func (x *G1) getPointer() (p *C.mclBnG1) {
	// #nosec
	return (*C.mclBnG1)(unsafe.Pointer(x))
}

// Clear --
func (x *G1) Clear() {
	// #nosec
	C.mclBnG1_clear(x.getPointer())
}

// SetString --
func (x *G1) SetString(s string, base int) error {
	buf := []byte(s)
	// #nosec
	err := C.mclBnG1_setStr(x.getPointer(), (*C.char)(getPointer(buf)), C.size_t(len(buf)), C.int(base))
	if err != 0 {
		return fmt.Errorf("err mclBnG1_setStr %x", err)
	}
	return nil
}

// Deserialize --
func (x *G1) Deserialize(buf []byte) error {
	// #nosec
	n := C.mclBnG1_deserialize(x.getPointer(), getPointer(buf), C.size_t(len(buf)))
	if n == 0 || int(n) != len(buf) {
		return fmt.Errorf("err mclBnG1_deserialize %x", buf)
	}
	return nil
}

const ZERO_HEADER = 1 << 6

func isZeroFormat(buf []byte, n int) bool {
	if len(buf) < n {
		return false
	}
	if buf[0] != ZERO_HEADER {
		return false
	}
	for i := 1; i < n; i++ {
		if buf[i] != 0 {
			return false
		}
	}
	return true
}

// DeserializeUncompressed -- x.Deserialize() + y.Deserialize()
func (x *G1) DeserializeUncompressed(buf []byte) error {
	if isZeroFormat(buf, GetG1ByteSize()*2) {
		x.Clear()
		return nil
	}
	// #nosec
	var n = C.mclBnFp_deserialize(x.X.getPointer(), getPointer(buf), C.size_t(len(buf)))
	if n == 0 {
		return fmt.Errorf("err UncompressedDeserialize X %x", buf)
	}
	buf = buf[n:]
	// #nosec
	n = C.mclBnFp_deserialize(x.Y.getPointer(), getPointer(buf), C.size_t(len(buf)))
	if n == 0 {
		return fmt.Errorf("err UncompressedDeserialize Y %x", buf)
	}
	x.Z.SetInt64(1)
	if !x.IsValid() {
		return fmt.Errorf("err invalid point")
	}
	return nil
}

// IsEqual --
func (x *G1) IsEqual(rhs *G1) bool {
	return C.mclBnG1_isEqual(x.getPointer(), rhs.getPointer()) == 1
}

// IsZero --
func (x *G1) IsZero() bool {
	return C.mclBnG1_isZero(x.getPointer()) == 1
}

// IsValid --
func (x *G1) IsValid() bool {
	return C.mclBnG1_isValid(x.getPointer()) == 1
}

// IsValidOrder --
func (x *G1) IsValidOrder() bool {
	return C.mclBnG1_isValidOrder(x.getPointer()) == 1
}

// HashAndMapTo --
func (x *G1) HashAndMapTo(buf []byte) error {
	// #nosec
	err := C.mclBnG1_hashAndMapTo(x.getPointer(), getPointer(buf), C.size_t(len(buf)))
	if err != 0 {
		return fmt.Errorf("err mclBnG1_hashAndMapTo %x", err)
	}
	return nil
}

// GetString --
func (x *G1) GetString(base int) string {
	buf := make([]byte, 2048)
	// #nosec
	n := C.mclBnG1_getStr((*C.char)(unsafe.Pointer(&buf[0])), C.size_t(len(buf)), x.getPointer(), C.int(base))
	if n == 0 {
		panic("err mclBnG1_getStr")
	}
	return string(buf[:n])
}

// Serialize --
func (x *G1) Serialize() []byte {
	buf := make([]byte, GetG1ByteSize())
	// #nosec
	n := C.mclBnG1_serialize(unsafe.Pointer(&buf[0]), C.size_t(len(buf)), x.getPointer())
	if n == 0 {
		panic("err mclBnG1_serialize")
	}
	return buf
}

// SerializeUncompressed -- all zero array if x.IsZero()
func (x *G1) SerializeUncompressed() []byte {
	buf := make([]byte, GetG1ByteSize()*2)
	if x.IsZero() {
		buf[0] = ZERO_HEADER
		return buf
	}
	var nx G1
	G1Normalize(&nx, x)
	// #nosec
	var n = C.mclBnFp_serialize(unsafe.Pointer(&buf[0]), C.size_t(len(buf)), nx.X.getPointer())
	if n == 0 {
		panic("err mclBnFp_serialize X")
	}
	n = C.mclBnFp_serialize(unsafe.Pointer(&buf[n]), C.size_t(len(buf))-n, nx.Y.getPointer())
	if n == 0 {
		panic("err mclBnFp_serialize Y")
	}
	return buf
}

// G1Normalize --
func G1Normalize(out *G1, x *G1) {
	C.mclBnG1_normalize(out.getPointer(), x.getPointer())
}

// G1Neg --
func G1Neg(out *G1, x *G1) {
	C.mclBnG1_neg(out.getPointer(), x.getPointer())
}

// G1Dbl --
func G1Dbl(out *G1, x *G1) {
	C.mclBnG1_dbl(out.getPointer(), x.getPointer())
}

// G1Add --
func G1Add(out *G1, x *G1, y *G1) {
	C.mclBnG1_add(out.getPointer(), x.getPointer(), y.getPointer())
}

// G1Sub --
func G1Sub(out *G1, x *G1, y *G1) {
	C.mclBnG1_sub(out.getPointer(), x.getPointer(), y.getPointer())
}

// G1Mul --
func G1Mul(out *G1, x *G1, y *Fr) {
	C.mclBnG1_mul(out.getPointer(), x.getPointer(), y.getPointer())
}

// G1MulVec -- multi scalar multiplication out = sum mul(xVec[i], yVec[i])
func G1MulVec(out *G1, xVec []G1, yVec []Fr) {
	n := len(xVec)
	if n != len(yVec) {
		panic("xVec and yVec have the same size")
	}
	if n == 0 {
		out.Clear()
		return
	}
	C.mclBnG1_mulVec(out.getPointer(), (*C.mclBnG1)(unsafe.Pointer(&xVec[0])), (*C.mclBnFr)(unsafe.Pointer(&yVec[0])), (C.size_t)(n))
}

// G1MulCT -- constant time (depending on bit lengh of y)
func G1MulCT(out *G1, x *G1, y *Fr) {
	C.mclBnG1_mulCT(out.getPointer(), x.getPointer(), y.getPointer())
}

// G2 --
type G2 struct {
	X Fp2
	Y Fp2
	Z Fp2
}

// getPointer --
func (x *G2) getPointer() (p *C.mclBnG2) {
	// #nosec
	return (*C.mclBnG2)(unsafe.Pointer(x))
}

// Clear --
func (x *G2) Clear() {
	// #nosec
	C.mclBnG2_clear(x.getPointer())
}

// SetString --
func (x *G2) SetString(s string, base int) error {
	buf := []byte(s)
	// #nosec
	err := C.mclBnG2_setStr(x.getPointer(), (*C.char)(getPointer(buf)), C.size_t(len(buf)), C.int(base))
	if err != 0 {
		return fmt.Errorf("err mclBnG2_setStr %x", err)
	}
	return nil
}

// Deserialize --
func (x *G2) Deserialize(buf []byte) error {
	// #nosec
	n := C.mclBnG2_deserialize(x.getPointer(), getPointer(buf), C.size_t(len(buf)))
	if n == 0 || int(n) != len(buf) {
		return fmt.Errorf("err mclBnG2_deserialize %x", buf)
	}
	return nil
}

// DeserializeUncompressed -- x.Deserialize() + y.Deserialize()
func (x *G2) DeserializeUncompressed(buf []byte) error {
	if isZeroFormat(buf, GetG2ByteSize()*2) {
		x.Clear()
		return nil
	}
	// #nosec
	var n = C.mclBnFp2_deserialize(x.X.getPointer(), getPointer(buf), C.size_t(len(buf)))
	if n == 0 {
		return fmt.Errorf("err UncompressedDeserialize X %x", buf)
	}
	buf = buf[n:]
	// #nosec
	n = C.mclBnFp2_deserialize(x.Y.getPointer(), getPointer(buf), C.size_t(len(buf)))
	if n == 0 {
		return fmt.Errorf("err UncompressedDeserialize Y %x", buf)
	}
	x.Z.D[0].SetInt64(1)
	x.Z.D[1].Clear()
	if !x.IsValid() {
		return fmt.Errorf("err invalid point")
	}
	return nil
}

// IsEqual --
func (x *G2) IsEqual(rhs *G2) bool {
	return C.mclBnG2_isEqual(x.getPointer(), rhs.getPointer()) == 1
}

// IsZero --
func (x *G2) IsZero() bool {
	return C.mclBnG2_isZero(x.getPointer()) == 1
}

// IsValid --
func (x *G2) IsValid() bool {
	return C.mclBnG2_isValid(x.getPointer()) == 1
}

// IsValidOrder --
func (x *G2) IsValidOrder() bool {
	return C.mclBnG2_isValidOrder(x.getPointer()) == 1
}

// HashAndMapTo --
func (x *G2) HashAndMapTo(buf []byte) error {
	// #nosec
	err := C.mclBnG2_hashAndMapTo(x.getPointer(), getPointer(buf), C.size_t(len(buf)))
	if err != 0 {
		return fmt.Errorf("err mclBnG2_hashAndMapTo %x", err)
	}
	return nil
}

// GetString --
func (x *G2) GetString(base int) string {
	buf := make([]byte, 2048)
	// #nosec
	n := C.mclBnG2_getStr((*C.char)(unsafe.Pointer(&buf[0])), C.size_t(len(buf)), x.getPointer(), C.int(base))
	if n == 0 {
		panic("err mclBnG2_getStr")
	}
	return string(buf[:n])
}

// Serialize --
func (x *G2) Serialize() []byte {
	buf := make([]byte, 2048)
	// #nosec
	n := C.mclBnG2_serialize(unsafe.Pointer(&buf[0]), C.size_t(len(buf)), x.getPointer())
	if n == 0 {
		panic("err mclBnG2_serialize")
	}
	return buf[:n]
}

// SerializeUncompressed -- all zero array if x.IsZero()
func (x *G2) SerializeUncompressed() []byte {
	buf := make([]byte, GetG2ByteSize()*2)
	if x.IsZero() {
		buf[0] = ZERO_HEADER
		return buf
	}
	var nx G2
	G2Normalize(&nx, x)
	// #nosec
	var n = C.mclBnFp2_serialize(unsafe.Pointer(&buf[0]), C.size_t(len(buf)), nx.X.getPointer())
	if n == 0 {
		panic("err mclBnFp2_serialize X")
	}
	n = C.mclBnFp2_serialize(unsafe.Pointer(&buf[n]), C.size_t(len(buf))-n, nx.Y.getPointer())
	if n == 0 {
		panic("err mclBnFp2_serialize Y")
	}
	return buf
}

// G2Normalize --
func G2Normalize(out *G2, x *G2) {
	C.mclBnG2_normalize(out.getPointer(), x.getPointer())
}

// G2Neg --
func G2Neg(out *G2, x *G2) {
	C.mclBnG2_neg(out.getPointer(), x.getPointer())
}

// G2Dbl --
func G2Dbl(out *G2, x *G2) {
	C.mclBnG2_dbl(out.getPointer(), x.getPointer())
}

// G2Add --
func G2Add(out *G2, x *G2, y *G2) {
	C.mclBnG2_add(out.getPointer(), x.getPointer(), y.getPointer())
}

// G2Sub --
func G2Sub(out *G2, x *G2, y *G2) {
	C.mclBnG2_sub(out.getPointer(), x.getPointer(), y.getPointer())
}

// G2Mul --
func G2Mul(out *G2, x *G2, y *Fr) {
	C.mclBnG2_mul(out.getPointer(), x.getPointer(), y.getPointer())
}

// G2MulVec -- multi scalar multiplication out = sum mul(xVec[i], yVec[i])
func G2MulVec(out *G2, xVec []G2, yVec []Fr) {
	n := len(xVec)
	if n != len(yVec) {
		panic("xVec and yVec have the same size")
	}
	if n == 0 {
		out.Clear()
		return
	}
	C.mclBnG2_mulVec(out.getPointer(), (*C.mclBnG2)(unsafe.Pointer(&xVec[0])), (*C.mclBnFr)(unsafe.Pointer(&yVec[0])), (C.size_t)(n))
}

// GT --
type GT struct {
	v C.mclBnGT
}

// getPointer --
func (x *GT) getPointer() (p *C.mclBnGT) {
	// #nosec
	return (*C.mclBnGT)(unsafe.Pointer(x))
}

// Clear --
func (x *GT) Clear() {
	// #nosec
	C.mclBnGT_clear(x.getPointer())
}

// SetInt64 --
func (x *GT) SetInt64(v int64) {
	// #nosec
	C.mclBnGT_setInt(x.getPointer(), C.int64_t(v))
}

// SetString --
func (x *GT) SetString(s string, base int) error {
	buf := []byte(s)
	// #nosec
	err := C.mclBnGT_setStr(x.getPointer(), (*C.char)(getPointer(buf)), C.size_t(len(buf)), C.int(base))
	if err != 0 {
		return fmt.Errorf("err mclBnGT_setStr %x", err)
	}
	return nil
}

// Deserialize --
func (x *GT) Deserialize(buf []byte) error {
	// #nosec
	n := C.mclBnGT_deserialize(x.getPointer(), getPointer(buf), C.size_t(len(buf)))
	if n == 0 || int(n) != len(buf) {
		return fmt.Errorf("err mclBnGT_deserialize %x", buf)
	}
	return nil
}

// IsEqual --
func (x *GT) IsEqual(rhs *GT) bool {
	return C.mclBnGT_isEqual(x.getPointer(), rhs.getPointer()) == 1
}

// IsZero --
func (x *GT) IsZero() bool {
	return C.mclBnGT_isZero(x.getPointer()) == 1
}

// IsOne --
func (x *GT) IsOne() bool {
	return C.mclBnGT_isOne(x.getPointer()) == 1
}

// GetString --
func (x *GT) GetString(base int) string {
	buf := make([]byte, 2048)
	// #nosec
	n := C.mclBnGT_getStr((*C.char)(unsafe.Pointer(&buf[0])), C.size_t(len(buf)), x.getPointer(), C.int(base))
	if n == 0 {
		panic("err mclBnGT_getStr")
	}
	return string(buf[:n])
}

// Serialize --
func (x *GT) Serialize() []byte {
	buf := make([]byte, 2048)
	// #nosec
	n := C.mclBnGT_serialize(unsafe.Pointer(&buf[0]), C.size_t(len(buf)), x.getPointer())
	if n == 0 {
		panic("err mclBnGT_serialize")
	}
	return buf[:n]
}

// GTNeg --
func GTNeg(out *GT, x *GT) {
	C.mclBnGT_neg(out.getPointer(), x.getPointer())
}

// GTInv --
func GTInv(out *GT, x *GT) {
	C.mclBnGT_inv(out.getPointer(), x.getPointer())
}

// GTAdd --
func GTAdd(out *GT, x *GT, y *GT) {
	C.mclBnGT_add(out.getPointer(), x.getPointer(), y.getPointer())
}

// GTSub --
func GTSub(out *GT, x *GT, y *GT) {
	C.mclBnGT_sub(out.getPointer(), x.getPointer(), y.getPointer())
}

// GTMul --
func GTMul(out *GT, x *GT, y *GT) {
	C.mclBnGT_mul(out.getPointer(), x.getPointer(), y.getPointer())
}

// GTDiv --
func GTDiv(out *GT, x *GT, y *GT) {
	C.mclBnGT_div(out.getPointer(), x.getPointer(), y.getPointer())
}

// GTPow --
func GTPow(out *GT, x *GT, y *Fr) {
	C.mclBnGT_pow(out.getPointer(), x.getPointer(), y.getPointer())
}

// MapToG1 --
func MapToG1(out *G1, x *Fp) error {
	if C.mclBnFp_mapToG1(out.getPointer(), x.getPointer()) != 0 {
		return fmt.Errorf("err mclBnFp_mapToG1")
	}
	return nil
}

// MapToG2 --
func MapToG2(out *G2, x *Fp2) error {
	if C.mclBnFp2_mapToG2(out.getPointer(), x.getPointer()) != 0 {
		return fmt.Errorf("err mclBnFp2_mapToG2")
	}
	return nil
}

// Pairing --
func Pairing(out *GT, x *G1, y *G2) {
	C.mclBn_pairing(out.getPointer(), x.getPointer(), y.getPointer())
}

// FinalExp --
func FinalExp(out *GT, x *GT) {
	C.mclBn_finalExp(out.getPointer(), x.getPointer())
}

// MillerLoop --
func MillerLoop(out *GT, x *G1, y *G2) {
	C.mclBn_millerLoop(out.getPointer(), x.getPointer(), y.getPointer())
}

// MillerLoopVec -- multi pairings ; out = prod_i e(xVec[i], yVec[i])
func MillerLoopVec(out *GT, xVec []G1, yVec []G2) {
	n := len(xVec)
	if n != len(yVec) {
		panic("xVec and yVec have the same size")
	}
	if n == 0 {
		out.SetInt64(1)
		return
	}
	C.mclBn_millerLoopVec(out.getPointer(), (*C.mclBnG1)(unsafe.Pointer(&xVec[0])), (*C.mclBnG2)(unsafe.Pointer(&yVec[0])), (C.size_t)(n))
}

// GetUint64NumToPrecompute --
func GetUint64NumToPrecompute() int {
	return int(C.mclBn_getUint64NumToPrecompute())
}

// PrecomputeG2 --
func PrecomputeG2(Qbuf []uint64, Q *G2) {
	// #nosec
	C.mclBn_precomputeG2((*C.uint64_t)(unsafe.Pointer(&Qbuf[0])), Q.getPointer())
}

// PrecomputedMillerLoop --
func PrecomputedMillerLoop(out *GT, P *G1, Qbuf []uint64) {
	// #nosec
	C.mclBn_precomputedMillerLoop(out.getPointer(), P.getPointer(), (*C.uint64_t)(unsafe.Pointer(&Qbuf[0])))
}

// PrecomputedMillerLoop2 --
func PrecomputedMillerLoop2(out *GT, P1 *G1, Q1buf []uint64, P2 *G1, Q2buf []uint64) {
	// #nosec
	C.mclBn_precomputedMillerLoop2(out.getPointer(), P1.getPointer(), (*C.uint64_t)(unsafe.Pointer(&Q1buf[0])), P1.getPointer(), (*C.uint64_t)(unsafe.Pointer(&Q1buf[0])))
}

// FrEvaluatePolynomial -- y = c[0] + c[1] * x + c[2] * x^2 + ...
func FrEvaluatePolynomial(y *Fr, c []Fr, x *Fr) error {
	n := len(c)
	if n == 0 {
		y.Clear()
		return nil
	}
	// #nosec
	err := C.mclBn_FrEvaluatePolynomial(y.getPointer(), (*C.mclBnFr)(unsafe.Pointer(&c[0])), (C.size_t)(n), x.getPointer())
	if err != 0 {
		return fmt.Errorf("err mclBn_FrEvaluatePolynomial")
	}
	return nil
}

// G1EvaluatePolynomial -- y = c[0] + c[1] * x + c[2] * x^2 + ...
func G1EvaluatePolynomial(y *G1, c []G1, x *Fr) error {
	n := len(c)
	if n == 0 {
		y.Clear()
		return nil
	}
	// #nosec
	err := C.mclBn_G1EvaluatePolynomial(y.getPointer(), (*C.mclBnG1)(unsafe.Pointer(&c[0])), (C.size_t)(n), x.getPointer())
	if err != 0 {
		return fmt.Errorf("err mclBn_G1EvaluatePolynomial")
	}
	return nil
}

// G2EvaluatePolynomial -- y = c[0] + c[1] * x + c[2] * x^2 + ...
func G2EvaluatePolynomial(y *G2, c []G2, x *Fr) error {
	n := len(c)
	if n == 0 {
		y.Clear()
		return nil
	}
	// #nosec
	err := C.mclBn_G2EvaluatePolynomial(y.getPointer(), (*C.mclBnG2)(unsafe.Pointer(&c[0])), (C.size_t)(n), x.getPointer())
	if err != 0 {
		return fmt.Errorf("err mclBn_G2EvaluatePolynomial")
	}
	return nil
}

// FrLagrangeInterpolation --
func FrLagrangeInterpolation(out *Fr, xVec []Fr, yVec []Fr) error {
	n := len(xVec)
	if n == 0 {
		return fmt.Errorf("err FrLagrangeInterpolation:n=0")
	}
	if n != len(yVec) {
		return fmt.Errorf("err FrLagrangeInterpolation:bad size")
	}
	// #nosec
	err := C.mclBn_FrLagrangeInterpolation(out.getPointer(), (*C.mclBnFr)(unsafe.Pointer(&xVec[0])), (*C.mclBnFr)(unsafe.Pointer(&yVec[0])), (C.size_t)(n))
	if err != 0 {
		return fmt.Errorf("err FrLagrangeInterpolation")
	}
	return nil
}

// G1LagrangeInterpolation --
func G1LagrangeInterpolation(out *G1, xVec []Fr, yVec []G1) error {
	n := len(xVec)
	if n == 0 {
		return fmt.Errorf("err G1LagrangeInterpolation:n=0")
	}
	if n != len(yVec) {
		return fmt.Errorf("err G1LagrangeInterpolation:bad size")
	}
	// #nosec
	err := C.mclBn_G1LagrangeInterpolation(out.getPointer(), (*C.mclBnFr)(unsafe.Pointer(&xVec[0])), (*C.mclBnG1)(unsafe.Pointer(&yVec[0])), (C.size_t)(n))
	if err != 0 {
		return fmt.Errorf("err G1LagrangeInterpolation")
	}
	return nil
}

// G2LagrangeInterpolation --
func G2LagrangeInterpolation(out *G2, xVec []Fr, yVec []G2) error {
	n := len(xVec)
	if n == 0 {
		return fmt.Errorf("err G2LagrangeInterpolation:n=0")
	}
	if n != len(yVec) {
		return fmt.Errorf("err G2LagrangeInterpolation:bad size")
	}
	// #nosec
	err := C.mclBn_G2LagrangeInterpolation(out.getPointer(), (*C.mclBnFr)(unsafe.Pointer(&xVec[0])), (*C.mclBnG2)(unsafe.Pointer(&yVec[0])), (C.size_t)(n))
	if err != 0 {
		return fmt.Errorf("err G2LagrangeInterpolation")
	}
	return nil
}
