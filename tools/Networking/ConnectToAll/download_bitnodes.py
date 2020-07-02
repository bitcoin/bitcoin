import urllib.request

opener = urllib.request.build_opener()
opener.addheaders = [('User-agent', 'Mozilla/5.0')]
urllib.request.install_opener(opener)
urllib.request.urlretrieve('https://bitnodes.io/api/v1/snapshots/latest', 'Bitnodes.json')
print('Done. File updated!')