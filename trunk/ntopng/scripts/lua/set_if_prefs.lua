--
-- (C) 2013 - ntop.org
--

dirs = ntop.getDirs()
package.path = dirs.installdir .. "/scripts/lua/modules/?.lua;" .. package.path

require "lua_utils"
require "flow_utils"

sendHTTPHeader('text/html')
ntop.dumpFile(dirs.installdir .. "/httpdocs/inc/header.inc")
dofile(dirs.installdir .. "/scripts/lua/inc/menu.lua")


print('<hr><h2>'..ifname..' Preferences</H2>\n')

key = 'ntopng.prefs.'..ifname..'.name'
if(_GET["ifName"] ~= nil) then
   custom_name = tostring(_GET["ifName"])
   ntop.setCache(key, custom_name)
else
   custom_name = ntop.getCache(key)
end

key = 'ntopng.prefs.'..ifname..'.speed'
if(_GET["ifSpeed"] ~= nil) then
   ifSpeed = _GET["ifSpeed"]
   if(ifSpeed ~= nil) then ifSpeed = tonumber(ifSpeed) end
   if((ifSpeed == nil) or (ifSpeed > 10000)) then
      ifSpeed = 10000
   end
   
   ntop.setCache(key, tostring(ifSpeed))
else
   ifSpeed = ntop.getCache(key)

   if((ifSpeed ~= nil) and (ifSpeed ~= "")) then
      ifSpeed = tonumber(ifSpeed)  
   else
      ifSpeed = 1000
   end
end

ifSpeed = math.floor(ifSpeed+0.5)



print [[

<form class="form-horizontal" method="GET">


<div class="control-group">
    <label class="control-label" for="ifSpeed">Interface Speed (Mbit):</label>
    <div class="controls">
 <input type="number" min="1" max="10000" step="1" name="ifSpeed" id="IfSpeed" value="]] print(ifSpeed) print [["/>
    </div>
</div>

<div class="control-group">
    <label class="control-label" for="ifName">Custom Name:</label>
    <div class="controls">
 <input type="text" name="ifName" id="IfName" value="]] print(custom_name) print [["/>
    </div>
</div>


<div class="control-group">
<div class="controls">
<button type="submit" class="btn btn-primary">Save</button> <button class="btn" type="reset">Reset Form</button>
</div>
</div>

</form>
]]



dofile(dirs.installdir .. "/scripts/lua/inc/footer.lua")