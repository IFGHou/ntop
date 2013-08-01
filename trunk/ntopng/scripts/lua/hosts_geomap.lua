--
-- (C) 2013 - ntop.org
--

dirs = ntop.getDirs()
package.path = dirs.installdir .. "/scripts/lua/modules/?.lua;" .. package.path

require "lua_utils"

sendHTTPHeader('text/html')


ntop.dumpFile(dirs.installdir .. "/httpdocs/inc/header.inc")
active_page = "hosts"
dofile(dirs.installdir .. "/scripts/lua/inc/menu.lua")

print [[

     <style type="text/css">
     #map-canvas { width: 800px; height: 600px; }
   </style>

<hr>
<h2>Hosts GeoMap</H2>

    <script src="https://maps.googleapis.com/maps/api/js?v=3.exp&sensor=false"></script>
<div class="container-fluid">
  <div class="row-fluid">
    <div class="span8">
      <div id="map-canvas"></div>
</div>
</div>
</div>

    <script type="text/javascript" src="/js/googleMapJson.js" ></script>



</body>
</html>
]]

dofile(dirs.installdir .. "/scripts/lua/inc/footer.lua")