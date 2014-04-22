--
-- (C) 2013 - ntop.org
--

dirs = ntop.getDirs()
package.path = dirs.installdir .. "/scripts/lua/modules/?.lua;" .. package.path

require "lua_utils"
require "flow_utils"
local json = require ("dkjson")

sendHTTPHeader('text/html')

ntop.dumpFile(dirs.installdir .. "/httpdocs/inc/header.inc")

function displayProc(proc) 
   print("<tr><th width=30%>Username</th><td colspan=2><A HREF=/lua/get_user_info.lua?user=".. proc.user_name .."&".. hostinfo2url(flow,"cli")..">".. proc.user_name .."</A></td></tr>\n")
   print("<tr><th width=30%>Process PID/Name</th><td colspan=2><A HREF=/lua/get_process_info.lua?pid=".. proc.pid .."&".. hostinfo2url(flow,"srv").. ">".. proc.pid .. "/" .. proc.name .. "</A>")
   print(" [son of <A HREF=/lua/get_process_info.lua?pid=".. proc.father_pid .. ">" .. proc.father_pid .. "/" .. proc.father_name .."</A>]</td></tr>\n")
   print("<tr><th width=30%>CPU ID</th><td colspan=2>".. proc.cpu_id .. "</td></tr>\n")
   print("<tr><th width=30%>Average CPU Load</th><td colspan=2>")

   if(proc.average_cpu_load < 33) then
      if(proc.average_cpu_load == 0) then proc.average_cpu_load = "< 1" end
      print("<font color=green>"..proc.average_cpu_load.." %</font>")
      elseif(proc.average_cpu_load < 66) then
      print("<font color=orange><b>"..proc.average_cpu_load.." %</b></font>")
   else
      print("<font color=red><b>"..proc.average_cpu_load.." %</b></font>")
   end
   print(" </td></tr>\n")

   print("<tr><th width=30%>Memory Actual/Peak</th><td colspan=2>".. bytesToSize(proc.actual_memory) .. " / ".. bytesToSize(proc.peak_memory) .. " [" .. round((proc.actual_memory*100)/proc.peak_memory, 1) .."%]</td></tr>\n")
   print("<tr><th width=30%>VM Page Faults</th><td colspan=2>")
   if(proc.num_vm_page_faults > 0) then
      print("<font color=red><b>"..proc.num_vm_page_faults.."</b></font>")
   else
      print("<font color=green>"..proc.num_vm_page_faults.."</font>")
   end

   print("</td></tr>\n")
end

active_page = "flows"
dofile(dirs.installdir .. "/scripts/lua/inc/menu.lua")

a = _GET["label"]

patterns = {
   ['_'] = "",
   ['-_'] = "<i class=\"fa fa-exchange fa-lg\"></i>"
}
for search,replace in pairs(patterns) do
   a = string.gsub(a, search, replace)
end

print [[

<div class="bs-docs-example">
            <div class="navbar">
              <div class="navbar-inner">
<ul class="nav">
	 <li><a href="#">Flow: ]] print(a) print [[ </a></li>
<li class="active"><a href="#">Overview</a></li>
<li><a href="javascript:history.go(-1)"><i class='fa fa-reply'></i></a></li>
</div>
</div>
</div>
]]


flow_key = _GET["flow_key"]
if(flow_key == nil) then
   flow = nil
else
   interface.find(ifname)
   flow = interface.findFlowByKey(tonumber(flow_key))
end

if(flow == nil) then
   print("<div class=\"alert alert-error\"><img src=/img/warning.png> This flow cannot be found (expired ?)</div>")
