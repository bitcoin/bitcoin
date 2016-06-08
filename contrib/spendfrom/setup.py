from distutils.core import setup
setup(name='CRWspendfrom',
      version='1.0',
      description='Command-line utility for crowncoin "coin control"',
      author='Gavin Andresen',
      author_email='gavin@crowncoinfoundation.org',
      requires=['jsonrpc'],
      scripts=['spendfrom.py'],
      )
