--
-- (C) 2013 - ntop.org
--

dirs = ntop.getDirs()
package.path = dirs.installdir .. "/scripts/lua/modules/?.lua;" .. package.path
require "lua_utils"
require "top_talkers"

sendHTTPHeader('text/html')

print(getTopTalkers(ifname, _GET["mode"], _GET["epoch"]))