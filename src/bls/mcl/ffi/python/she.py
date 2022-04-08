import os
import sys
import platform
from ctypes import *
#from ctypes.util import find_library

# for init 2 Level HE
BN254 = 0
BLS12_381 = 5
# for initG1only (lifted ElGamal)
SECP192K1 = 100
SECP224K1 = 101
SECP256K1 = 102
NIST_P192 = 105
NIST_P224 = 106
NIST_P256 = 107

MCLBN_FR_UNIT_SIZE = 4
MCLBN_FP_UNIT_SIZE = 6

FR_SIZE = MCLBN_FR_UNIT_SIZE
G1_SIZE = MCLBN_FP_UNIT_SIZE * 3
G2_SIZE = MCLBN_FP_UNIT_SIZE * 6
GT_SIZE = MCLBN_FP_UNIT_SIZE * 12

SEC_SIZE = FR_SIZE * 2
PUB_SIZE = G1_SIZE + G2_SIZE
G1_CIPHER_SIZE = G1_SIZE * 2
G2_CIPHER_SIZE = G2_SIZE * 2
GT_CIPHER_SIZE = GT_SIZE * 4

MCLBN_COMPILED_TIME_VAR = (MCLBN_FR_UNIT_SIZE * 10) + MCLBN_FP_UNIT_SIZE

Buffer = c_ubyte * 2304
lib = None

def _init(curveType=BN254, G1only=False):
	global lib
	name = platform.system()
	if name == 'Linux':
		libName = 'libmclshe384_256.so'
	elif name == 'Darwin':
		libName = 'libmclshe384_256.dylib'
	elif name == 'Windows':
		libName = 'mclshe384_256.dll'
	else:
		raise RuntimeError("not support yet", name)
#	lib = cdll.LoadLibrary(find_library(libName))
	lib = cdll.LoadLibrary(libName)
	if G1only:
		ret = lib.sheInitG1only(curveType, MCLBN_COMPILED_TIME_VAR)
	else:
		ret = lib.sheInit(curveType, MCLBN_COMPILED_TIME_VAR)
	if ret != 0:
		raise RuntimeError("sheInit", ret)
	lib.mclBn_verifyOrderG1(0)
	lib.mclBn_verifyOrderG2(0)
	# custom setup for a function which returns pointer
	lib.shePrecomputedPublicKeyCreate.restype = c_void_p

def init(curveType=BN254):
	_init(curveType, False)

def initG1only(curveType=SECP256K1):
	_init(curveType, True)

def setRangeForDLP(hashSize):
	ret = lib.sheSetRangeForDLP(hashSize)
	if ret != 0:
		raise RuntimeError("setRangeForDLP", ret)

def setTryNum(tryNum):
	lib.sheSetTryNum(tryNum)

def _hexStr(v):
	s = ""
	for x in v:
		s += format(x, '02x')
	return s

def _serialize(self, f):
	buf = Buffer()
	ret = f(byref(buf), len(buf), byref(self.v))
	if ret == 0:
		raise RuntimeError("serialize")
	return buf[0:ret]

def _deserialize(cstr, f, buf):
	x = cstr()
	ca = (c_ubyte * len(buf))(*buf)
	ret = f(byref(x.v), byref(ca), len(buf))
	if ret == 0:
		raise RuntimeError("deserialize")
	return x

class CipherTextG1(Structure):
	_fields_ = [("v", c_ulonglong * G1_CIPHER_SIZE)]
	def serialize(self):
		return _serialize(self, lib.sheCipherTextG1Serialize)
	def serializeToHexStr(self):
		return _hexStr(self.serialize())

class CipherTextG2(Structure):
	_fields_ = [("v", c_ulonglong * G2_CIPHER_SIZE)]
	def serialize(self):
		return _serialize(self, lib.sheCipherTextG2Serialize)
	def serializeToHexStr(self):
		return _hexStr(self.serialize())

class CipherTextGT(Structure):
	_fields_ = [("v", c_ulonglong * GT_CIPHER_SIZE)]
	def serialize(self):
		return _serialize(self, lib.sheCipherTextGTSerialize)
	def serializeToHexStr(self):
		return _hexStr(self.serialize())

