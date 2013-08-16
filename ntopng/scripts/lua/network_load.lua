--
-- (C) 2013 - ntop.org
--

dirs = ntop.getDirs()
package.path = dirs.installdir .. "/scripts/lua/modules/?.lua;" .. package.path

require "lua_utils"

interface.find(ifname)
ifstats = interface.getStats()
info = ntop.getInfo()

sendHTTPHeader('text/html')
--sendHTTPHeader('application/json')

print('{ "packets": '.. ifstats.stats_packets .. ', "bytes": ' .. ifstats.stats_bytes .. ', "drops": ' .. ifstats.stats_drops .. ', "num_flows": '.. ifstats.stats_flows .. ', "num_hosts": ' .. ifstats.stats_hosts .. ', "epoch": ' .. os.time()..', "uptime": " ' .. secondsToTime(info["uptime"]) .. '" }\n')
