--
-- (C) 2013 - ntop.org
--

dirs = ntop.getDirs()
package.path = dirs.installdir .. "/scripts/lua/modules/?.lua;" .. package.path
require "lua_utils"

sendHTTPHeader('text/html')

ntop.dumpFile(dirs.installdir .. "/httpdocs/inc/header.inc")

active_page = "admin"
dofile(dirs.installdir .. "/scripts/lua/inc/menu.lua")

print [[
   <h2>Runtime Preferences</h2>
   <table class="table table-striped">
   ]]

-- ================================================================================

print('<tr><th colspan=2 class="info">Report Visualization</th></tr>')
toggleTableButton("Throughput Unit",
		  "Select the throughput unit to be displayed in traffic reports.",
		  "Bytes", "bps", "Packets", "pps", "toggle_thpt_content", "ntopng.prefs.thpt_content")

-- ================================================================================
print('<tr><th colspan=2 class="info">Traffic Storage (RRD)</th></tr>')

toggleTableButton("RRDs For Local Hosts", 
		  "Toggle the creation of RRDs for local hosts. Turn it off to save storage space.",
		  "On", "1", "Off", "0", "toggle_local", "ntopng.prefs.host_rrd_creation")

toggleTableButton("nDPI RRDs For Local Hosts",
		  "Toggle the creation of nDPI RRDs for local hosts. Enable their creation allows you to keep application protocol statistics at the cost of using more disk space.",
		  "On", "1", "Off", "0", "toggle_local_ndpi", "ntopng.prefs.host_ndpi_rrd_creation")

print [[
   </table>
   ]]

dofile(dirs.installdir .. "/scripts/lua/inc/footer.lua")
