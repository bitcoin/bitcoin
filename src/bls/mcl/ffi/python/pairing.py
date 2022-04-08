from ctypes import *
from ctypes.wintypes import LPWSTR, LPCSTR, LPVOID

g_lib = None

def BN256_init():
	global g_lib
	g_lib = cdll.LoadLibrary("../../bin/bn256.dll")
	ret = g_lib.BN256_init()
	if ret:
		print "ERR BN256_init"

class Fr(Structure):
	_fields_ = [("v", c_ulonglong * 4)]
	def setInt(self, v):
		g_lib.BN256_Fr_setInt(self.v, v)
	def setStr(self, s):
		ret = g_lib.BN256_Fr_setStr(self.v, c_char_p(s))
		if ret:
			print("ERR Fr:setStr")
	def __str__(self):
		svLen = 1024
		sv = create_string_buffer('\0' * svLen)
		ret = g_lib.BN256_Fr_getStr(sv, svLen, self.v)
		if ret:
			print("ERR Fr:getStr")
		return sv.value
	def isZero(self, rhs):
		return g_lib.BN256_Fr_isZero(self.v) != 0
	def isOne(self, rhs):
		return g_lib.BN256_Fr_isOne(self.v) != 0
	def __eq__(self, rhs):
		return g_lib.BN256_Fr_isEqual(self.v, rhs.v) != 0
	def __ne__(self, rhs):
		return not(P == Q)
	def __add__(self, rhs):
		ret = Fr()
		g_lib.BN256_Fr_add(ret.v, self.v, rhs.v)
		return ret
	def __sub__(self, rhs):
		ret = Fr()
		g_lib.BN256_Fr_sub(ret.v, self.v, rhs.v)
		return ret
	def __mul__(self, rhs):
		ret = Fr()
		g_lib.BN256_Fr_mul(ret.v, self.v, rhs.v)
		return ret
	def __div__(self, rhs):
		ret = Fr()
		g_lib.BN256_Fr_div(ret.v, self.v, rhs.v)
		return ret
	def __neg__(self):
		ret = Fr()
		g_lib.BN256_Fr_neg(ret.v, self.v)
		return ret

def Fr_add(z, x, y):
	g_lib.BN256_Fr_add(z.v, x.v, y.v)

def Fr_sub(z, x, y):
	g_lib.BN256_Fr_sub(z.v, x.v, y.v)

def Fr_mul(z, x, y):
	g_lib.BN256_Fr_mul(z.v, x.v, y.v)

def Fr_div(z, x, y):
	g_lib.BN256_Fr_div(z.v, x.v, y.v)

BN256_init()

P = Fr()
Q = Fr()
print P == Q
print P != Q
P.setInt(5)
Q.setStr("34982034824")
print Q
R = Fr()
Fr_add(R, P, Q)
print R
