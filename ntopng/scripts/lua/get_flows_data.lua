ifname      = _GET["if"]
currentPage = _GET["currentPage"]
perPage     = _GET["perPage"]
columnSort  = _GET["sort"]

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


if(ifname == nil) then	  
  ifname = "any"
end

interface.find(ifname)
hosts_stats = interface.getFlowsInfo()

print ("{ \"currentPage\" : " .. currentPage .. ",\n \"data\" : [\n")
num = 0
total = 0
to_skip = (currentPage-1) * perPage
for key, value in pairs(hosts_stats) do
    -- print(key.."/num="..num.."/perPage="..perPage.."/toSkip="..to_skip.."\n")	 
   if(to_skip > 0) then
      to_skip = to_skip-1
   else
      if(num < perPage) then
	 if(num > 0) then
	    print ",\n"
	 end

	 src_key="<A HREF='/host_details.lua?interface=".. ifname .. "&host=" .. value["src.ip"] .. "'>"..value["src.host"].."</A>"
	 if(value["src.port"] > 0) then
  	   src_port=":<A HREF='/port_details.lua?interface=".. ifname .. "&port=" .. value["src.port"] .. "'>"..value["src.port"].."</A>"
         else
	   src_port=""
         end

	 dst_key="<A HREF='/host_details.lua?interface=".. ifname .. "&host=" .. value["dst.ip"] .. "'>"..value["dst.host"].."</A>"
	 if(value["dst.port"] > 0) then
  	   dst_port=":<A HREF='/port_details.lua?interface=".. ifname .. "&port=" .. value["dst.port"] .. "'>"..value["dst.port"].."</A>"
         else
	   dst_port=""
         end

	 descr=value["src.host"]..":"..value["src.port"].." &lt;-&gt; "..value["dst.host"]..":"..value["dst.port"]
	 print ("{ \"column_key\" : \"<A HREF='/flow_details.lua?interface=".. ifname .. "&flow_key=" .. key .. "&label=" .. descr.."'><img border=0 src='/img/info.png'></A>")
	 print ("\", \"column_client\" : \"" .. src_key .. src_port)
	 print ("\", \"column_server\" : \"" .. dst_key .. dst_port)
	 print ("\", \"column_vlan\" : \"" .. value["vlan"])
	 print ("\", \"column_proto_l4\" : \"" .. value["proto.l4"])
	 print ("\", \"column_ndpi\" : \"" .. value["proto.ndpi"])
	 print ( "\", \"column_bytes\" : " .. value["bytes"])
	 print (" }\n")
	 num = num + 1
      end
   end

   total = total + 1
end -- for


print ("\n], \"perPage\" : " .. perPage .. ",\n")
print ("\"sort\" : [ [ \"column_0\", \"desc\" ] ],\n \"totalRows\" : " .. total .. " \n}")
