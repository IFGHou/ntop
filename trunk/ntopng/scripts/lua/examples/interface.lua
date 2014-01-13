--
-- (C) 2013 - ntop.org
--

-- Ntop lua class example

function printTable(table,key)
  if (key ~= nil) then print(""..key..":<ul>") end
  for k, v in pairs(table) do
    if (type(v) == "table") then
       printTable(table[k],k)
    else
      if (type(v) == "boolean") then 
        if (v) then v = "true" else v = "false" end
      end
      print("<li>"..k .." = "..v.."<br>")
    end
  end
  print("</ul>")
end

-- Set package.path information to be able to require lua module
dirs = ntop.getDirs()
package.path = dirs.installdir .. "/scripts/lua/modules/?.lua;" .. package.path
require "lua_utils"

host_ip       = _GET["host"]
hostinfotype  = _GET["hostinfotype"]
interfacetype = _GET["interfacetype"]  
flowtype      = _GET["flowtype"]  

-- Here you can choose the type of your HTTP message {'text/html','application/json',...}. There are two main function that you can use:
-- function sendHTTPHeaderIfName(mime, ifname, maxage)
-- function sendHTTPHeader(mime)
-- For more information please read the scripts/lua/modules/lua_utils.lua file.
sendHTTPHeader('text/html')
ntop.dumpFile(dirs.installdir .. "/httpdocs/inc/header.inc")

print('<html><head><title>ntopng API Lua example</title></head>')
print('<body>')
print('<h1>Examples of interface lua class</h1>')
print('<p>This class provides to hook to objects that describe flows and hosts and it allows you to access to live monitoring data.<br><b>For more information, please read the source code of this file and the doxygen of API Lua.</b></p>')

print('<br><h2>Generic information of Network interface</h2>')
print('<p>By default ntopng set the \"ntop_inteface\"  global variable in lua stack, it is the network interface name where ntopng is running.<br>Every time when you want use the interface class, in order to refresh the \"ntop_interface\" global variable , please remember to call the method <b>interface.find(ifname))</b> before to use the interface class.</p>')
print('<pre><code>print("Default ifname = " .. interface.getDefaultIfName())\nprint("Network interface name = " .. interface.find(ifname))\nprint("Network interface id = " .. interface.name2id(ifname))</code></pre>')
print('<ul>')
print('<li>Default ifname = ' .. interface.getDefaultIfName())
print('<li>Network interface name = ' .. interface.find(ifname))
print('<li>Network interface id = ' .. interface.name2id(ifname))
if (interface.isRunning()) then
  print('<li>'..ifname..' is running')
else
  print('<li>'..ifname..' is not running')
end
print('</ul>')

print('<br><h4>Switch network interface</h4>')
print('<p>In order to switch the network interface where ntopng is running, you need to use the method <b>setActiveInterfaceId(id)</b>, for more information please read the documentation and if you are looking for a complete and correctly example how to switch interface and active a new session, please read the source code of the <b>set_active_interface.lua</b> script.</p>')

print('<br><h2>Interface information</h2>')
print('<p>The interface lua class provide a few methods to get information about the active network interface.</p>')

print('<h4>Get interface statistics information</h4>')
print('<p>Available examples:<ul>')
print('<li><a href="?interfacetype=show">Show statistics information</a>')
print('</ul></p>')
print('<p><b>Output:</b><p>')
print('<ul>')
if (interfacetype == "show") then
print('<pre><code>ifstats = interface.getStats()</code></pre>')
  ifstats = interface.getStats()
  for key, value in pairs(ifstats) do
   if (type(ifstats[key]) == "table") then
      printTable(ifstats[key],key)
   else
     print("<li>".. key.." = " ..value.."<br>")
   end
  end
end --if
print('</ul>')

print('<br><h2>Host information</h2>')
print('<p>The interface lua class provide a few methods to get information about the hosts.</p>')

print('<h4>Get hosts information</h4>')
print('<p>This is an example how to use the interface methods to get storage information. In order to extract all information about an host you can use the method "interface.getHostInfo(host_ip,vlan_id)". Please read the doxygen documentation for more information.</p>')

