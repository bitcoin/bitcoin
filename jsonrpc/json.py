_json = __import__('json')
loads = _json.loads
dumps = _json.dumps
if hasattr(_json, 'JSONEncodeException'):
	JSONEncodeException = _json.JSONEncodeException
	JSONDecodeException = _json.JSONDecodeException
else:
	JSONEncodeException = TypeError
	JSONDecodeException = ValueError
