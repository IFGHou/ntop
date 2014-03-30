--
-- (C) 2013 - ntop.org
--

dirs = ntop.getDirs()
package.path = dirs.installdir .. "/scripts/lua/modules/?.lua;" .. package.path

require "lua_utils"

sendHTTPHeader('text/json')

interface.find(ifname)
flows_stats = interface.getFlowsInfo()


links = {}
num = 0

-- Create links (flows)

for key, value in pairs(flows_stats) do
  
  -- Find client and server name
  srv_name = flows_stats[key]["srv.host"]
  if((srv_name == "") or (srv_name == nil)) then
    srv_name = flows_stats[key]["srv.ip"]
  end
  srv_name = ntop.getResolvedAddress(srv_name)

  cli_name = flows_stats[key]["cli.host"]
  if((cli_name == "") or (cli_name == nil)) then
    cli_name = flows_stats[key]["cli.ip"]
  end
  cli_name = ntop.getResolvedAddress(cli_name)

  if (flows_stats[key]["client_process"] ~= nil) then 
    client_id = flows_stats[key]["cli.source_id"]..'-'..flows_stats[key]["cli.ip"]..'-'..flows_stats[key]["client_process"]["pid"]
    client_name = flows_stats[key]["client_process"]["name"]
    client_type = "syshost"
  else
    client_id = flows_stats[key]["cli.source_id"]..'-'..flows_stats[key]["cli.ip"]
    client_name = abbreviateString(cli_name, 20)
    client_type = "host"
  end

  if (flows_stats[key]["server_process"] ~= nil) then 
    server_id = flows_stats[key]["srv.source_id"]..'-'..flows_stats[key]["srv.ip"]..'-'..flows_stats[key]["server_process"]["pid"]
    server_name = flows_stats[key]["server_process"]["name"]
    server_type = "syshost"
  else
    server_id = flows_stats[key]["srv.source_id"]..'-'..flows_stats[key]["srv.ip"]
    server_name = abbreviateString(srv_name, 20)
    server_type = "host"
  end

  key_link = client_id.."-"..client_name..":"..server_id.."-"..server_name
  -- print("Key:"..key)
  if (links[key_link] == nil) then
    links[key_link] = {};
    links[key_link]["client_id"] = client_id
    links[key_link]["client_system_id"] = flows_stats[key]["cli.source_id"];
    links[key_link]["client_name"] = client_name
    links[key_link]["client_type"] = client_type
    links[key_link]["server_id"] = server_id
    links[key_link]["server_system_id"] = flows_stats[key]["srv.source_id"];
    links[key_link]["server_name"] = server_name
    links[key_link]["server_type"] = server_type
    links[key_link]["bytes"] = flows_stats[key]["bytes"]
    links[key_link]["srv2cli.bytes"] = flows_stats[key]["srv2cli.bytes"]
    links[key_link]["cli2srv.bytes"] = flows_stats[key]["cli2srv.bytes"]
  else
    -- Aggregate values
    links[key_link]["bytes"] = links[key_link]["bytes"] + flows_stats[key]["bytes"]
    links[key_link]["cli2srv.bytes"] = links[key_link]["cli2srv.bytes"] + flows_stats[key]["cli2srv.bytes"]
    links[key_link]["srv2cli.bytes"] = links[key_link]["srv2cli.bytes"] + flows_stats[key]["srv2cli.bytes"]
  end
    
end 


print('[\n')

-- Create link (flows)

num = 0
for key, value in pairs(links) do

  process = 1

  -- Condition

  -- if ((flows_stats[key]["server_process"] == nil) or 
  --   (flows_stats[key]["client_process"] == nil)) then 
  --   process = 0
  -- end


  -- Get information
  if(process == 1) then
    if (num > 0) then print(',\n') end

    print('{'..
      '\"client\":\"'           .. links[key]["client_id"]        .. '\",' ..
      '\"client_system_id\":\"' .. links[key]["client_system_id"] .. '\",' ..
      '\"client_name\":\"'      .. links[key]["client_name"]      .. '\",' ..
      '\"client_type\":\"'      .. links[key]["client_type"]      .. '\",' ..
      '\"server\":\"'           .. links[key]["server_id"]        .. '\",' ..
      '\"server_system_id\":\"' .. links[key]["server_system_id"] .. '\",' ..
      '\"server_name\":\"'      .. links[key]["server_name"]      .. '\",' ..
      '\"server_type\":\"'      .. links[key]["server_type"]      .. '\",' ..
      '\"bytes\":'              .. links[key]["bytes"]            .. ','   ..
      '\"cli2srv_bytes\":'      .. links[key]["cli2srv.bytes"]    .. ','   ..
      '\"srv2cli_bytes\":'      .. links[key]["srv2cli.bytes"]    ..
    '}')
    num = num + 1
  end

end

print('\n]')
