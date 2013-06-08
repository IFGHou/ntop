--
-- (C) 2013 - ntop.org
--

package.path = "./scripts/lua/modules/?.lua;" .. package.path
require "lua_utils"

ntop.dumpFile("./httpdocs/inc/header.inc")

active_page = "about"
dofile "./scripts/lua/menu.lua"
info = ntop.getInfo()

print [[
    <hr />
    <h2>About ntopng</h2>]]

print("<table class=\"table table-bordered table-striped\">\n")
print("<tr><th>Copyright</th><td>"..info["copyright"].."</td></tr>\n")
print("<tr><th>Authors</th><td>"..info["authors"].."</td></tr>\n")
print("<tr><th>License</th><td>"..info["license"].."</td></tr>\n")
print("<tr><th>Version</th><td>"..info["version"].."</td></tr>\n")
print("<tr><th>Uptime</th><td>"..secondsToTime(info["uptime"]).."</td></tr>\n")
print("<tr><th colspan=2 align=center>&nbsp;</th></tr>\n")
print("<tr><th><a href=http://www.rrdtool.org/>RRDtool</A> Version</th><td>"..info["version.rrd"].."</td></tr>\n")
print("<tr><th><a href=http://www.redis.io>Redis</A> Server Version</th><td>"..info["version.redis"].."</td></tr>\n")
print("<tr><th><a href=http://www.gnu.org/software/libmicrohttpd/>libmicrohttpd</A> Version</th><td>"..info["version.libmicrohttpd"].."</td></tr>\n")
print("<tr><th><a href=http://www.luajit.org>LuaJIT</A> Version</th><td>"..info["version.luajit"].."</td></tr>\n")
print("<tr><th><a href=http://www.zeromq.org>�MQ</A> Version</th><td>"..info["version.zmq"].."</td></tr>\n")



print("</table>\n")
dofile "./scripts/lua/footer.inc.lua"