else
   print("<table class=\"table table-bordered\">\n")
   if(flow["vlan"] ~= 0) then
      print("<tr><th width=30%>VLAN ID</th><td colspan=2>" .. flow["vlan"].. "</td></tr>\n")
   end
   print("<tr><th width=30%>Client</th><td colspan=2><A HREF=\"/lua/host_details.lua?"..hostinfo2url(flow,"cli") .. "\">")
   if(flow["cli.host"] ~= "") then print(flow["cli.host"]) else print(flow["cli.ip"]) end
   if(flow["cli.systemhost"] == true) then print("&nbsp;<i class='fa fa-flag'></i>") end
   print("</A>")
   if(flow["cli.port"] > 0) then
      print(":<A HREF=\"/lua/port_details.lua?port=" .. flow["cli.port"].. "\">" .. flow["cli.port"])
   end
   print("</A></td></tr>\n")
   print("<tr><th width=30%>Server</th><td colspan=2><A HREF=\"/lua/host_details.lua?" .. hostinfo2url(flow,"srv") .. "\">")
   if(flow["srv.host"] ~= "") then print(flow["srv.host"]) else print(flow["srv.ip"]) end
   if(flow["srv.systemhost"] == true) then print("&nbsp;<i class='fa fa-flag'></i>") end
   print("</A>")
   if(flow["srv.port"] > 0) then
      print(":<A HREF=\"/lua/port_details.lua?port=" .. flow["srv.port"].. "\">" .. flow["srv.port"].. "</A>")
   end
   print("</td></tr>\n")
   if(flow["category"] ~= "") then
      print("<tr><th width=30%>Category</th><td colspan=2>" .. getCategory(flow["category"]) .. "</td></tr>\n")
   end

   print("<tr><th width=30%>Application Protocol</th><td colspan=2><A HREF=\"/lua/")
   if((flow.client_process ~= nil) or (flow.server_process ~= nil))then	print("s") end
   print("flows_stats.lua?application=" .. flow["proto.ndpi"] .. "\">" .. getApplicationLabel(flow["proto.ndpi"]) .. "</A></td></tr>\n")
   print("<tr><th width=30%>First Seen</th><td colspan=2><div id=first_seen>" .. formatEpoch(flow["seen.first"]) ..  " [" .. secondsToTime(os.time()-flow["seen.first"]) .. " ago]" .. "</div></td></tr>\n")
   print("<tr><th width=30%>Last Seen</th><td colspan=2><div id=last_seen>" .. formatEpoch(flow["seen.last"]) .. " [" .. secondsToTime(os.time()-flow["seen.last"]) .. " ago]" .. "</div></td></tr>\n")

   print("<tr><th width=30%>Total Traffic Volume</th><td colspan=2><span id=volume>" .. bytesToSize(flow["bytes"]) .. "</span> <span id=volume_trend></span></td></tr>\n")

   print("<tr><th width=30%>Client vs Server Traffic Breakdown</th><td colspan=2>")
   cli2srv = round((flow["cli2srv.bytes"] * 100) / flow["bytes"], 0)

   print('<div class="progress"><div class="bar bar-warning" style="width: ' .. cli2srv.. '%;">'.. flow["cli.ip"]..'</div><div class="bar bar-info" style="width: ' .. (100-cli2srv) .. '%;">' .. flow["srv.ip"] .. '</div></div>')
   print("</td></tr>\n")

   print("<tr><th width=30%>Client to Server Traffic</th><td colspan=2><span id=cli2srv>" .. formatPackets(flow["cli2srv.packets"]) .. " / ".. bytesToSize(flow["cli2srv.bytes"]) .. "</span> <span id=sent_trend></span></td></tr>\n")
   print("<tr><th width=30%>Server to Client Traffic</th><td colspan=2><span id=srv2cli>" .. formatPackets(flow["srv2cli.packets"]) .. " / ".. bytesToSize(flow["srv2cli.bytes"]) .. "</span> <span id=rcvd_trend></span></td></tr>\n")


   if((flow.client_process == nil) and (flow.server_process == nil)) then
      print("<tr><th width=30%>Actual Throughput</th><td width=20%>")

      print("<span id=throughput>" .. bitsToSize(8*flow["throughput"]) .. "</span> <span id=throughput_trend></span>")
      print("</td><td><span id=thpt_load_chart>0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0</span>")

      print("</td></tr>\n")
   else
      if(flow.client_process ~= nil) then
         print("<tr><th colspan=3 bgcolor=lightgray>Client Process Information</th></tr>\n")
         displayProc(flow.client_process)
      end
      if(flow.server_process ~= nil) then
         print("<tr><th colspan=3 bgcolor=lightgray>Server Process Information</th></tr>\n")
         displayProc(flow.server_process)	 
      end
   end

   if(flow["tcp_flags"] > 0) then
      print("<tr><th width=30%>TCP Flags</th><td colspan=2>")

      if(hasbit(flow["tcp_flags"],0x01)) then print('<span class="label label-info">FIN</span> ')  end
      if(hasbit(flow["tcp_flags"],0x02)) then print('<span class="label label-info">SYN</span> ')  end
      if(hasbit(flow["tcp_flags"],0x04)) then print('<span class="label label-info">RST</span> ')  end
      if(hasbit(flow["tcp_flags"],0x08)) then print('<span class="label label-info">PUSH</span> ') end
      if(hasbit(flow["tcp_flags"],0x10)) then print('<span class="label label-info">ACK</span> ')  end
      if(hasbit(flow["tcp_flags"],0x20)) then print('<span class="label label-info">URG</span> ')  end

      print("</td></tr>\n")
   end

   local info, pos, err = json.decode(flow["moreinfo.json"], 1, nil)
   num = 0
   for key,value in pairs(info) do
      if(num == 0) then
	 print("<tr><th colspan=3 bgcolor=lightgray>Additional Flow Elements</th></tr>\n")
      end
      print("<tr><th width=30%>" .. getFlowKey(key) .. "</th><td colspan=2>" .. handleCustomFlowField(key, value) .. "</td></tr>\n")
      num = num + 1
   end

   print("</table>\n")
