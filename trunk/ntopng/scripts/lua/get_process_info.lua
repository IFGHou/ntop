--
-- (C) 2014 - ntop.org
--

dirs = ntop.getDirs()
package.path = dirs.installdir .. "/scripts/lua/modules/?.lua;" .. package.path

require "lua_utils"
require "flow_utils"

sendHTTPHeader('text/html')

ntop.dumpFile(dirs.installdir .. "/httpdocs/inc/header.inc")

page = _GET["page"]
if(page == nil) then page = "Protocols" end
dofile(dirs.installdir .. "/scripts/lua/inc/menu.lua")

pid_key = _GET["pid"]
name_key = _GET["name"]
host_key = _GET["host"]
application = _GET["application"]

general_process = 0

if((pid_key == nil) and (name_key == nil))then
   print("<div class=\"alert alert-error\"><img src=/img/warning.png> Missing pid name</div>")
else
  if ((name_key ~= nil) and (pid_key == nil) and (host_key == nil)) then
    general_process = 1
  end
  -- Prepare displayed value
  if (pid_key ~= nil) then
   flows = interface.findPidFlows(tonumber(pid_key))
   err_label = "PID"
   err_val = pid_key
  elseif (name_key ~= nil) then
   flows = interface.findNameFlows(name_key)
   err = "Name"
   err_val = name_key
  end
   
   if(flows == nil) then
      print("<div class=\"alert alert-error\"><img src=/img/warning.png> Unknown "..err.." "..err_val..": no traffic detected for this process, or process terminated.</div>")
  else
   print [[
	    <div class="bs-docs-example">
            <div class="navbar">
	    <div class="navbar-inner">
	    <ul class="nav"> ]]

if(pid_key ~= nil)then
   print [[ <li><a href="#">Pid: ]] print(pid_key) if(host_key ~= nill) then print(" - IP: "..host_key) end print [[ </a></li>]]
elseif (name_key ~= nil)then
    print [[ <li><a href="#">Name: ]] print(name_key) if(host_key ~= nill) then print(" - IP: "..host_key) end print [[ </a></li>]]
end

if(page == "Protocols") then active=' class="active"' else active = "" end

if (pid_key ~= nil) then
  print('<li'..active..'><a href="?pid='.. pid_key) if(host_key ~= nill) then print("&host="..host_key) end print('&page=Protocols">Protocols</a></li>\n')
  elseif (name_key ~= nil) then
   print('<li'..active..'><a href="?name='.. name_key) if(host_key ~= nill) then print("&host="..host_key) end print('&page=Protocols">Protocols</a></li>\n')
  end

if (general_process == 1) then
  if(page == "Hosts") then active=' class="active"' else active = "" end
  if (pid_key ~= nil) then
    print('<li'..active..'><a href="?pid='.. pid_key) if(host_key ~= nill) then print("&host="..host_key) end print('&page=Hosts">Hosts</a></li>\n')
  elseif (name_key ~= nil) then
   print('<li'..active..'><a href="?name='.. name_key) if(host_key ~= nill) then print("&host="..host_key) end print('&page=Hosts">Hosts</a></li>\n')
  end
end

if(page == "Flows") then active=' class="active"' else active = "" end
if (pid_key ~= nil) then
 print('<li'..active..'><a href="?pid='.. pid_key) if(host_key ~= nill) then print("&host="..host_key) end print('&page=Flows">Flows</a></li>\n')
  elseif (name_key ~= nil) then
   print('<li'..active..'><a href="?name='.. name_key) if(host_key ~= nill) then print("&host="..host_key) end print('&page=Flows">Flows</a></li>\n')
  end


-- End Tab Menu

print('</ul>\n\t</div>\n\t</div>\n')


if(page == "Protocols") then

print [[
  <br>
  <!-- Left Tab -->
  <div class="tabbable tabs-left">

    <ul class="nav nav-tabs">
]]

print [[<li class="active"><a href="#l7" data-toggle="tab">L7 Protocols</a></li> ]]

print [[<li><a href="#l4" data-toggle="tab">L4 Protocols</a></li>]]

print [[
    </ul>
    
      <!-- Tab content-->
      <div class="tab-content">
]]

print [[
        <div class="tab-pane active" id="l7">
          <table class="table table-bordered">
            <tr>
              <th class="text-center">Top L7 Protocols</th>
              <td><div class="pie-chart" id="topL7"></div></td>
          </tr>
          </table>
        </div> <!-- Tab l7-->
]]

print [[

        <div class="tab-pane" id="l4">
          <table class="table table-bordered">
            <tr>
              <th class="text-center">Top L4 Protocols</th>
              <td><div class="pie-chart" id="topL4"></div></td>
          </tr>
          </table>
        </div> <!-- Tab l4-->
]]

