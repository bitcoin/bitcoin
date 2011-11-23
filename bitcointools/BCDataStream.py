#
# Workalike python implementation of Bitcoin's CDataStream class.
#
import struct
import StringIO
import mmap

class SerializationError(Exception):
  """ Thrown when there's a problem deserializing or serializing """

class BCDataStream(object):
  def __init__(self):
    self.input = None
    self.read_cursor = 0

  def clear(self):
    self.input = None
    self.read_cursor = 0

  def write(self, bytes):  # Initialize with string of bytes
    if self.input is None:
      self.input = bytes
    else:
      self.input += bytes

  def map_file(self, file, start):  # Initialize with bytes from file
    self.input = mmap.mmap(file.fileno(), 0, access=mmap.ACCESS_READ)
    self.read_cursor = start
  def seek_file(self, position):
    self.read_cursor = position
  def close_file(self):
    self.input.close()

  def read_string(self):
    # Strings are encoded depending on length:
    # 0 to 252 :  1-byte-length followed by bytes (if any)
    # 253 to 65,535 : byte'253' 2-byte-length followed by bytes
    # 65,536 to 4,294,967,295 : byte '254' 4-byte-length followed by bytes
    # ... and the Bitcoin client is coded to understand:
    # greater than 4,294,967,295 : byte '255' 8-byte-length followed by bytes of string
    # ... but I don't think it actually handles any strings that big.
    if self.input is None:
      raise SerializationError("call write(bytes) before trying to deserialize")

    try:
      length = self.read_compact_size()
    except IndexError:
      raise SerializationError("attempt to read past end of buffer")

    return self.read_bytes(length)

  def write_string(self, string):
    # Length-encoded as with read-string
    self.write_compact_size(len(string))
    self.write(string)

  def read_bytes(self, length):
    try:
      result = self.input[self.read_cursor:self.read_cursor+length]
      self.read_cursor += length
      return result
    except IndexError:
      raise SerializationError("attempt to read past end of buffer")

    return ''

  def read_boolean(self): return self.read_bytes(1)[0] != chr(0)
  def read_int16(self): return self._read_num('<h')
  def read_uint16(self): return self._read_num('<H')
  def read_int32(self): return self._read_num('<i')
  def read_uint32(self): return self._read_num('<I')
  def read_int64(self): return self._read_num('<q')
  def read_uint64(self): return self._read_num('<Q')

  def write_boolean(self, val): return self.write(chr(1) if val else chr(0))
  def write_int16(self, val): return self._write_num('<h', val)
  def write_uint16(self, val): return self._write_num('<H', val)
  def write_int32(self, val): return self._write_num('<i', val)
  def write_uint32(self, val): return self._write_num('<I', val)
  def write_int64(self, val): return self._write_num('<q', val)
  def write_uint64(self, val): return self._write_num('<Q', val)

  def read_compact_size(self):
    size = ord(self.input[self.read_cursor])
    self.read_cursor += 1
    if size == 253:
      size = self._read_num('<H')
    elif size == 254:
      size = self._read_num('<I')
    elif size == 255:
      size = self._read_num('<Q')
    return size

  def write_compact_size(self, size):
    if size < 0:
      raise SerializationError("attempt to write size < 0")
    elif size < 253:
       self.write(chr(size))
    elif size < 2**16:
      self.write('\xfd')
      self._write_num('<H', size)
    elif size < 2**32:
      self.write('\xfe')
      self._write_num('<I', size)
    elif size < 2**64:
      self.write('\xff')
      self._write_num('<Q', size)

  def _read_num(self, format):
    (i,) = struct.unpack_from(format, self.input, self.read_cursor)
    self.read_cursor += struct.calcsize(format)
    return i

  def _write_num(self, format, num):
    s = struct.pack(format, num)
    self.write(s)