def _enc(CT, enc, encIntVec, neg, p, m):
	c = CT()
	if -0x80000000 <= m <= 0x7fffffff:
		ret = enc(byref(c.v), p, m)
		if ret != 0:
			raise RuntimeError("enc", m)
		return c
	if m < 0:
		minus = True
		m = -m
	else:
		minus = False
	if m >= 1 << (MCLBN_FR_UNIT_SIZE * 64):
		raise RuntimeError("enc:too large m", m)
	a = []
	while m > 0:
		a.append(m & 0xffffffff)
		m >>= 32
	ca = (c_uint * len(a))(*a)
	ret = encIntVec(byref(c.v), p, byref(ca), sizeof(ca))
	if ret != 0:
		raise RuntimeError("enc:IntVec", m)
	if minus:
		ret = neg(byref(c.v), byref(c.v))
		if ret != 0:
			raise RuntimeError("enc:neg", m)
	return c

class PrecomputedPublicKey(Structure):
	def __init__(self):
		self.p = 0
	def create(self):
		if not self.p:
			self.p = c_void_p(lib.shePrecomputedPublicKeyCreate())
			if self.p == 0:
				raise RuntimeError("PrecomputedPublicKey::create")
	def destroy(self):
		lib.shePrecomputedPublicKeyDestroy(self.p)
	def encG1(self, m):
		return _enc(CipherTextG1, lib.shePrecomputedPublicKeyEncG1, lib.shePrecomputedPublicKeyEncIntVecG1, lib.sheNegG1, self.p, m)
	def encG2(self, m):
		return _enc(CipherTextG2, lib.shePrecomputedPublicKeyEncG2, lib.shePrecomputedPublicKeyEncIntVecG2, lib.sheNegG2, self.p, m)
	def encGT(self, m):
		return _enc(CipherTextGT, lib.shePrecomputedPublicKeyEncGT, lib.shePrecomputedPublicKeyEncIntVecGT, lib.sheNegGT, self.p, m)

class PublicKey(Structure):
	_fields_ = [("v", c_ulonglong * PUB_SIZE)]
	def serialize(self):
		return _serialize(self, lib.shePublicKeySerialize)
	def serializeToHexStr(self):
		return _hexStr(self.serialize())
	def encG1(self, m):
		return _enc(CipherTextG1, lib.sheEncG1, lib.sheEncIntVecG1, lib.sheNegG1, byref(self.v), m)
	def encG2(self, m):
		return _enc(CipherTextG2, lib.sheEncG2, lib.sheEncIntVecG2, lib.sheNegG2, byref(self.v), m)
	def encGT(self, m):
		return _enc(CipherTextGT, lib.sheEncGT, lib.sheEncIntVecGT, lib.sheNegGT, byref(self.v), m)
	def createPrecomputedPublicKey(self):
		ppub = PrecomputedPublicKey()
		ppub.create()
		ret = lib.shePrecomputedPublicKeyInit(ppub.p, byref(self.v))
		if ret != 0:
			raise RuntimeError("createPrecomputedPublicKey")
		return ppub

class SecretKey(Structure):
	_fields_ = [("v", c_ulonglong * SEC_SIZE)]
	def setByCSPRNG(self):
		ret = lib.sheSecretKeySetByCSPRNG(byref(self.v))
		if ret != 0:
			raise RuntimeError("setByCSPRNG", ret)
	def serialize(self):
		return _serialize(self, lib.sheSecretKeySerialize)
	def serializeToHexStr(self):
		return _hexStr(self.serialize())
	def getPulicKey(self):
		pub = PublicKey()
		lib.sheGetPublicKey(byref(pub.v), byref(self.v))
		return pub
	def dec(self, c):
		m = c_longlong()
		if isinstance(c, CipherTextG1):
			ret = lib.sheDecG1(byref(m), byref(self.v), byref(c.v))
		elif isinstance(c, CipherTextG2):
			ret = lib.sheDecG2(byref(m), byref(self.v), byref(c.v))
		elif isinstance(c, CipherTextGT):
			ret = lib.sheDecGT(byref(m), byref(self.v), byref(c.v))
		if ret != 0:
			raise RuntimeError("dec")
		return m.value
	def isZero(self, c):
		if isinstance(c, CipherTextG1):
			return lib.sheIsZeroG1(byref(self.v), byref(c.v)) == 1
		elif isinstance(c, CipherTextG2):
			return lib.sheIsZeroG2(byref(self.v), byref(c.v)) == 1
		elif isinstance(c, CipherTextGT):
			return lib.sheIsZeroGT(byref(self.v), byref(c.v)) == 1
		raise RuntimeError("dec")

def neg(c):
	ret = -1
	if isinstance(c, CipherTextG1):
		out = CipherTextG1()
		ret = lib.sheNegG1(byref(out.v), byref(c.v))
	elif isinstance(c, CipherTextG2):
		out = CipherTextG2()
		ret = lib.sheNegG2(byref(out.v), byref(c.v))
	elif isinstance(c, CipherTextGT):
		out = CipherTextGT()
		ret = lib.sheNegGT(byref(out.v), byref(c.v))
	if ret != 0:
		raise RuntimeError("neg")
	return out