print [[
      </div> <!-- End Tab content-->
    </div> <!-- End Left Tab -->
     </table>
]]

 print [[
     
<script type='text/javascript'>
window.onload=function() {
   var refresh = 3000 /* ms */;
]]
if(pid_key ~= nil)then
   print [[ 
  do_pie("#topL7", '/lua/pid_stats.lua', { "pid": ]] print(pid_key) print [[, "mode": "l7" ]] 
if (host_key ~= nil) then print(", host: \""..host_key.."\"") end
print [[
 }, "", refresh);
 do_pie("#topL4", '/lua/pid_stats.lua', { "pid": ]] print(pid_key) print [[, "mode": "l4"  ]] 
if (host_key ~= nil) then print(", host: \""..host_key.."\"") end
print [[
 }, "", refresh);
  ]]
elseif (name_key ~= nil)then
    print [[ 
    do_pie("#topL7", '/lua/pid_stats.lua', { "name": "]] print(name_key) print [[", "mode": "l7" ]] 
if (host_key ~= nil) then print(", host: \""..host_key.."\"") end
print [[
 }, "", refresh);
    do_pie("#topL4", '/lua/pid_stats.lua', { "name": "]] print(name_key) print [[", "mode": "l4"  ]] 
if (host_key ~= nil) then print(", host: \""..host_key.."\"") end
print [[
 }, "", refresh); ]]
end
print [[	    
}
</script>
]]


elseif(page == "Flows") then

stats = interface.getNdpiStats()
num_param = 0

print [[
      <hr>
      <div id="table-hosts"></div>
   <script>
   $("#table-hosts").datatable({
      url: "/lua/get_flows_data.lua]] 
if(application ~= nil) then
   print("?application="..application)
   num_param = num_param + 1
end

if(pid_key ~= nil) then
  if (num_param > 0) then
    print("&")
  else
    print("?")
  end
   print("pid="..pid_key)
   num_param = num_param + 1
end

if(name_key ~= nil) then
  if (num_param > 0) then
    print("&")
  else
    print("?")
  end
  print("name="..name_key)
  num_param = num_param + 1
end

if(host_key ~= nil) then
  if (num_param > 0) then
    print("&")
  else
    print("?")
  end
  print("host="..host_key)
  num_param = num_param + 1
end



print [[",
         showPagination: true,
         buttons: [ '<div class="btn-group"><button class="btn dropdown-toggle" data-toggle="dropdown">Applications<span class="caret"></span></button> <ul class="dropdown-menu">]]

if (pid_key ~= nil) then
  print('<li><a href="/lua/get_process_info.lua?pid='.. pid_key) if(host_key ~= nill) then print("&host="..host_key) end print('&page=Flows">All Proto</a></li>')
end
if (name_key ~= nil) then
  print('<li><a href="/lua/get_process_info.lua?name='.. name_key) if(host_key ~= nill) then print("&host="..host_key) end print('&page=Flows">All Proto</a></li>')
end

for key, value in pairsByKeys(stats["ndpi"], asc) do
   class_active = ''
   if(key == application) then
      class_active = ' class="active"'
   end


   if (pid_key ~= nil) then
    print('<li '..class_active..'><a href="/lua/get_process_info.lua?pid='.. pid_key) if(host_key ~= nill) then print("&host="..host_key) end print('&page=Flows&application=' .. key..'">'..key..'</a></li>')
    end

    if (name_key ~= nil) then
    print('<li '..class_active..'><a href="/lua/get_process_info.lua?name='.. name_key) if(host_key ~= nill) then print("&host="..host_key) end print('&page=Flows&application=' .. key..'">'..key..'</a></li>')
    end


end


print("</ul> </div>' ],\n")


ntop.dumpFile(dirs.installdir .. "/httpdocs/inc/sflows_stats_top.inc")

prefs = ntop.getPrefs()

ntop.dumpFile(dirs.installdir .. "/httpdocs/inc/sflows_stats_bottom.inc")


elseif(page == "Hosts") then

print [[
  <br>
  <!-- Left Tab -->
  <div class="tabbable tabs-left">

    <ul class="nav nav-tabs">
]]

print [[<li class="active"><a href="#topHost" data-toggle="tab">Top Hosts</a></li> ]]

print [[
    </ul>
    
      <!-- Tab content-->
      <div class="tab-content">
]]

print [[
        <div class="tab-pane active" id="topHost">
          <table class="table table-bordered">
            <tr>
              <th class="text-center span3">Top Hosts Traffic</th>
              <td><div class="pie-chart" id="topHosts"></div></td>
          </tr>
          </table>
        </div> <!-- Tab l7-->
]]


print [[
      </div> <!-- End Tab content-->
    </div> <!-- End Left Tab -->
     </table>
]]

 print [[
<script type='text/javascript'>
window.onload=function() {
   var refresh = 3000 /* ms */;
]]

if(pid_key ~= nil)then
  print [[ 
    do_pie("#topHosts", '/lua/pid_stats.lua', { "pid": ]] print(pid_key) print [[", "mode": "host" }, "", refresh);
  ]]
elseif (name_key ~= nil)then
  print [[ 
    do_pie("#topHosts", '/lua/pid_stats.lua', { "name": "]] print(name_key) print [[", "mode": "host" }, "", refresh); 
    ]]
end

print [[      
}
</script>
]]

end -- If page

end -- Error one
end -- Error two

dofile(dirs.installdir .. "/scripts/lua/inc/footer.lua")