print('<p>Available examples:<ul>')
print('<li><a href="?hostinfotype=minimal_one_host">Minimal information of one host.</a>')
print('<li><a href="?hostinfotype=minimal_all_host">Minimal information of all host.</a>')
print('<li><a href="?hostinfotype=more_one_host">More information of one host.</a>')
print('<li><a href="?hostinfotype=more_all_host">More information of all host.</a>')
print('</ul></p>')

print('<p><b>Output:</b><p>')
print('<ul>')

if (hostinfotype == "minimal_one_host" ) or (hostinfotype == "minimal_all_host") then
  print('<pre><code>hosts = interface.getHosts()</code></pre>')
  hosts = interface.getHosts()
  for key, value in pairs(hosts) do
    print("<li> HostName: ".. key.."   -- Sent Byte + Received Byte: " .. value.."<br>")
    if (hostinfotype == "minimal_one_host" ) then break end
  end
end

if (hostinfotype == "more_one_host" ) or (hostinfotype == "more_all_host") then
  print('<pre><code>hosts = interface.getHostsInfo()</code></pre>')
    hosts = interface.getHostsInfo()

    for key, value in pairs(hosts) do
          print("<li> HostName: ".. key.."<br>")
          printTable(hosts[key],key)
          if (hostinfotype == "more_one_host") then break end
    end
end
print('</ul>')

random_host = nil
print('<br><h4>Export information in JSON format</h4>')
print('<p>This is an example how to use the interface methods to export information in json format.</p>')
print('<p>Available hosts:<ul>')
print('<pre><code>hosts_json = interface.getHosts()</code></pre>')
print('<li><a href="/lua/do_export_data.lua" target="_blank"> All hosts</a>')

hosts_json = interface.getHosts()
  for key, value in pairs(hosts_json) do
    random_host = key
    print('<li>'..key)
    print('<ul>')
    print('<li><a href="/lua/host_get_json.lua?host=' .. key..'" target="_blank"> All information</a>')
    print('<li><a href="/lua/get_host_activitymap.lua?host=' .. key..'" target="_blank"> Only Activity Map </a>')
    print('</ul>')
  end
print('</ul></p>')


print('<br><h2>Flow information</h2>')
print('<p>The interface lua class provide a few methods to get information about the flows.</p>')

print('<h4>Get flows information</h4>')
print('<p>This is an example how to use the interface methods to get flows information.</p>')

print('<p>Available examples:<ul>')
print('<li><a href="?flowtype=description">Flows description.</a>')
print('<li><a href="?flowtype=peers">Flow peers</a>')
-- print('<li><a href="?flowtype=more_one_host">More information of one host.</a>')
-- print('<li><a href="?flowtype=more_all_host">More information of all host.</a>')
print('</ul></p>')

print('<p><b>Output:</b><p>')

if (flowtype == "description" ) then
  print('<pre><code>flows_stats = interface.getFlowsInfo()\nprintTable(flows_stats)</code></pre>')
  flows_stats = interface.getFlowsInfo()
   printTable(flows_stats)
end

if (flowtype == "peers" ) then
  print('<pre><code>flows_stats = interface.getFlowPeers(random_host)\nprint(\'Host: \'..random_host..)\nprintTable(flows_peers,"Peers")</code></pre>')
  flows_peers = interface.getFlowPeers(random_host)
   print('Host: '..random_host.."<br>")
   printTable(flows_peers,"Peers")
end


print('<br><h4>TDB</h4>')
print('<p><ul>')
print('<li>findFlowByKey')
print('<li>findHost')
print('<li>getEndpoint')
print('<li>incrDrops')
print('<li>getAggregationsForHost')
print('<li>getAggregatedHostInfo')
print('<li>getAggregationFamilies')
print('<li>getNumAggregatedHosts')
print('<li>getNdpiProtoName')
print('<li>flushHostContacts')
print('<li>getNdpiProtoName')
print('<li>restoreHost')

print('</ul></p>')

print('</body></html>\n')


