from scapy.all import *

victim = '10.0.2.7'
unprocessed_IPs = [
	'117.221.10.12',
	'176.245.224.161',
	'232.167.124.50',
	'251.57.3.85',
	'201.161.228.31',
	'30.80.139.85',
	'224.98.82.54',
	'96.40.225.15',
	'201.18.196.17',
	'99.109.76.160',
	'139.197.12.236',
	'30.66.151.31',
	'167.89.3.118',
	'110.252.118.238',
	'248.49.150.72'
]
pending_IPs = []
successful_IPs = []


def initialize_connection(ip):
	pass

def packet_received(packet):
	try:
		magic_number = packet.load[0:4]
		if magic_number != b'\xf9\xbe\xb4\xd9':
			return # Only allow Bitcoin messages
	except:
		return

	# Filter out sources that are not the victim
	#if packet[IP].src != victim:
	#	return
	# Filter out destinations that are not a spoofed IP
	#if not packet[IP].dst in unprocessed_IPs:
	#	return

	packet.show()
	#packet.show2()
	msgtype = packet.load[4:16].decode()
	#print(packet.dst)

	if msgtype == 'verack':
		# Successful connection! Move from pending to successful
		pass
	elif msgtype == 'ping':
		# send pong
		pass


	
	#print(dir(packet[IP]))



	"""

	'add_payload', 'add_underlayer', 'aliastypes', 'answers', 'build', 'build_done', 'build_padding', 'build_ps', 'canvas_dump', 'class_default_fields', 'class_default_fields_ref', 'class_dont_cache', 'class_fieldtype', 'class_packetfields', 'clear_cache', 'clone_with', 'command', 'convert_packet', 'convert_packets', 'convert_to', 'copy', 'copy_field_value', 'copy_fields_dict', 'decode_payload_as', 'default_fields', 'default_payload_class', 'delfieldval', 'deprecated_fields', 'direction', 'dispatch_hook', 'display', 'dissect', 'dissection_done', 'do_build', 'do_build_payload', 'do_build_ps', 'do_dissect', 'do_dissect_payload', 'do_init_cached_fields', 'do_init_fields', 'dst', 'explicit', 'extract_padding', 'fields', 'fields_desc', 'fieldtype', 'firstlayer', 'fragment', 'from_hexcap', 'get_field', 'getfield_and_val', 'getfieldval', 'getlayer', 'guess_payload_class', 'hashret', 'haslayer', 'hide_defaults', 'init_fields', 'iterpayloads', 'lastlayer', 'layers', 'lower_bonds', 'match_subclass', 'mysummary', 'name', 'original', 'overload_fields', 'overloaded_fields', 'packetfields',

	'payload', 'payload_guess', 'pdfdump', 'post_build', 'post_dissect', 'post_dissection', 'post_transforms', 'pre_dissect', 'prepare_cached_fields', 'psdump', 'raw_packet_cache', 'raw_packet_cache_fields', 'remove_payload', 'remove_underlayer', 'route', 'self_build', 'sent_time', 'setfieldval',

	'show', 'show2', 'show_indent', 'show_summary', 'sniffed_on', 'sprintf', 'src', 'summary', 'svgdump', 'time', 'type', 'underlayer', 'update_sent_time', 'upper_bonds', 'wirelen'

	"""

# Simeon Home
sniff(iface="wlp2s0",prn=packet_received, filter="tcp", store=1)

# Cybersecurity Lab
#sniff(iface="enp0s3",prn=packet_received, filter="tcp", store=1)


