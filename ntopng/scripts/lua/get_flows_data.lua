--
-- (C) 2013 - ntop.org
--

dirs = ntop.getDirs()
package.path = dirs.installdir .. "/scripts/lua/modules/?.lua;" .. package.path

require "lua_utils"

sendHTTPHeader('text/html')
local debug = false

-- Table parameters
currentPage = _GET["currentPage"]
perPage     = _GET["perPage"]
sortColumn  = _GET["sortColumn"]
sortOrder   = _GET["sortOrder"]
host        = _GET["host"]
port        = _GET["port"]
application = _GET["application"]

-- Host comparison parameters
aggregation = _GET["aggregation"]
key = _GET["key"]

-- System host parameters
hosts = _GET["hosts"]
user = _GET["user"]
pid = tonumber(_GET["pid"])
name = _GET["name"]

if(currentPage == nil) then
   currentPage = 1
else
   currentPage = tonumber(currentPage)
end

if(perPage == nil) then
   perPage = 10
else
   perPage = tonumber(perPage)
end

if(port ~= nil) then port = tonumber(port) end

interface.find(ifname)
flows_stats = interface.getFlowsInfo()

print ("{ \"currentPage\" : " .. currentPage .. ",\n \"data\" : [\n")
total = 0
to_skip = (currentPage-1) * perPage

--host = "a"

-- Prepare host
host_list = {}
num_host_list = 0
sigle_host = 0

if (hosts ~= nil) then host_list, num_host_list = getHostCommaSeparatedList(hosts) end
if (host ~= nil) then
   sigle_host = 1
   num_host_list = 1
end

-- Prepare aggregation
if ((aggregation ~= nil) and (key ~= nil)) then

   if (aggregation == "ndpi") then application = key end
   if (aggregation == "l4proto") then l4proto = key end
   if (aggregation == "port") then port = tonumber(key) end

end

vals = {}
num = 0

