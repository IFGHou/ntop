--
-- (C) 2014 - ntop.org
--

dirs = ntop.getDirs()
package.path = dirs.installdir .. "/scripts/lua/modules/?.lua;" .. package.path

require "lua_utils"

if(mode ~= "embed") then
   sendHTTPHeader('text/html; charset=iso-8859-1')
   ntop.dumpFile(dirs.installdir .. "/httpdocs/inc/header.inc")
   active_page = "hosts"
   dofile(dirs.installdir .. "/scripts/lua/inc/menu.lua")
end

print("<hr><h2>Hosts Interaction</H2>")

ntop.dumpFile(dirs.installdir .. "/httpdocs/inc/sprobe_process_header.inc")

print [[

d3.json("/lua/sprobe_hosts_data.lua", function(error, json) {
    if (error) return console.warn(error);
    links = json;
    
    // Compute the distinct nodes from the links.
    links.forEach(function(link) {
			s = link.source.split("@");
			t = link.target.split("@");
			link.source = s[0];
			link.sourceId = s[1];
			link.target = t[0];
			link.targetId = t[1];
			link.source = nodes[link.source] || (nodes[link.source] = {name: link.source_name, num:link.source, num_procs: link.source_num,
										   link: "/lua/sprobe_host_process.lua?host="+link.source+"&id="+link.sourceId+"&name="+link.source_name });
			link.target = nodes[link.target] || (nodes[link.target] = { name: link.target_name, num: link.target, num_procs: link.target_num,
										    link: "/lua/sprobe_host_process.lua?host="+link.target+"&id="+link.targetId+"&name="+link.target_name});
		     });
]]


ntop.dumpFile(dirs.installdir .. "/httpdocs/inc/sprobe_process.inc")

if(mode ~= "embed") then
dofile(dirs.installdir .. "/scripts/lua/inc/footer.lua")
end
