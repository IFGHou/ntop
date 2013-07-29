--
-- (C) 2013 - ntop.org
--

dirs = ntop.getDirs()
package.path = dirs.installdir .. "/scripts/lua/modules/?.lua;" .. package.path
require "lua_utils"

ifname = _GET["if"]
if(ifname == nil) then
   ifname = "any"
end

hostname = _GET["name"]

interface.find(ifname)
host = interface.getAggregatedHostInfo(hostname)

sendHTTPHeader('text/html')
--sendHTTPHeader('application/json')

if(host == nil) then
   print('{ "name": \"' .. hostname .. '\"}\n')
   return
else
   diff = os.time()-host["seen.last"]
   print('{ "name": \"' .. hostname .. '\", "last_seen": "' .. os.date("%x %X", host["seen.last"]) .. ' ['.. secondsToTime(diff) .. ' ago]", "num_contacts": ' .. host["pkts.rcvd"] .. ', "epoch": ' .. os.time())

print(', "contacts": [')

num = 0
for k,v in pairs(host["contacts"]["client"]) do 
   if(num > 0) then print(",") end
   print('{ "key": "'..k..'", "value": '..v..'}')
   num = num + 1 
end

print ('] }\n')

end