def add(cx, cy):
	ret = -1
	if isinstance(cx, CipherTextG1) and isinstance(cy, CipherTextG1):
		out = CipherTextG1()
		ret = lib.sheAddG1(byref(out.v), byref(cx.v), byref(cy.v))
	elif isinstance(cx, CipherTextG2) and isinstance(cy, CipherTextG2):
		out = CipherTextG2()
		ret = lib.sheAddG2(byref(out.v), byref(cx.v), byref(cy.v))
	elif isinstance(cx, CipherTextGT) and isinstance(cy, CipherTextGT):
		out = CipherTextGT()
		ret = lib.sheAddGT(byref(out.v), byref(cx.v), byref(cy.v))
	if ret != 0:
		raise RuntimeError("add")
	return out

def sub(cx, cy):
	ret = -1
	if isinstance(cx, CipherTextG1) and isinstance(cy, CipherTextG1):
		out = CipherTextG1()
		ret = lib.sheSubG1(byref(out.v), byref(cx.v), byref(cy.v))
	elif isinstance(cx, CipherTextG2) and isinstance(cy, CipherTextG2):
		out = CipherTextG2()
		ret = lib.sheSubG2(byref(out.v), byref(cx.v), byref(cy.v))
	elif isinstance(cx, CipherTextGT) and isinstance(cy, CipherTextGT):
		out = CipherTextGT()
		ret = lib.sheSubGT(byref(out.v), byref(cx.v), byref(cy.v))
	if ret != 0:
		raise RuntimeError("sub")
	return out

def mul(cx, cy):
	ret = -1
	if isinstance(cx, CipherTextG1) and isinstance(cy, CipherTextG2):
		out = CipherTextGT()
		ret = lib.sheMul(byref(out.v), byref(cx.v), byref(cy.v))
	elif isinstance(cx, CipherTextG1) and (isinstance(cy, int) or isinstance(cy, long)):
		return _enc(CipherTextG1, lib.sheMulG1, lib.sheMulIntVecG1, lib.sheNegG1, byref(cx.v), cy)
	elif isinstance(cx, CipherTextG2) and (isinstance(cy, int) or isinstance(cy, long)):
		return _enc(CipherTextG2, lib.sheMulG2, lib.sheMulIntVecG2, lib.sheNegG2, byref(cx.v), cy)
	elif isinstance(cx, CipherTextGT) and (isinstance(cy, int) or isinstance(cy, long)):
		return _enc(CipherTextGT, lib.sheMulGT, lib.sheMulIntVecGT, lib.sheNegGT, byref(cx.v), cy)
	if ret != 0:
		raise RuntimeError("mul")
	return out

def deserializeToSecretKey(buf):
	return _deserialize(SecretKey, lib.sheSecretKeyDeserialize, buf)

def deserializeToPublicKey(buf):
	return _deserialize(PublicKey, lib.shePublicKeyDeserialize, buf)

def deserializeToCipherTextG1(buf):
	return _deserialize(CipherTextG1, lib.sheCipherTextG1Deserialize, buf)

def deserializeToCipherTextG2(buf):
	return _deserialize(CipherTextG2, lib.sheCipherTextG2Deserialize, buf)

def deserializeToCipherTextGT(buf):
	return _deserialize(CipherTextGT, lib.sheCipherTextGTDeserialize, buf)