for key, value in pairs(flows_stats) do
   --   print(key.."\n")
   if (debug) then io.write("==================\n")end
   
   process = 1
   client_process = 0
   server_process = 0

  if (debug) then io.write("Cli:"..flows_stats[key]["cli.ip"].."\n")end
  if (debug) then io.write("Srv:"..flows_stats[key]["srv.ip"].."\n")end


   ---------------- L4 PROTO ----------------
   if(l4proto ~= nil) then
      if (flows_stats[key]["proto.l4"] ~= l4proto) then
	 process = 0
      end
   end
   if (debug) then io.write("L4 -\t"..process.."\n")end
   
   ---------------- USER ----------------
   if(user ~= nil) then
    if (debug) then io.write("User:"..user.."\n")end
    if (flows_stats[key]["client_process"] ~= nil) then 
      if (debug) then io.write("Client user:"..flows_stats[key]["client_process"]["user_name"].."\n") end
      if ((flows_stats[key]["client_process"]["user_name"] == user)) then 
        client_process = 1
      end
      if (debug) then io.write("USER: => ClientProcess -\t"..client_process.."\n")end
    end
      if (flows_stats[key]["server_process"] ~= nil) then 
      if (debug) then io.write("Server user:"..flows_stats[key]["server_process"]["user_name"].."\n") end
      if ((flows_stats[key]["server_process"]["user_name"] == user)) then 
        server_process = 1
        if (debug) then io.write("USER: => 1ServerProcess -\t"..server_process.."\n")end
      end
      if (debug) then io.write("USER: => ServerProcess -\t"..server_process.."\n")end
      end
     if ((client_process == 1) or (server_process == 1)) then
      process = 1
    else
      process = 0
    end
   end
   if (debug) then io.write("user -\t"..process.."\n")end

   ---------------- PID ----------------
   if(pid ~= nil) then
    if (debug) then io.write("Pid:"..pid.."\n")end
    if (flows_stats[key]["client_process"] ~= nil) then 
      if (debug) then io.write("Client pid:"..flows_stats[key]["client_process"]["pid"].."\n") end
      if ((flows_stats[key]["client_process"]["pid"] == pid)) then 
        client_process = 1
      end
      if (debug) then io.write("PID: => ClientProcess -\t"..client_process.."\n")end
    end
    if (flows_stats[key]["server_process"] ~= nil) then 
      if (debug) then io.write("Server pid:"..flows_stats[key]["server_process"]["pid"].."\n") end
      if ((flows_stats[key]["server_process"]["pid"] == pid)) then 
        server_process = 1
      end
      if (debug) then io.write("PID: => ServerProcess -\t"..server_process.."\n")end
    end
    if ((client_process == 1) or (server_process == 1)) then
      process = 1
    else
      process = 0
    end
   end
   if (debug) then io.write("pid -\t"..process.."\n")end
   

  ---------------- NAME ----------------
   if(name ~= nil) then
    if (debug) then io.write("Name:"..name.."\n")end
    if (flows_stats[key]["client_process"] ~= nil) then 
      if (debug) then io.write("Client name:"..flows_stats[key]["client_process"]["name"].."\n") end
      if ((flows_stats[key]["client_process"]["name"] == name)) then 
        client_process = 1
      end
      if (debug) then io.write("ClientProcess -\t"..client_process.."\n")end
    end
    if (flows_stats[key]["server_process"] ~= nil) then 
      if (debug) then io.write("Server name:"..flows_stats[key]["server_process"]["name"].."\n") end
      if ((flows_stats[key]["server_process"]["name"] == name)) then 
        server_process = 1
      end
      if (debug) then io.write("ServerProcess -\t"..server_process.."\n")end
    end
    if ((client_process == 1) or (server_process == 1)) then
      process = 1
    else
      process = 0
    end
   end
   if (debug) then io.write("name -\t"..process.."\n")end
   

   ---------------- APP ----------------
   if(application ~= nil) then
      if(flows_stats[key]["proto.ndpi"] == "(Too Early)") then
         if(application ~= "Unknown") then process = 0 end
      else
	 if(flows_stats[key]["proto.ndpi"] ~= application) then
	    process = 0
	 end
      end
   end
   if (debug) then io.write("ndpi -\t"..process.."\n")end

  ---------------- PORT ----------------
   if(port ~= nil) then
      if((flows_stats[key]["cli.port"] ~= port) and (flows_stats[key]["srv.port"] ~= port)) then
   process = 0
      end
   end
   if (debug) then io.write("port -\t"..process.."\n")end   

   ---------------- HOST ----------------
   if(num_host_list > 0) then
      if(sigle_host == 1) then
        if (debug) then io.write("Host:"..host.."\n")end 
   if((flows_stats[key]["cli.ip"] ~= host) and (flows_stats[key]["srv.ip"] ~= host)) then
      process = 0
   end
      elseif ((findStringArray(flows_stats[key]["cli.ip"],host_list) == nil) and
     (findStringArray(flows_stats[key]["srv.ip"],host_list) == nil))then
   process  = 0
      end 
   end
   if (debug) then io.write("Host -\t"..process.."\n")end
   

  ---------------- TABLE SORTING ----------------
   if(process == 1) then 
    if (debug) then io.write("Flow Processing\n")end
    if ((flows_stats[key]["cli.ip"] == "192.12.193.5") or 
          (flows_stats[key]["srv.ip"] == "192.12.193.5"))then
    debug = false
  end
      -- postfix is used to create a unique key otherwise entries with the same key will disappear
      num = num + 1
      postfix = string.format("0.%04u", num)
      if(sortColumn == "column_client") then
	 vkey = flows_stats[key]["cli.ip"]..postfix
	 elseif(sortColumn == "column_server") then
	 vkey = flows_stats[key]["srv.ip"]..postfix
	 elseif(sortColumn == "column_bytes") then
	 vkey = flows_stats[key]["bytes"]+postfix
	 elseif(sortColumn == "column_vlan") then
	 vkey = flows_stats[key]["vlan"]+postfix
	 elseif(sortColumn == "column_bytes_last") then
	 vkey = flows_stats[key]["bytes.last"]+postfix
	 elseif(sortColumn == "column_ndpi") then
	 vkey = flows_stats[key]["proto.ndpi"]..postfix
	 elseif(sortColumn == "column_server_process") then
	 if(flows_stats[key]["server_process"] ~= nil) then
	    vkey = flows_stats[key]["server_process"]["name"]..postfix
	 else
	    vkey = postfix
	 end
	 elseif(sortColumn == "column_client_process") then
	 if(flows_stats[key]["client_process"] ~= nil) then
	    vkey = flows_stats[key]["client_process"]["name"]..postfix
	 else
	    vkey = postfix
	 end
	 elseif(sortColumn == "column_category") then
	 c = flows_stats[key]["category"]
	 if(c == nil) then c = "" end
   	 vkey = c..postfix
	 elseif(sortColumn == "column_duration") then
	 vkey = flows_stats[key]["duration"]+postfix	  
	 elseif(sortColumn == "column_thpt") then
	 vkey = flows_stats[key]["throughput"]+postfix	  
	 elseif(sortColumn == "column_proto_l4") then
	 vkey = flows_stats[key]["proto.l4"]..postfix
      else
	 -- By default sort by bytes
	 vkey = flows_stats[key]["bytes"]+postfix
      end
      
      --      print("-->"..num.."="..vkey.."\n")
      vals[vkey] = key
   end
