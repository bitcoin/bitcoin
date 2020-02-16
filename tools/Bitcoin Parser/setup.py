from setuptools import setup, find_packages
from blockchain_parser import __version__


setup(
    name='blockchain-parser',
    version=__version__,
    packages=find_packages(),
    url='https://github.com/alecalve/python-bitcoin-blockchain-parser',
    author='Antoine Le Calvez',
    author_email='antoine@p2sh.info',
    description='Bitcoin blockchain parser',
    test_suite='blockchain_parser.tests',
    classifiers=[
        'Development Status :: 5 - Production/Stable',
        'Environment :: Console',
        'Intended Audience :: Developers',
        'License :: OSI Approved :: GNU Lesser General Public License v3 (LGPLv3)',
        'Topic :: Software Development :: Libraries',
    ],
    install_requires=[
        'python-bitcoinlib==0.5.0',
        'plyvel==1.0.4'
    ]
)