"""
pACKET

b' \x8d\xdf\x02G\xac\x89\x08\xfa\xe2\xcb\\P\x18\x01\x81t\xc1\x00\x00\xf9\xbe\xb4\xd9inv\x00\x00\x00\x00\x00\x00\x00\x00\x00\xed\x04\x00\x00\xb0\x9f{r#\x01\x00\x00\x006\xdf\xdb\xe8gG\xdd*r\x7fR\x8e\xfb\x12yu\xe7\xb2\xd8\xda\xd5\x18\xe7\x8eL/\xbc\x18\xf3\x0c\n\x1b\x01\x00\x00\x00?\x84Q\xd8\xd7CV\xdf\x93\xdf\xce:S^\xf1FE\xd2i\x1f>\xaf\x14\xbf\xe6m\xf9\x16\xde\xbarM\x01\x00\x00\x00t\xb7\x0f\x1e\x81\x84G\xfe\xbe\xb4\xede\xa3\xb1\xb2\xf7\xf2\xff\x1c\xeckE\xddz\x970x\x11\xda\xf8\x15\xda\x01\x00\x00\x00\xe8t\x9e\xc6fH\xee\xfb9\xc0f1\xec2\xf7\xe00\xe9]\xcb\x86`a\xd3G\xc7\x97\x8d\xb0\x98\xe0\x91\x01\x00\x00\x00\x00Y\xb5\x838\x7f>\r`:\xf4\x83\xa1\x9f` \x8a\xa6W\xfe\x0fD\xf26\x00\xd6`\x85\xfbG2%\x01\x00\x00\x00O/\xa4rT\xaaJ6\x80\xd6 \xd3\xc3\xf2\x93,\x90\xda\xdf\x029p\xcc\xf1\xff\x8c\x05\x89D\x7f.\xa1\x01\x00\x00\x00i\xc2wfO\xd11\x81\x9a\x88L\xca\x13\x0f\xfa\x89\x08?\xb1\x03y\xd1\xcfkWE\x12\x1d\xba\x175~\x01\x00\x00\x00\x8d5\x15|\xe9\x10\x8fs\xf71\xc9\xe7Ai\xb5\xf0FR\x91l,\xbf\xe3\xf0@\xa7\x82\xf1\xa0\x9fC`\x01\x00\x00\x00\xa8\xfb\x85\xe5Eo\x88\xd7\xcfL\xe4C\x0c$\xafg\xbb\x01R\x80\x13\xa1\xea\xd0\x051&&)\x10P]\x01\x00\x00\x00\xb9\xb7\xfd\xa0\xab\x04-d5\x8f\n\xf1\x07\xc3\\6fV@\xc6\xc7\xefF?\xf9(\x81S\xd4\xadU^\x01\x00\x00\x00\xa6\xb7I\xde\x92[a\\V\x01\x9d\x99m\xa3\xf8\xe7\x94I\x1f\xa0\xf5a\x08\xef\xcf\'B\xbe\x0f\xbf\xd4\x13\x01\x00\x00\x00\xf6-\xee\xc3/E\x99\xd2\xe4[\x80\xb3oB\x0b\xf9N\xb8\xc0\xd8\x07t\xe6\xbb\r\x89\x02)FJ\x04\xba\x01\x00\x00\x00\xf3\xd1\x8d\x9e\x07\x97ut\xff\xe9\x1fY"O\xce\xa9Ws\\\x8d\x91\x98\x9fJ\xc96,\x13\xf9\x9cm\xff\x01\x00\x00\x00\xf1Fo\xc4\x99\xce\xc2\xe3\xc4\xef\xaa\x91`\xa4\xd2R?\xfa\xe2F\x9173&\xed\xb8\xd9\x1f\x1e\x1c-%\x01\x00\x00\x00\xe5\x95\x9at\xc4\x1a\xb5p}\xc9\x9fT\x98/\x14\x192l\x04\x95\xf4\xaaR\xa2\t\xf71\xe96\x9d\xf7\xcd\x01\x00\x00\x00\xdfO5R\x97\x1a\xeb\xfa\xb1\xc7a\xd0\xd5b\x11\xf9\x15\xd9l\x0b\xd4\xf9\x91\xe8\xf0\xdf\xa2\xbe\xdd}\x82\xea\x01\x00\x00\x00\xdd\x12X\x87\xdc:&\xdc\xa1$u\x9cv\x96\xca\x0c\x9a\xab]\xcc\x82=R\x13ED\xc2\x83\xc3_\x97\xb1\x01\x00\x00\x00\x96\xed*c\x0f\x82[c@;`\x13\x02q4a\xb8B\x11\xabt2\x02\x97\xba\x83 J\x8c:\xef\x89\x01\x00\x00\x00\x95\x9ekIF\xcds\xe7U\x8c\xe2\xf8\x86\xc9\xc6wt\x1fB\xe9m\xb2Dm\x96\xef\xc7U\x94>\xe1\x95\x01\x00\x00\x00\x91.\x89\xc4\xe7\xa7/\xf1\x05\xd2\xb0b\xb1j]\xcb\xabXoL\xf0\x91K\xf53g\x9e \x0e\xb0\xcfn\x01\x00\x00\x00w\xe6\xf6Lo\xbb>G\x9e\xc1\xf2i\xc6\xcd^\xb3\xe2\xbb\x83%\xfe\xa9nn\x14{\x90x\xb9\xb9\xc4\xed\x01\x00\x00\x00]\xa9\x92z\xe0\xd7Y\x88\xdc\xcb*B#\x10uwr\x9e\x932\xa9\x13,v\xb8\x9f\xc0}\xd3*4\x17\x01\x00\x00\x00K\x13p\xdbv:\xd0\x12t5\xf8\x82\xb0{W\x0f\xf11(\xab\xc8\x12$F\x18lF/\x970\xca\\\x01\x00\x00\x00I\x8e\'\xf8\xa9\'\t\xf5}R\xdd\x9bZ\xacm\x0ee[\xa3pMj\xee6\xe0-\xd1\xef\x08\xdb\xc1\xce\x01\x00\x00\x00C\x8b\xf6\xbdE*AIN\x8f\x05\xaaqrt+\xc9\x18\xb3\x90\xaem \x1d>\x18(\xa9\xaf\x06k\xcd\x01\x00\x00\x00=W->\x82\xf5\x81\x99}m$\x06\x06^\xa1\xd7[o\xe4\xa8\xb4<I\x9e@\xb3\x1e\xe7\xbc\x14\xea[\x01\x00\x00\x00=N@\xc3qe\x0c\xc2uD\x7f\xb8\xee\xc2\xd5\xb2\x00\xee\x11\xb3q\x00\xcc>\xeeq\x9f\xe0\x91\xacM\n\x01\x00\x00\x008D\x99\x17zyZ{\xf0\xe3\xe1\xc9\xd5\x1d\xf3\x10g*s\x9a\x0bi\xf1\xa8hi\xa0)$\xc09\x9f\x01\x00\x00\x00\x1fp\x084;k\xb5t^\x14\xa2F\xd0\x14\x0eO\x17;rY\x1c\xb1\x1et\xbfC\x10`\xf2G\x94E\xe7\x01\x00\x00\x000\x12\xb2\x9a\xc5"W\x97\x86\xff\x0bR\x8c\xceM\xc7\xd9\xab\xbc\x96\x1d\xd1Bj\xf2\x8f[\x91\\\x9f\xceH\x01\x00\x00\x00\x8b\x05c\x8b\xe1b\x1c\x19\\\xcd\x16\xd5#\xbb\xc3i.\x0e\xf9i_|\xd0*\xd4zd\xc1Z\xf4\xf4L\x01\x00\x00\x00\xe5\xe8v\x7f\xc4\x9e\x8c\xca\xe3Scj\x87\x10@\xa8Q\x19M+\xed\xbbe\x9fQ\x8b\xd8\x9f\x16\x1f\n\\\x01\x00\x00\x00\x89\xa8:w-\xa3\x1e\xc4\xfe\x1c\xe8(\xdf\x93\x93\x9b\xa3\x01T\xc9\x8em\xaf\xa3\xccjj\xd8\xa8\xe0\xb2\xca\x01\x00\x00\x00\xe2r\xa5\x8a\x8bm\x08\x0fL"<\x82\xaf\xd2y\xa0NYx\x93jq\xf4\x80\x9e\xb4\x15\xcd\x8c\xdaU\x87\x01\x00\x00\x00\xe3CZi^\x84\xc6K\x87\xaf:=!$\x9f\xb4Z\x88K\xbeQ\xc0\x08I\x95\xc7\xa0\x96c\xa6\x1e_'
['__all_slots__', '__bool__', '__bytes__', '__class__', '__contains__', '__deepcopy__', '__delattr__', '__delitem__', '__dict__', '__dir__', '__div__', '__doc__', '__eq__', '__format__', '__ge__', '__getattr__', '__getattribute__', '__getitem__', '__getstate__', '__gt__', '__hash__', '__init__', '__init_subclass__', '__iter__', '__iterlen__', '__le__', '__len__', '__lt__', '__module__', '__mul__', '__ne__', '__new__', '__nonzero__', '__rdiv__', '__reduce__', '__reduce_ex__', '__repr__', '__rmul__', '__rtruediv__', '__setattr__', '__setitem__', '__setstate__', '__sizeof__', '__slots__', '__str__', '__subclasshook__', '__truediv__', '__weakref__', '_answered', '_defrag_pos', '_do_summary', '_name', '_overload_fields', '_pkt', '_resolve_alias', '_show_or_dump', '_superdir', '_tmp_dissect_pos', '_unpickle',

'add_payload', 'add_underlayer', 'aliastypes', 'answers', 'build', 'build_done', 'build_padding', 'build_ps', 'canvas_dump', 'class_default_fields', 'class_default_fields_ref', 'class_dont_cache', 'class_fieldtype', 'class_packetfields', 'clear_cache', 'clone_with', 'command', 'convert_packet', 'convert_packets', 'convert_to', 'copy', 'copy_field_value', 'copy_fields_dict', 'decode_payload_as', 'default_fields', 'default_payload_class', 'delfieldval', 'deprecated_fields', 'direction', 'dispatch_hook', 'display', 'dissect', 'dissection_done', 'do_build', 'do_build_payload', 'do_build_ps', 'do_dissect', 'do_dissect_payload', 'do_init_cached_fields', 'do_init_fields', 'dst', 'explicit', 'extract_padding', 'fields', 'fields_desc', 'fieldtype', 'firstlayer', 'fragment', 'from_hexcap', 'get_field', 'getfield_and_val', 'getfieldval', 'getlayer', 'guess_payload_class', 'hashret', 'haslayer', 'hide_defaults', 'init_fields', 'iterpayloads', 'lastlayer', 'layers', 'lower_bonds', 'match_subclass', 'mysummary', 'name', 'original', 'overload_fields', 'overloaded_fields', 'packetfields',

'payload', 'payload_guess', 'pdfdump', 'post_build', 'post_dissect', 'post_dissection', 'post_transforms', 'pre_dissect', 'prepare_cached_fields', 'psdump', 'raw_packet_cache', 'raw_packet_cache_fields', 'remove_payload', 'remove_underlayer', 'route', 'self_build', 'sent_time', 'setfieldval',

'show', 'show2', 'show_indent', 'show_summary', 'sniffed_on', 'sprintf', 'src', 'summary', 'svgdump', 'time', 'type', 'underlayer', 'update_sent_time', 'upper_bonds', 'wirelen']


"""