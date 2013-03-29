local zmq = require "zmq"

local ctx = zmq.init()
local s = ctx:socket(zmq.REQ)
s:connect("ipc://.bitcoin.rep")

v = zmq.version()
print("Using ZMQ Version " .. v[1] .. "." ..  v[2] .. "." .. v[3])
s:send("[{\"method\":\"getblockcount\"},{\"method\":\"getblockcount\"}]")
local msg = s:recv()
print (msg)
s:send("{\"method\":\"getblockcount\"}")
msg = s:recv()
print (msg)
s:send("{\"method\":\"getblockhash\",\"params\":[228620]}")
msg = s:recv()
print (msg)
