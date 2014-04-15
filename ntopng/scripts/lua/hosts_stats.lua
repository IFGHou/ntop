--
-- (C) 2013 - ntop.org
--

dirs = ntop.getDirs()
package.path = dirs.installdir .. "/scripts/lua/modules/?.lua;" .. package.path

require "lua_utils"

sendHTTPHeader('text/html')

ntop.dumpFile(dirs.installdir .. "/httpdocs/inc/header.inc")

mode = _GET["mode"]
if(mode == nil) then mode = "all" end

active_page = "hosts"
dofile(dirs.installdir .. "/scripts/lua/inc/menu.lua")

print [[
      <hr>
      <div id="table-hosts"></div>
	 <script>
	 $("#table-hosts").datatable({
					title: "Hosts List",
					url: "/lua/get_hosts_data.lua?mode=]]
print(mode..'",')

if(mode == "all") then
   print('title: "All Hosts",\n')
elseif(mode == "local") then
   print('title: "Local Hosts",\n')
else
   print('title: "Remote Hosts",\n')
end

print [[
	       showPagination: true,
	       buttons: [ '<div class="btn-group"><button class="btn dropdown-toggle" data-toggle="dropdown">Filter Hosts<span class="caret"></span></button> <ul class="dropdown-menu"><li><a href="/lua/hosts_stats.lua">All Hosts</a></li><li><a href="/lua/hosts_stats.lua?mode=local">Local Only</a></li><li><a href="/lua/hosts_stats.lua?mode=remote">Remote Only</a></li></ul> </div>' ],
	        columns: [
			     {
			     title: "IP Address",
				 field: "column_ip",
				 sortable: true,
	 	             css: {
			        textAlign: 'left'
			     }
				 },
			     {
			  ]]

ifstats = interface.getStats()

if(ifstats.iface_sprobe) then
   print('title: "Source Id",\n')
else
   print('title: "VLAN",\n')
end

print [[
				 field: "column_vlan",
				 sortable: true,
	 	             css: {
			        textAlign: 'center'
			     }

				 },
]]

ntop.dumpFile(dirs.installdir .. "/httpdocs/inc/hosts_stats_top.inc")

prefs = ntop.getPrefs()

if(prefs.is_categorization_enabled) then
print [[
			     {
			     title: "Category",
				 field: "column_category",
				 sortable: true,
	 	             css: {
			        textAlign: 'center'
			       }
			       },

		       ]]
end

if(prefs.is_httpbl_enabled) then
print [[
			     {
			     title: "HTTP:BL",
				 field: "column_httpbl",
				 sortable: true,
	 	             css: {
			        textAlign: 'center'
			       }
			       },

		       ]]
end



ntop.dumpFile(dirs.installdir .. "/httpdocs/inc/hosts_stats_bottom.inc")
dofile(dirs.installdir .. "/scripts/lua/inc/footer.lua")
