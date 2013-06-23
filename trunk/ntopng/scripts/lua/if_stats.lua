--
-- (C) 2013 - ntop.org
--

dirs = ntop.getDirs()
package.path = dirs.workingdir .. "/scripts/lua/modules/?.lua;" .. package.path

require "lua_utils"
require "graph_utils"

sendHTTPHeader('text/html')

ntop.dumpFile(dirs.workingdir .. "/httpdocs/inc/header.inc")

active_page = "if_stats"
dofile(dirs.workingdir .. "/scripts/lua/inc/menu.lua")

page = _GET["page"]

ifname = _GET["interface"]
if(ifname == nil) then
   ifname = "any"
end

rrdname = ntop.getDataDir() .. "/rrd/interface.any/bytes.rrd"

url= '/lua/if_stats.lua?interface=' .. ifname

print [[
<div class="bs-docs-example">
            <div class="navbar">
              <div class="navbar-inner">
<ul class="nav">
]]



if((page == "overview") or (page == nil)) then
  print("<li class=\"active\"><a href=\"#\">Overview</a></li>\n")
else
  print("<li><a href=\""..url.."&page=overview\">Overview</a></li>")
end

if(ntop.exists(rrdname)) then
if(page == "historical") then
  print("<li class=\"active\"><a href=\"#\">Historical Activity</a></li>\n")
else
  print("<li><a href=\""..url.."&page=historical\">Historical Activity</a></li>")
end
end

print [[
</ul>
</div>
</div>
</div>
   ]]


if((page == "overview") or (page == nil)) then
   interface.find(ifname)
   ifstats = interface.getStats()

   print("<table class=\"table table-bordered\">\n")
   print("<tr><th>Name</th><td>" .. ifstats.name .. "</td></tr>\n")
   print("<tr><th>Bytes</th><td>" .. bytesToSize(ifstats.stats_bytes) .. "</td></tr>\n")
   print("<tr><th>Received Packets</th><td>" .. formatPackets(ifstats.stats_packets) .. "</td></tr>\n")
   print("<tr><th>Dropped Packets</th><td>")

   if(ifstats.stats_drops > 0) then print('<span class="label label-important">') end
   print(formatPackets(ifstats.stats_drops))

   if((ifstats.stats_packets+ifstats.stats_drops) > 0) then
      local pctg = round((ifstats.stats_drops*100)/(ifstats.stats_packets+ifstats.stats_drops), 2)   
      print(" [ " .. pctg .. " % ] ")
   end

   if(ifstats.stats_drops > 0) then print('</span>') end
   print("</td></tr>\n")
   print("</table>\n")
else
   drawRRD('interface.any', "bytes.rrd", _GET["graph_zoom"], url.."&page=historical", 0, _GET["epoch"], "/lua/top_talkers.lua")
end

dofile(dirs.workingdir .. "/scripts/lua/inc/footer.lua")
