import os, sys, subprocess

EXE='bin/bls_smpl.exe'

def init():
	subprocess.check_call([EXE, "init"])

def sign(m, i=0):
	subprocess.check_call([EXE, "sign", "-m", m, "-id", str(i)])

def verify(m, i=0):
	subprocess.check_call([EXE, "verify", "-m", m, "-id", str(i)])

def share(n, k):
	subprocess.check_call([EXE, "share", "-n", str(n), "-k", str(k)])

def recover(ids):
	cmd = [EXE, "recover", "-ids"]
	for i in ids:
		cmd.append(str(i))
	subprocess.check_call(cmd)

def main():
	m = "hello bls threshold signature"
	n = 10
	ids = [1, 5, 3, 7]
	k = len(ids)
	init()
	sign(m)
	verify(m)
	share(n, k)
	for i in ids:
		sign(m, i)
		verify(m, i)
	subprocess.check_call(["rm", "sample/sign.txt"])
	recover(ids)
	verify(m)

if __name__ == '__main__':
    main()