end

num = 0
table.sort(vals)

if(sortOrder == "asc") then
   funct = asc
else
   funct = rev
end


for _key, _value in pairsByKeys(vals, funct) do
   key = vals[_key]   
   value = flows_stats[key]

   --   print(key.."="..flows_stats[key]["duration"].."\n");
   --   print(key.."=".."\n");
   -- print(key.."/num="..num.."/perPage="..perPage.."/toSkip="..to_skip.."\n")	 
   if(to_skip > 0) then
      to_skip = to_skip-1
   else
      if(num < perPage) then
	 if(num > 0) then
	    print ",\n"
	 end
	 srv_tooltip = ""
	 cli_tooltip = ""

	 srv_name = value["srv.host"]
	 if((srv_name == "") or (srv_name == nil)) then
	    srv_name = value["srv.ip"]
	 end
	 srv_name = ntop.getResolvedAddress(srv_name)
	 if (srv_name ~= value["srv.ip"]) then
	    srv_tooltip = value["srv.ip"]
	 end

	 cli_name = value["cli.host"]
	 if((cli_name == "") or (cli_name == nil)) then
	    cli_name = value["cli.ip"]
	 end
	 cli_name = ntop.getResolvedAddress(cli_name)
	 if (cli_name ~= value["cli.ip"]) then
	    cli_tooltip = value["cli.ip"]
	 end

	 src_key="<A HREF='/lua/host_details.lua?host=" .. value["cli.ip"] .. "' data-toggle='tooltip' title='" ..cli_tooltip.. "' >".. abbreviateString(cli_name, 20)
	 if(value["cli.systemhost"] == true) then src_key = src_key .. "&nbsp;<i class='fa fa-flag'></i>" end
	 src_key = src_key .. "</A>"

	 if(value["cli.port"] > 0) then
	    src_port=":<A HREF='/lua/port_details.lua?port=" .. value["cli.port"] .. "'>"..value["cli.port"].."</A>"
         else
	    src_port=""
         end

	 dst_key="<A HREF='/lua/host_details.lua?host=" .. value["srv.ip"] .. "' data-toggle='tooltip' title='" ..srv_tooltip.. "' >".. abbreviateString(srv_name, 20)
	 if(value["srv.systemhost"] == true) then dst_key = dst_key .. "&nbsp;<i class='fa fa-flag'></i>" end
	 dst_key = dst_key .. "</A>"

	 if(value["srv.port"] > 0) then
	    dst_port=":<A HREF='/lua/port_details.lua?port=" .. value["srv.port"] .. "'>"..value["srv.port"].."</A>"
         else
	    dst_port=""
         end

	 descr=cli_name..":"..value["cli.port"].." &lt;-&gt; "..srv_name..":"..value["srv.port"]
	 print ("{ \"column_key\" : \"<A HREF='/lua/flow_details.lua?flow_key=" .. key .. "&label=" .. descr.."'><span class='label label-info'>Info</span></A>")
	 print ("\", \"column_client\" : \"" .. src_key)

	 info = interface.getHostInfo(value["cli.ip"])
	 if(info ~= nil) then
	    if((info["country"] ~= nil) and (info["country"] ~= "")) then
	       print(" <img src='/img/blank.gif' class='flag flag-".. string.lower(info["country"]) .."'> ")
	    end
	 end

	 print(src_port)

	 print ("\", \"column_server\" : \"" .. dst_key)

	 info = interface.getHostInfo(value["srv.ip"])
	 if(info ~= nil) then
	    if((info["country"] ~= nil) and (info["country"] ~= "")) then
	       print(" <img src='/img/blank.gif' class='flag flag-".. string.lower(info["country"]) .."'> ")
	    end
	 end


	 print(dst_port)
	 print ("\", \"column_vlan\" : \"" .. value["vlan"])

	 if((value["vlan"] ~= nil) and (value["vlan"] ~= 0)) then 
	    print("\", \"column_vlan\" : "..value["vlan"]) 
	 else
	    print("\", \"column_vlan\" : \"\"") 
	 end

	 if(value["category"] ~= nil) then print (", \"column_category\" : \"" .. getCategory(value["category"])) else print (",") end
	 -- if (debug) then io.write(value["category"].."[" .. getCategory(value["category"]).. "]\n")	 end
	 print ("\"column_proto_l4\" : \"" .. value["proto.l4"])
	 print ("\", \"column_ndpi\" : \"" .. getApplicationLabel(value["proto.ndpi"]))
	 if(value["client_process"] ~= nil) then
	    print ("\", \"column_client_process\" : \"<A HREF=/lua/get_process_info.lua?pid=".. value["client_process"]["pid"] .."&name="..value["client_process"]["name"].."&host="..value["cli.ip"]..">" .. value["client_process"]["name"].."</A>")
	    print ("\", \"column_client_user_name\" : \"<A HREF=/lua/get_user_info.lua?user=" .. value["client_process"]["user_name"] .."&host="..value["cli.ip"]..">" .. value["client_process"]["user_name"].."</A>")
	 end
	 if(value["server_process"] ~= nil) then
	    print ("\", \"column_server_process\" : \"<A HREF=/lua/get_process_info.lua?pid=".. value["server_process"]["pid"] .."&name="..value["server_process"]["name"].."&host="..value["srv.ip"]..">" .. value["server_process"]["name"].."</A>")
	    print ("\", \"column_server_user_name\" : \"<A HREF=/lua/get_user_info.lua?user=" .. value["server_process"]["user_name"] .."&host="..value["srv.ip"]..">" .. value["server_process"]["user_name"].."</A>")
	 end

	 print ("\", \"column_duration\" : \"" .. secondsToTime(value["duration"]))
	 print ("\", \"column_bytes\" : \"" .. bytesToSize(value["bytes"]) .. "")

	 if(value["throughput_trend"] > 0) then 
	    print ("\", \"column_thpt\" : \"" .. bitsToSize(8*value["throughput"]).. " ")

	    if(value["throughput_trend"] == 1) then 
	       print("<i class='fa fa-arrow-up'></i>")
	       elseif(value["throughput_trend"] == 2) then
	       print("<i class='fa fa-arrow-down'></i>")
	       elseif(value["throughput_trend"] == 3) then
	       print("<i class='fa fa-minus'></i>")
	    end
	    print("\"")
	 else
	    print ("\", \"column_thpt\" : \"\"")
	 end

	 cli2srv = round((value["cli2srv.bytes"] * 100) / value["bytes"], 0)
	 print (", \"column_breakdown\" : \"<div class='progress'><div class='bar bar-warning' style='width: " .. cli2srv .."%;'>Client</div><div class='bar bar-info' style='width: " .. (100-cli2srv) .. "%;'>Server</div></div>")

	 print ("\" }\n")
	 num = num + 1
      end
   end

   total = total + 1
end -- for


print ("\n], \"perPage\" : " .. perPage .. ",\n")

if(sortColumn == nil) then
   sortColumn = ""
end

if(sortOrder == nil) then
   sortOrder = ""
end

print ("\"sort\" : [ [ \"" .. sortColumn .. "\", \"" .. sortOrder .."\" ] ],\n")
print ("\"totalRows\" : " .. total .. " \n}")
