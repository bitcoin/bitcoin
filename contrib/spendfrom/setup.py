from distutils.core import setup
setup(name='sysspendfrom',
      version='1.0',
      description='Command-line utility for syscoin "coin control"',
      author='Gavin Andresen',
      author_email='gavin@syscoinfoundation.org',
      requires=['jsonrpc'],
      scripts=['spendfrom.py'],
      )
