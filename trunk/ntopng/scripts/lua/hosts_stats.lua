--
-- (C) 2013 - ntop.org
--

dirs = ntop.getDirs()
package.path = dirs.workingdir .. "/scripts/lua/modules/?.lua;" .. package.path

require "lua_utils"

sendHTTPHeader('text/html')

ntop.dumpFile(dirs.workingdir .. "/httpdocs/inc/header.inc")

active_page = "hosts"
dofile dirs.workingdir .. "/scripts/lua/inc/menu.lua"

ntop.dumpFile(dirs.workingdir .. "/httpdocs/inc/hosts_stats_top.inc")

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

ntop.dumpFile(dirs.workingdir .. "/httpdocs/inc/hosts_stats_bottom.inc")
dofile dirs.workingdir .. "/scripts/lua/inc/footer.lua"
