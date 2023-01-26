
# return x*y
def mulPre(N):
	print('void mulPre(uint64_t *z, const uint32_t *x, const uint32_t *y)')
	print('{')
	print('	uint64_t t, L = 0, H = 0;')
	for i in range(N*2-1):
		for j in range(N+1):
			if j < N and 0 <= i-j < N:
				print(f'	t = x[{j}] *% y[{i-j}];')
				print(f'	L = L + (t & 0xffffffff);')
				print(f'	H = H + (t >> 32);')
		print(f'	z[{i}] = L;')
		print(f'	L := H; H := 0;')
	print(f'	z[{N*2-1}] = L;')
	print('}')

# return x*x
def sqrPre(N):
	print(f'inline void sqrPre(uint32_t y[{N*2}], const uint32_t x[{N}])')
	print('{')
	print('	uint64_t t;')
	print(f'	uint64_t b[{N*2-2}];')
	for i in range(1,N):
		op = '=' if i == 1 else '+='
		print()
		for step in range(0,N-i):
			print(f'	t = x[{step}]; t *= x[{step+i}];')
			print(f'	b[{step*2+i-1}] {op} t & 0xffffffff;')
			print(f'	b[{step*2+i}] {op} t >> 32;')

	print()
	print('	t = x[0]; t *= t;')
	print('	y[0] = uint32_t(t);')
	print('	uint64_t H = 0;')
	for i in range(1, N):
		print(f'	H = (H >> 32) + (t >> 32) + b[{i*2-2}] * 2;')
		print(f'	y[{i*2-1}] = uint32_t(H);')
		print(f'	t = x[{i}]; t *= t;')
		print(f'	H = (H >> 32) + (t & 0xffffffff) + b[{i*2-1}] * 2;')
		print(f'	y[{i*2}] = uint32_t(H);')
	print(f'	y[{N*2-1}] = (H >> 32) + (t >> 32);')
	print('}')


sqrPre(8)
