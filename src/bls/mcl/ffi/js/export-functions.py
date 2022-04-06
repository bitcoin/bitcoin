import sys, re, argparse

#RE_PROTOTYPE = re.compile(r'MCLBN_DLL_API\s\w\s\w\([^)]*\);')
RE_PROTOTYPE = re.compile(r'\w*\s(\w*)\s(\w*)\(([^)]*)\);')
def export_functions(args, fileNames, reToAddUnderscore):
	modName = args.js
	json = args.json
	if not reToAddUnderscore:
		reToAddUnderscore = r'(mclBn_init|setStr|getStr|[sS]erialize|setLittleEndian|setHashOf|hashAndMapTo|DecStr|HexStr|HashTo|blsSign|blsVerify|GetCurveOrder|GetFieldOrder|KeyShare|KeyRecover|blsSignatureRecover|blsInit)'
	reSpecialFunctionName = re.compile(reToAddUnderscore)
	if json:
		print '['
	elif modName:
		print 'function define_exported_' + modName + '(mod) {'
	comma = ''
	for fileName in fileNames:
		with open(fileName, 'rb') as f:
			for line in f.readlines():
				p = RE_PROTOTYPE.search(line)
				if p:
					ret = p.group(1)
					name = p.group(2)
					arg = p.group(3)
					if json or modName:
						retType = 'null' if ret == 'void' else 'number'
						if arg == '' or arg == 'void':
							paramNum = 0
						else:
							paramNum = len(arg.split(','))
						if reSpecialFunctionName.search(name):
							exportName = '_' + name # to wrap function
						else:
							exportName = name
						if json:
							print comma + '{'
							if comma == '':
								comma = ','
							print '  "name":"{0}",'.format(name)
							print '  "exportName":"{0}",'.format(exportName)
							print '  "ret":"{0}",'.format(retType)
							print '  "args":[',
							if paramNum > 0:
								print '"number"' + (', "number"' * (paramNum - 1)),
							print ']'
							print '}'
						else:
							paramType = '[' + ("'number', " * paramNum) + ']'
							print "{0} = mod.cwrap('{1}', '{2}', {3})".format(exportName, name, retType, paramType)
					else:
						print comma + "'_" + name + "'",
						if comma == '':
							comma = ','
	if json:
		print ']'
	elif modName:
		print '}'

def main():
	p = argparse.ArgumentParser('export_functions')
	p.add_argument('header', type=str, nargs='+', help='headers')
	p.add_argument('-js', type=str, nargs='?', help='module name')
	p.add_argument('-re', type=str, nargs='?', help='regular expression file to add underscore to function name')
	p.add_argument('-json', action='store_true', help='output json')
	args = p.parse_args()

	reToAddUnderscore = ''
	if args.re:
		reToAddUnderscore = open(args.re).read().strip()
	export_functions(args, args.header, reToAddUnderscore)

if __name__ == '__main__':
    main()

