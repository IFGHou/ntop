--
-- (C) 2013-14 - ntop.org
--

dirs = ntop.getDirs()
package.path = dirs.installdir .. "/scripts/lua/modules/?.lua;" .. package.path

require "lua_utils"

sendHTTPHeader('text/html; charset=iso-8859-1')
ntop.dumpFile(dirs.installdir .. "/httpdocs/inc/header.inc")

if(_GET["csrf"] ~= nil) then
   if(_GET["id_to_delete"] ~= nil) then
      if(_GET["id_to_delete"] == "__all__") then
	 ntop.flushAllQueuedAlerts()
	 print("")
      else
	 ntop.deleteQueuedAlert(tonumber(_GET["id_to_delete"]))
      end
   end
end

active_page = "alerts"
dofile(dirs.installdir .. "/scripts/lua/inc/menu.lua")

print [[
      <hr>
      <div id="table-alerts"></div>
	 <script>
	 $("#table-alerts").datatable({
			url: "/lua/get_alerts_data.lua",
	       showPagination: true,
]]

if(_GET["currentPage"] ~= nil) then print("currentPage: ".._GET["currentPage"]..",\n") end
if(_GET["perPage"] ~= nil)     then print("perPage: ".._GET["perPage"]..",\n") end

print [[
	        title: "Queued Alerts",
      columns: [
	 {
	    title: "Action",
	    field: "column_key",
	    css: { 
	       textAlign: 'center'
	    }
	 },
	 
	 {
	    title: "Date",
	    field: "column_date",
	    css: { 
	       textAlign: 'center'
	    }
	 },
	 {
	    title: "Severity",
	    field: "column_severity",
	    css: { 
	       textAlign: 'center'
	    }
	 },
	 
	 {
	    title: "Type",
	    field: "column_type",
	    css: { 
	       textAlign: 'center'
	    }
	 },

	 {
	    title: "Description",
	    field: "column_msg",
	    css: { 
	       textAlign: 'left'
	    }
	 }
      ]
   });
   </script>
	      ]]


if(ntop.getNumQueuedAlerts() > 0) then
   print [[

<a href="#myModal" role="button" class="btn btn-default" data-toggle="modal"><i type="submit" class="fa fa-trash-o"></i> Purge All Alerts</button></a>
 
<!-- Modal -->
<div class="modal fade" id="myModal" tabindex="-1" role="dialog" aria-labelledby="myModalLabel" aria-hidden="true">
  <div class="modal-dialog">
    <div class="modal-content">
      <div class="modal-header">
    <button type="button" class="close" data-dismiss="modal" aria-hidden="true">X</button>
    <h3 id="myModalLabel">Confirm Action</h3>
  </div>
  <div class="modal-body">
    <p>Do you really want to purge all alerts?</p>
  </div>
  <div class="modal-footer">

    <form class=form-inline style="margin-bottom: 0px;" method=get action="#"><input type=hidden name=id_to_delete value="__all__">
      ]]

print('<input id="csrf" name="csrf" type="hidden" value="'..ntop.getRandomCSRFValue()..'" />\n')

print [[
    <button class="btn btn-default" data-dismiss="modal" aria-hidden="true">Close</button>
    <button class="btn btn-primary" type="submit">Purge All</button>
</form>
  </div>
  </div>
</div>
</div>

      ]]
end

dofile(dirs.installdir .. "/scripts/lua/inc/footer.lua")