--
-- (C) 2014 - ntop.org
--

dirs = ntop.getDirs()
package.path = dirs.installdir .. "/scripts/lua/modules/?.lua;" .. package.path

require "lua_utils"

sendHTTPHeader('application/json')
-- Defaul value
local debug = false
interface.find(ifname)

max_num_links = 32
max_num_hosts = 8
aggregation = "ndpi"

compared_hosts = {}
compared_hosts_size = 0;

if(debug) then io.write("==== hosts_compared_sankey ====\n") end
hosts = _GET["hosts"]
if(debug) then io.write("Host:"..hosts.."\n") end

if (_GET["hosts"] ~= nil) then

  compared_hosts, compared_hosts_size = getHostCommaSeparatedList(_GET["hosts"])

  if (compared_hosts_size >= 2) then

    if(_GET["aggregation"] ~= nil) then
        aggregation = _GET["aggregation"]
    end

    -- 1.    Find all flows between compared hosts
    flows_stats = interface.getFlowsInfo()
    
    links = {}
    links_size = 0
    
    ndpi = {}
    ndpi_size = compared_hosts_size
    
    l4 = {}
    l4_size = compared_hosts_size
    
    ports = {}
    ports_size = compared_hosts_size
   

    for key, value in pairs(flows_stats) do

        process = 0
        if ((findStringArray(flows_stats[key]["cli.ip"],compared_hosts) ~= nil) and
            (findStringArray(flows_stats[key]["srv.ip"],compared_hosts) ~= nil))then
            process  = 1
        end -- findStringArray
        
        if (links_size > max_num_links) then process = 0 end
        if ((ndpi_size > max_num_hosts) or 
            (l4_size > max_num_hosts) or 
            (ports_size > max_num_hosts))then process = 0 end

        if (process == 1) then
            if (debug) then io.write("Cli:"..flows_stats[key]["cli.ip"]..",Srv:"..flows_stats[key]["srv.ip"]..",Ndpi:"..flows_stats[key]["proto.ndpi"]..",L4:"..flows_stats[key]["proto.l4"].."\n") end
            if (debug) then io.write("Aggregation:"..aggregation.."\n") end
            aggregation_value = {}
            if (aggregation == "ndpi") then
                if (debug) then io.write("=>Value:"..flows_stats[key]["proto.ndpi"].."\n") end
                -- 1.1   Save ndpi protocol
                if (ndpi[flows_stats[key]["proto.ndpi"]] == nil) then
                    ndpi[flows_stats[key]["proto.ndpi"]] = ndpi_size
                    ndpi_size = ndpi_size + 1
                    aggregation_value[0] = flows_stats[key]["proto.ndpi"]
                end
            end

            if (aggregation == "l4proto") then
                if (debug) then io.write("=>Value:"..flows_stats[key]["proto.l4"].."\n") end
                -- 1.2   Save l4 protocol
                if (l4[flows_stats[key]["proto.l4"]] == nil) then
                    l4[flows_stats[key]["proto.l4"]] = l4_size
                    l4_size = l4_size + 1
                    aggregation_value[0] = flows_stats[key]["proto.l4"]
                end
            end

            if (aggregation == "port") then
                if (debug) then io.write("=>Value:"..flows_stats[key]["cli.port"].."\n") end
                -- 1.3   Save port
                nport = 0
                if (ports[flows_stats[key]["cli.port"]] == nil) then
                    ports[flows_stats[key]["cli.port"]] = ports_size
                    ports_size = ports_size + 1
                    aggregation_value[nport] = flows_stats[key]["cli.port"]
                    nport = nport + 1
                end

                if (ports[flows_stats[key]["srv.port"]] == nil) then
                    ports[flows_stats[key]["srv.port"]] = ports_size
                    ports_size = ports_size + 1
                    aggregation_value[nport] = flows_stats[key]["srv.port"]
                    nport = nport + 1
                end
            end
            
            for k,v in pairs(aggregation_value) do
            
                if (links[flows_stats[key]["cli.ip"]..":"..v] == nil) then
                    links[flows_stats[key]["cli.ip"]..":"..v] = {}
                    links[flows_stats[key]["cli.ip"]..":"..v]["value"] = flows_stats[key]["cli2srv.bytes"]
                else
                    links[flows_stats[key]["cli.ip"]..":"..v]["value"] = links[flows_stats[key]["cli.ip"]..":"..v]["value"] + flows_stats[key]["cli2srv.bytes"]
                end
                
                if (links[flows_stats[key]["srv.ip"]..":"..v] == nil) then
                    links[flows_stats[key]["srv.ip"]..":"..v] = {}
                    links[flows_stats[key]["srv.ip"]..":"..v]["value"] = flows_stats[key]["srv2cli.bytes"]
                else
                    links[flows_stats[key]["srv.ip"]..":"..v]["value"] = links[flows_stats[key]["srv.ip"]..":"..v]["value"] + flows_stats[key]["cli2srv.bytes"]
                end

                if(debug) then io.write("Client: "..flows_stats[key]["cli.ip"]..", aggregation: "..v..",Value: "..links[flows_stats[key]["cli.ip"]..":"..v]["value"].."\n") end
                if(debug) then io.write("Server: "..flows_stats[key]["srv.ip"]..", aggregation: "..v..",Value: "..links[flows_stats[key]["srv.ip"]..":"..v]["value"].."\n") end
                
            end
        end
    end

    -- 2.    Create node
    print '{"nodes":[\n'

    -- 2.1   Host node
    node_size = 0
    
    for i,host_ip in ipairs(compared_hosts) do
      
      if(node_size > 0) then
        print ",\n"
      end
      
      print ("\t{\"name\": \"" .. ntop.getResolvedAddress(host_ip) .. "\", \"ip\": \"" .. host_ip .. "\"}")     
      node_size = node_size + 1
    end
    
    -- 2.2   Aggregation node

    if(aggregation == "l4proto") then
        aggregation_node = l4
    elseif (aggregation == "port") then
        aggregation_node = ports
    else
        -- Default ndpi
        aggregation_node = ndpi
    end
        
      for key,value in pairs(aggregation_node) do
          if(debug) then io.write("Aggregation Node: "..key.."\n") end
          if(node_size > 0) then
            print ",\n"
          end
      
          print ("\t{\"name\": \"" .. key .. "\", \"ip\": \"" .. key .. "\"}")    
          
          node_size = node_size + 1
      end

   
    -- 3.    Create links
   
    print "\n],\n"
    print '"links" : [\n'

    
    -- 2. print links
    num = 0
    for i,host_ip in ipairs(compared_hosts) do

       for aggregation_key,value in pairs(aggregation_node) do
           
           if(num > 0) then
               print ",\n"
           end

           if(links[host_ip..":"..aggregation_key] ~= nil) then
               val = links[host_ip..":"..aggregation_key]["value"]
               
               if (val == 0 ) then val = 1 end

               print ("\t{\"source\": "..(i -1).. ", \"target\": "..(compared_hosts_size + value -2)..", \"value\": " .. val .. ", \"aggregation\": \""..aggregation.."\", \"key\": \"" .. aggregation_key .."\"}")
               num = num + 1
           end

       end


    end


  end --End if (compared host size)
  print ("\n]}\n")
end -- End if _GET[hosts]





