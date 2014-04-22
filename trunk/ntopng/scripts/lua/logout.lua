--
-- (C) 2013 - ntop.org
--

dirs = ntop.getDirs()
package.path = dirs.installdir .. "/scripts/lua/modules/?.lua;" .. package.path

require "lua_utils"
local debug = false

sendHTTPHeaderLogout('text/html')

ntop.delCache("sessions.".._SESSION["session"])
ntop.delCache("sessions.".._SESSION["session"]..".ifname")

if (debug) then io.write("Deleting ".."sessions.".._SESSION["session"].."\n") end
if (debug) then io.write("Deleting ".."sessions.".._SESSION["session"]..".ifname\n") end

print [[
 <meta http-equiv="refresh" content="1; URL=/">
<html>
<body>
 Logging out...
</body>
</html>

]]

