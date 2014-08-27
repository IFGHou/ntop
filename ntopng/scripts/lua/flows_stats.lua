--
-- (C) 2013-14 - ntop.org
--

dirs = ntop.getDirs()
package.path = dirs.installdir .. "/scripts/lua/modules/?.lua;" .. package.path

require "lua_utils"

sendHTTPHeader('text/html; charset=iso-8859-1')
ntop.dumpFile(dirs.installdir .. "/httpdocs/inc/header.inc")

active_page = "flows"
dofile(dirs.installdir .. "/scripts/lua/inc/menu.lua")

application = _GET["application"]
application_filter = ""
hosts = _GET["hosts"]
aggregation = _GET["aggregation"]
key = _GET["key"]

host = _GET["host"]

network_id=_GET["network_id"]

prefs = ntop.getPrefs()
interface.find(ifname)
ifstats = interface.getStats()
ndpistats = interface.getNdpiStats()

num_param = 0
print [[
      <hr>
      <div id="table-flows"></div>
	 <script>
   var url_update = "/lua/get_flows_data.lua]]

if(application ~= nil) then
   print("?application="..application)
   num_param = num_param + 1
   application_filter = '<span class="glyphicon glyphicon-filter"></span>'
end

if(host ~= nil) then
  if(application ~= nil) then print("&") else print("?") end
   print("host="..host)
   num_param = num_param + 1
end

if(hosts ~= nil) then
  if (num_param > 0) then
    print("&")
  else
    print("?")
  end
  print("hosts="..hosts)
  num_param = num_param + 1
end

if(aggregation ~= nil) then
  if (num_param > 0) then
    print("&")
  else
    print("?")
  end
  print("aggregation="..aggregation)
  num_param = num_param + 1
end

if(key ~= nil) then
  if (num_param > 0) then
    print("&")
  else
    print("?")
  end
  print("key="..key)
  num_param = num_param + 1
end

if(network_id ~= nil) then
  if (num_param > 0) then
    print("&")
  else
    print("?")
  end
  print("network_id="..network_id)
  num_param = num_param + 1
end


print ('";')

ntop.dumpFile(dirs.installdir .. "/httpdocs/inc/flows_stats_id.inc")
-- Set the flow table option
if(prefs.is_categorization_enabled) then print ('flow_rows_option["categorization"] = true;\n') end
if(ifstats.iface_vlan) then print ('flow_rows_option["vlan"] = true;\n') end
   print [[

	 var table = $("#table-flows").datatable({
			url: url_update , ]]
print ('rowCallback: function ( row ) { return flow_table_setID(row); },\n')

preference = tablePreferences("rows_number",_GET["perPage"])
if (preference ~= "") then print ('perPage: '..preference.. ",\n") end

print(" title: \"Active Flows")
if(_GET["network_name"] ~= nil) then
  print(" [".._GET["network_name"].."]")
end

print [[",
         showFilter: true,
	       showPagination: true,
]]

-- Automatic default sorted. NB: the column must be exists.
print ('sort: [ ["' .. getDefaultTableSort("flows") ..'","' .. getDefaultTableSortOrder("flows").. '"] ],\n')

print ('buttons: [ \'<div class="btn-group"><button class="btn btn-link dropdown-toggle" data-toggle="dropdown">Applications ' .. application_filter .. '<span class="caret"></span></button> <ul class="dropdown-menu" role="menu" id="flow_dropdown">')

print('<li><a href="/lua/flows_stats.lua">All Proto</a></li>')
for key, value in pairsByKeys(ndpistats["ndpi"], asc) do
   class_active = ''
   if(key == application) then
      class_active = ' class="active"'
   end
   print('<li '..class_active..'><a href="/lua/flows_stats.lua?application=' .. key)
   if(host ~= nil) then print('&host='..host) end
   print('">'..key..'</a></li>')
end


print("</ul> </div>' ],\n")


ntop.dumpFile(dirs.installdir .. "/httpdocs/inc/flows_stats_top.inc")

if(ifstats.iface_vlan) then
print [[
           {
           title: "VLAN",
         field: "column_vlan",
         sortable: true,
                 css: {
              textAlign: 'center'
           }
         },
]]
end


ntop.dumpFile(dirs.installdir .. "/httpdocs/inc/flows_stats_middle.inc")



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

ntop.dumpFile(dirs.installdir .. "/httpdocs/inc/flows_stats_bottom.inc")
dofile(dirs.installdir .. "/scripts/lua/inc/footer.lua")
