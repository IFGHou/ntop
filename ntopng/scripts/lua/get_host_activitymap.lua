--
-- (C) 2013 - ntop.org
--

dirs = ntop.getDirs()
package.path = dirs.installdir .. "/scripts/lua/modules/?.lua;" .. package.path

require "lua_utils"
require "graph_utils"

host_info = urt2hostinfo(_GET)

sendHTTPHeader('application/json')

interface.find(ifname)

if((_GET["aggregated"] == nil) or (_GET["aggregated"] == 0)) then
   aggregation = false
   --print("false")
else
   aggregation = true
   --print("true")
end
-- print(host_info["host"].."-".."-"..host_info["vlan"])
rsp = interface.getHostActivityMap(host_info["host"], aggregation,host_info["vlan"])
--print (host_ip)
print(rsp)