end

print [[
<script>
/*
      $(document).ready(function() {
	      $('.progress .bar').progressbar({ use_percentage: true, display_text: 1 });
   });
*/


var thptChart = $("#thpt_load_chart").peity("line", { width: 64 });
]]

if(flow ~= nil) then
   print("var cli2srv_packets = " .. flow["cli2srv.packets"] .. ";")
   print("var srv2cli_packets = " .. flow["srv2cli.packets"] .. ";")
   print("var throughput = " .. flow["throughput"] .. ";")
   print("var bytes = " .. flow["bytes"] .. ";")
end

print [[
setInterval(function() {
	  $.ajax({
		    type: 'GET',
		    url: '/lua/flow_stats.lua',
		    data: { ifname: "]] print(tostring(interface.name2id(ifname))) print [[", flow_key: "]] print(flow_key) print [[" },
		    success: function(content) {
			var rsp = jQuery.parseJSON(content);
			$('#first_seen').html(rsp["seen.first"]);
			$('#last_seen').html(rsp["seen.last"]);
			$('#volume').html(bytesToVolume(rsp.bytes));
			$('#cli2srv').html(addCommas(rsp["cli2srv.packets"])+" Pkts / "+bytesToVolume(rsp["cli2srv.bytes"]));
			$('#srv2cli').html(addCommas(rsp["srv2cli.packets"])+" Pkts / "+bytesToVolume(rsp["srv2cli.bytes"]));
			$('#throughput').html(rsp.throughput);

			/* **************************************** */

			if(cli2srv_packets == rsp["cli2srv.packets"]) {
			   $('#sent_trend').html("<i class=\"fa fa-minus\"></i>");
			} else {
			   $('#sent_trend').html("<i class=\"fa fa-arrow-up\"></i>");
			}

			if(srv2cli_packets == rsp["srv2cli.packets"]) {
			   $('#rcvd_trend').html("<i class=\"fa fa-minus\"></i>");
			} else {
			   $('#rcvd_trend').html("<i class=\"fa fa-arrow-up\"></i>");
			}

			if(bytes == rsp["bytes"]) {
			   $('#volume_trend').html("<i class=\"fa fa-minus\"></i>");
			} else {
			   $('#volume_trend').html("<i class=\"fa fa-arrow-up\"></i>");
			}

			if(throughput > rsp["throughput"]) {
			   $('#throughput_trend').html("<i class=\"fa fa-arrow-down\"></i>");
			} else if(throughput < rsp["throughput"]) {
			   $('#throughput_trend').html("<i class=\"fa fa-arrow-up\"></i>");
			} else {
			   $('#throughput_trend').html("<i class=\"fa fa-minus\"></i>");
			}

			cli2srv_packets = rsp["cli2srv.packets"];
			srv2cli_packets = rsp["srv2cli.packets"];
			throughput = rsp["throughput"];
			bytes = rsp["bytes"];

			/* **************************************** */

			var values = thptChart.text().split(",");
			values.shift();
			values.push(rsp.throughput_raw);
			thptChart.text(values.join(",")).change();
		     }
	           });
		 }, 3000);

</script>
 ]]

dofile(dirs.installdir .. "/scripts/lua/inc/footer.lua")
