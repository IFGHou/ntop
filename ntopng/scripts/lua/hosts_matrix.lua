--
-- (C) 2013-14 - ntop.org
--

dirs = ntop.getDirs()
package.path = dirs.installdir .. "/scripts/lua/modules/?.lua;" .. package.path

require "lua_utils"

sendHTTPHeader('text/html; charset=iso-8859-1')

ntop.dumpFile(dirs.installdir .. "/httpdocs/inc/header.inc")

active_page = "hosts"
dofile(dirs.installdir .. "/scripts/lua/inc/menu.lua")

function getTraffic(stats, host_a, host_b)
   sent_total = 0
   rcvd_total = 0

   --io.write(">>> "..host_a.." / "..host_b.."\n")

   for key, value in pairs(stats) do
      client = hostinfo2hostkey(flows_stats[key],"cli")
      server = hostinfo2hostkey(flows_stats[key],"srv")
      -- io.write(">>> "..flows_stats[key]["cli.ip"].." / "..flows_stats[key]["srv.ip"].."\n")
      if((client == host_a) and (server == host_b)) then
	 sent_total = sent_total +  flows_stats[key]["cli2srv.bytes"]
	 rcvd_total = rcvd_total + flows_stats[key]["srv2cli.bytes"]
      else
	 if((server == host_a) and (client == host_b)) then
	    sent_total = sent_total +  flows_stats[key]["srv2cli.bytes"]
	    rcvd_total = rcvd_total + flows_stats[key]["cli2srv.bytes"]
	 end   
      end
   end

   rc = { sent_total, rcvd_total }
   return(rc)
end

interface.find(ifname)
hosts_stats = interface.getHostsInfo()
flows_stats = interface.getFlowsInfo()

localhosts = {}
found = false
for key, value in pairs(hosts_stats) do
   --print(hosts_stats[key]["name"].."<p>\n")

   if((hosts_stats[key]["localhost"] == true) and (hosts_stats[key]["ip"] ~= nil)) then
      localhosts[key] = hosts_stats[key]

      -- io.write(key..'\n')
      -- io.write(ntop.getResolvedAddress(key)..'\n')
	 localhosts[key]["name"] = ntop.getResolvedAddress(key)

      found = true
   end
end

if(found == false) then
   print("<div class=\"alert alert-danger\"><img src=/img/warning.png> No local hosts can be found</div>")
else
   print("<hr><h2>Local Hosts Matrix</h2>\n<p>&nbsp;<p>\n<table class=\"table table-striped table-bordered\">\n")

   -- Header
   print("<tr><th>&nbsp;</th>")
   for key, value in pairs(localhosts) do
      print("<th>"..shortHostName(localhosts[key]["name"]).."</th>\n")
   end
   print("</tr>\n")

   for row_key, row_value in pairs(localhosts) do
      if(localhosts[row_key]["ip"] ~= nil) then
	 print("<tr><th><A HREF=\"/lua/host_details.lua?host="..row_key.."\">"..shortHostName(localhosts[row_key]["name"]).."</A></th>\n")
	 for column_key, column_value in pairs(localhosts) do	
	    val = "&nbsp;"
	    if(row_key ~= column_key) then
	       rsp = getTraffic(flows_stats, row_key, column_key)

	       if((rsp[1] > 0) or (rsp[2] > 0)) then	       
		  val = ""
		  if(rsp[1] > 0) then val = val .. '<span class="label label-warning" data-toggle="tooltip" data-placement="top" title="'..localhosts[row_key]["name"]..' -> ' .. localhosts[column_key]["name"] .. '\">'..bytesToSize(rsp[1]) .. '</span> ' end
		  if(rsp[2] > 0) then val = val .. '<span class="label label-info" data-toggle="tooltip" data-placement="bottom" title="'..localhosts[column_key]["name"]..' -> ' .. localhosts[row_key]["name"]..'\">'..bytesToSize(rsp[2]) .. '</span> ' end
	       end
	    end
	    
	    print("<td align=center>" .. val .. "</td>\n")
	 end
	 print("</tr>\n")
      end
   end


   print("</table>\n")
end

-- Activate tooltips
print [[
	 <script type="text/javascript">
	 $(document).ready(function () { $("span").tooltip({ 'selector': '', 'placement': 'bottom'  });});
			   </script>
			      </script>
			]]

		     dofile(dirs.installdir .. "/scripts/lua/inc/footer.lua")