if __name__ == '__main__':
	if len(sys.argv) > 1:
		if not sys.argv[1] == 'g1only':
			print("err bad option")
			sys.exit(1)
		initG1only(SECP256K1)
		sec = SecretKey()
		sec.setByCSPRNG()
		print("sec=", sec.serializeToHexStr())
		if sec.serialize() != deserializeToSecretKey(sec.serialize()).serialize(): print("err-ser1")
		pub = sec.getPulicKey()
		print("pub=", pub.serializeToHexStr())
		if pub.serialize() != deserializeToPublicKey(pub.serialize()).serialize(): print("err-ser2")
		ppub = pub.createPrecomputedPublicKey()
		m1 = 123
		m2 = 256
		c1 = ppub.encG1(m1)
		c2 = ppub.encG1(m2)
		if sec.dec(c1) != m1: print("err1")
		if sec.dec(c2) != m2: print("err2")
		if sec.dec(add(c1, c2)) != m1 + m2: print("err3")
		if sec.dec(mul(c1, 4)) != m1 * 4: print("err4")
		if c1.serialize() != deserializeToCipherTextG1(c1.serialize()).serialize(): print("err-ser3")
		ppub.destroy() # necessary to avoid memory leak
		sys.exit(0)


	init(BLS12_381)
	sec = SecretKey()
	sec.setByCSPRNG()
	print("sec=", sec.serializeToHexStr())
	pub = sec.getPulicKey()
	print("pub=", pub.serializeToHexStr())
	if sec.serialize() != deserializeToSecretKey(sec.serialize()).serialize(): print("err-ser1")
	if pub.serialize() != deserializeToPublicKey(pub.serialize()).serialize(): print("err-ser2")

	m11 = 1
	m12 = 5
	m21 = 3
	m22 = -4
	c11 = pub.encG1(m11)
	c12 = pub.encG1(m12)
	# dec(enc) for G1
	if sec.dec(c11) != m11: print("err1")

	# add/sub for G1
	if sec.dec(add(c11, c12)) != m11 + m12: print("err2")
	if sec.dec(sub(c11, c12)) != m11 - m12: print("err3")

	# add/sub for G2
	c21 = pub.encG2(m21)
	c22 = pub.encG2(m22)
	if sec.dec(c21) != m21: print("err4")
	if sec.dec(add(c21, c22)) != m21 + m22: print("err5")
	if sec.dec(sub(c21, c22)) != m21 - m22: print("err6")

	# mul const for G1/G2
	if sec.dec(mul(c11, 3)) != m11 * 3: print("err_mul1")
	if sec.dec(mul(c21, 7)) != m21 * 7: print("err_mul2")

	if c11.serialize() != deserializeToCipherTextG1(c11.serialize()).serialize(): print("err-ser3")
	if c21.serialize() != deserializeToCipherTextG2(c21.serialize()).serialize(): print("err-ser3")

	# large integer
	m1 = 0x140712384712047127412964192876419276341
	m2 = -m1 + 123
	c1 = pub.encG1(m1)
	c2 = pub.encG1(m2)
	if sec.dec(add(c1, c2)) != 123: print("err-large11")
	c1 = mul(pub.encG1(1), m1)
	if sec.dec(add(c1, c2)) != 123: print("err-large12")

	c1 = pub.encG2(m1)
	c2 = pub.encG2(m2)
	if sec.dec(add(c1, c2)) != 123: print("err-large21")
	c1 = mul(pub.encG2(1), m1)
	if sec.dec(add(c1, c2)) != 123: print("err-large22")

	c1 = pub.encGT(m1)
	c2 = pub.encGT(m2)
	if sec.dec(add(c1, c2)) != 123: print("err-large31")
	c1 = mul(pub.encGT(1), m1)
	if sec.dec(add(c1, c2)) != 123: print("err-large32")
	if c1.serialize() != deserializeToCipherTextGT(c1.serialize()).serialize(): print("err-ser4")

	mt = -56
	ct = pub.encGT(mt)
	if sec.dec(ct) != mt: print("err7")

	# mul G1 and G2
	if sec.dec(mul(c11, c21)) != m11 * m21: print("err8")

	if not sec.isZero(pub.encG1(0)): print("err-zero11")
	if sec.isZero(pub.encG1(3)): print("err-zero12")
	if not sec.isZero(pub.encG2(0)): print("err-zero21")
	if sec.isZero(pub.encG2(3)): print("err-zero22")
	if not sec.isZero(pub.encGT(0)): print("err-zero31")
	if sec.isZero(pub.encGT(3)): print("err-zero32")

	# use precomputedPublicKey for performance
	ppub = pub.createPrecomputedPublicKey()
	c1 = ppub.encG1(m11)
	if sec.dec(c1) != m11: print("err9")

	# large integer for precomputedPublicKey
	m1 = 0x140712384712047127412964192876419276341
	m2 = -m1 + 123
	c1 = ppub.encG1(m1)
	c2 = ppub.encG1(m2)
	if sec.dec(add(c1, c2)) != 123: print("err10")
	c1 = ppub.encG2(m1)
	c2 = ppub.encG2(m2)
	if sec.dec(add(c1, c2)) != 123: print("err11")
	c1 = ppub.encGT(m1)
	c2 = ppub.encGT(m2)
	if sec.dec(add(c1, c2)) != 123: print("err12")

	import sys
	if sys.version_info.major >= 3:
		import timeit
		N = 100000
		print(str(timeit.timeit("pub.encG1(12)", number=N, globals=globals()) / float(N) * 1e3) + "msec")
		print(str(timeit.timeit("ppub.encG1(12)", number=N, globals=globals()) / float(N) * 1e3) + "msec")

	ppub.destroy() # necessary to avoid memory leak

