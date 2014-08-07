--
-- (C) 2013-14 - ntop.org
--

-- ########################################################

function navigatedir(url, label, base, path, go_deep)
   local shown = false
   local to_skip = false
   --print("<li> <b>(d)</b> "..path.."  </li>\n")

   rrds = ntop.readdir(path)   
   for k,v in pairs(rrds) do
      if(v ~= nil) then
	 p = fixPath(path .. "/" .. v)
	 
	 if(ntop.isdir(p)) then
	    if(go_deep) then navigatedir(url, label.."/"..v, base, p) end
	 else
	    if(label == "*") then
	       to_skip = true
	    else
	       if(not(shown) and not(to_skip)) then
		  print('<li class="dropdown-submenu"><a tabindex="-1" href="#">'..label..'</a>\n<ul class="dropdown-menu">\n')
		  shown = true
	       end
	    end
	    
	    what = string.sub(path.."/"..v, string.len(base)+2)
	    
	    label = string.sub(v,  1, string.len(v)-4)
	    label = l4Label(string.gsub(label, "_", " "))

	    print("<li> <A HREF="..url..what..">"..label.."</A>  </li>\n")
	 end
      end
   end

   if(shown) then
      print('</ul></li>\n')
   end
end

-- ########################################################

function breakdownBar(sent, sentLabel, rcvd, rcvdLabel)
   if((sent+rcvd) > 0) then
      sent2rcvd = round((sent * 100) / (sent+rcvd), 0)
      print('<div class="progress"><div class="progress-bar progress-bar-warning" aria-valuenow="'.. sent2rcvd..'" aria-valuemin="0" aria-valuemax="100" style="width: ' .. sent2rcvd.. '%;">'..sentLabel)
      print('</div><div class="progress-bar progress-bar-info" aria-valuenow="'.. (100 -sent2rcvd)..'" aria-valuemin="0" aria-valuemax="100" style="width: ' .. (100-sent2rcvd) .. '%;">' .. rcvdLabel .. '</div></div>')
   else
      print('&nbsp;')
   end
end

-- ########################################################

function percentageBar(total, value, valueLabel)
   if(total > 0) then
      pctg = round((value * 100) / total, 0)
      print('<div class="progress"><div class="progress-bar progress-bar-warning" aria-valuenow="'.. pctg..'" aria-valuemin="0" aria-valuemax="100" style="width: ' .. pctg.. '%;">'..valueLabel)
      print('</div></div>')
   else
      print('&nbsp;')
   end
end

-- ########################################################

function getRRDName(ifid, host, rrdFile)
   rrdname = fixPath(dirs.workingdir .. "/" .. ifid .. "/rrd/")
   if(host ~= nil) then
     rrdname = rrdname .. host .. "/"
   end

   return(rrdname  .. rrdFile)
end

-- ########################################################

zoom_vals = {
   { "5m",  "now-300s", 60*5 },
   { "10m", "now-600s", 60*10 },
   { "1h",  "now-1h",   60*60*1 },
   { "3h",  "now-3h",   60*60*3 },
   { "6h",  "now-6h",   60*60*6 },
   { "12h", "now-12h",  60*60*12 },
   { "1d",  "now-1d",   60*60*24 },
   { "1w",  "now-1w",   60*60*24*7 },
   { "2w",  "now-2w",   60*60*24*14 },
   { "1m",  "now-1mon", 60*60*24*31 },
   { "6m",  "now-6mon", 60*60*24*31*6 },
   { "1y",  "now-1y",   60*60*24*366 }
}

-- ########################################################

function drawPeity(ifid, host, rrdFile, zoomLevel, selectedEpoch)
   rrdname = getRRDName(ifid, host, rrdFile)

   if(zoomLevel == nil) then
      zoomLevel = "1h"
   end

   nextZoomLevel = zoomLevel;
   epoch = tonumber(selectedEpoch);

   for k,v in ipairs(zoom_vals) do
      if(zoom_vals[k][1] == zoomLevel) then
         if (k > 1) then
           nextZoomLevel = zoom_vals[k-1][1]
         end
         if (epoch) then
           start_time = epoch - zoom_vals[k][3]/2
           end_time = epoch + zoom_vals[k][3]/2
         else
	   start_time = zoom_vals[k][2]
           end_time = "now"
         end
      end
   end

   if(ntop.exists(rrdname)) then
      --print("=> Found "..rrdname.."<p>\n")
      --print("=> Found ".. start_time .. "|" .. end_time .. "<p>\n")
      local fstart, fstep, fnames, fdata = ntop.rrd_fetch(rrdname, '--start', start_time, '--end', end_time, 'AVERAGE')
      local max_num_points = 512 -- This is to avoid having too many points and thus a fat graph
      local num_points_found = table.getn(fdata)
      local sample_rate = round(num_points_found / max_num_points)
      local num_points = 0
      local step = 1
      local series = {}


      if(sample_rate < 1) then
	 sample_rate = 1
      end

      -- print("=> "..num_points_found.."[".. sample_rate .."]["..fstart.."]<p>")

      id = 0
      num = 0
      total = 0
      sample_rate = sample_rate-1
      points = {}
      for i, v in ipairs(fdata) do
	 timestamp = fstart + (i-1)*fstep
	 num_points = num_points + 1
	 
	 local elemId = 1
	 for _, w in ipairs(v) do
	    if(w ~= w) then
	       -- This is a NaN
	       v = 0
	    else
	       v = tonumber(w)
	       if(v < 0) then
		  v = 0
	       end
	    end
	    
	    value = v*8 -- bps
	    
	    total = total + value
	    if(id == sample_rate) then
	       points[num] = round(value)..""
	       num = num+1
	       id = 0
	    else
	       id = id + 1
	    end
	    elemId = elemId + 1
	 end   
      end
  end

   print("<td class=\"text-right\">"..round(total).."</td><td> <span class=\"peity-line\">")
   for i=0,10 do
      if(i > 0) then print(",") end
      print(points[i])
   end
   print("</span>\n")
end

-- ########################################################

function drawRRD(ifid, host, rrdFile, zoomLevel, baseurl, show_timeseries, selectedEpoch, xInfoURL)
   dirs = ntop.getDirs()
   rrdname = getRRDName(ifid, host, rrdFile)
   names =  {}
   series = {}

   if(zoomLevel == nil) then
      zoomLevel = "1h"
   end

   nextZoomLevel = zoomLevel;
   epoch = tonumber(selectedEpoch);

   for k,v in ipairs(zoom_vals) do
      if(zoom_vals[k][1] == zoomLevel) then
         if (k > 1) then
	    nextZoomLevel = zoom_vals[k-1][1]
         end
         if (epoch) then
	    start_time = epoch - zoom_vals[k][3]/2
	    end_time = epoch + zoom_vals[k][3]/2
         else
	    start_time = zoom_vals[k][2]
	    end_time = "now"
         end
      end
   end

   local maxval_bits_time = 0
   local maxval_bits = 0
   local minval_bits = 0
   local minval_bits_time = 0
   local lastval_bits = 0
   local lastval_bits_time = 0
   local total_bytes = 0
   local num_points = 0
   local step = 1

   prefixLabel = l4Label(string.gsub(rrdFile, ".rrd", ""))

   if(prefixLabel == "bytes") then
      prefixLabel = "Traffic"
   end

   if(ntop.exists(rrdname)) then
      -- print("=> "..rrdname)
      -- print("=> Found ".. start_time .. "|" .. end_time .. "\n")
      local fstart, fstep, fnames, fdata = ntop.rrd_fetch(rrdname, '--start', start_time, '--end', end_time, 'AVERAGE')
      --print("=> here we go")
      local max_num_points = 600 -- This is to avoid having too many points and thus a fat graph
      local num_points_found = table.getn(fdata)
      local sample_rate = round(num_points_found / max_num_points)

      if(sample_rate < 1) then
	 sample_rate = 1
      end

      -- DEBUG
      --tprint(fdata, 1)
      
      step = fstep
      num = 0
      for i, n in ipairs(fnames) do
	 names[num] = prefixLabel
	 if(prefixLabel ~= firstToUpper(n)) then names[num] = names[num] .. " " .. firstToUpper(n) end
	 num = num + 1
	 --io.write(prefixLabel.."\n")
	 --print(num.."\n")
      end

      id = 0
      fend = 0
      sampling = 0
      --sample_rate = 1
      sample_rate = sample_rate-1
      accumulated = 0
      for i, v in ipairs(fdata) do
	 s = {}
	 s[0] = fstart + (i-1)*fstep
	 num_points = num_points + 1

	 local elemId = 1
	 for _, w in ipairs(v) do
	    if(w ~= w) then
	       -- This is a NaN
	       v = 0
	    else
	       --io.write(w.."\n")
	       v = tonumber(w)
	       if(v < 0) then
		  v = 0
	       end
	    end

	    if(v > 0) then
	       lastval_bits_time = s[0]
	       lastval_bits = v
	    end

	    s[elemId] = v*8 -- bps
	    -- if(s[elemId] > 0) then io.write("[".. elemId .. "]=" .. s[elemId] .."\n") end
	    elemId = elemId + 1
	 end

	 total_bytes = total_bytes + v*fstep
	 --if((v*fstep) > 0) then io.write(" | " .. (v*fstep) .." | [sampling: ".. sampling .. "/" .. sample_rate.."]\n") end

	 if(sampling == sample_rate) then
	    if(sample_rate > 0) then
	       s[1] = accumulated / sample_rate 
	    end
	    series[id] = s
	    id = id + 1
	    sampling = 0
	    accumulated = 0
	 else
	    accumulated = accumulated + s[1]
	    sampling = sampling + 1
	 end
      end

      for key, value in pairs(series) do
	 local t = 0

	 for elemId=0,(num-1) do
	    --io.write(key.."="..value[elemId+1].. "\n")
	    t = t + value[elemId+1] -- bps
	 end

	 t = t * step

	 if((minval_bits_time == 0) or (minval_bits >= t)) then
	    minval_bits_time = value[0]
	    minval_bits = t
	 end

	 if((maxval_bits_time == 0) or (maxval_bits <= t)) then
	    maxval_bits_time = value[0]
	    maxval_bits = t
	 end
      end

      print [[

<style>
#chart_container {
display: inline-block;
font-family: Arial, Helvetica, sans-serif;
}
#chart {
   float: left;
}
#legend {
   float: left;
   margin-left: 15px;
   color: black;
   background: white;
}
#y_axis {
   float: left;
   width: 40px;
}

</style>

<div>
]]


if(show_timeseries == 1) then
   print [[
<div class="btn-group">
  <button class="btn btn-default btn-sm dropdown-toggle" data-toggle="dropdown">Timeseries <span class="caret"></span></button>
  <ul class="dropdown-menu">
]]

   print('<li><a  href="'..baseurl .. '&rrd_file=' .. "bytes.rrd" .. '&graph_zoom=' .. zoomLevel .. '&epoch=' .. (selectedEpoch or '') .. '">'.. "Traffic" ..'</a></li>\n')
   print('<li class="divider"></li>\n')
   dirs = ntop.getDirs()
   p = dirs.workingdir .. "/" .. purifyInterfaceName(ifid) .. "/rrd/"
   if(host ~= nil) then 
      p = p .. host 
      go_deep = true
   else
      go_deep = false
   end
   d = fixPath(p)
   
   navigatedir(baseurl .. '&graph_zoom=' .. zoomLevel .. '&epoch=' .. (selectedEpoch or '')..'&rrd_file=', "*", d, d, go_deep)
   
   print [[
  </ul>
</div><!-- /btn-group -->
]]
end

print('&nbsp;Timeframe:  <div class="btn-group" data-toggle="buttons" id="graph_zoom">\n')

for k,v in ipairs(zoom_vals) do

   print('<label class="btn btn-link ')
   
   if(zoom_vals[k][1] == zoomLevel) then
      print("active")
   end
   print('">')
   print('<input type="radio" name="options" id="zoom_level_'..k..'" value="'..baseurl .. '&rrd_file=' .. rrdFile .. '&graph_zoom=' .. zoom_vals[k][1] .. '&epoch=' .. (selectedEpoch or '') ..'">'.. zoom_vals[k][1] ..'</input></label>\n')
end

print [[
</div>
</div>

<script>
   $('input:radio[id^=zoom_level_]').change( function() {
   window.open(this.value,'_self',false);
});
</script>

<br />
<p>
<div style="margin-left: 10px">
<div id="chart_container">
<p><font color=lightgray><small>NOTE: Click on the graph to zoom.</small></font>
   <div id="y_axis"></div>
   <div id="chart" style="margin-right: 10px"></div>

   <table style="border: 0">
   <tr><td><div id="legend"></div></td><td><div id="chart_legend"></div></td></tr>
   <tr><td colspan=2>

   <table class="table table-bordered table-striped">
   ]]


print('   <tr><th>' .. prefixLabel .. '</th><th>Time</th><th>Value</th></tr>\n')

if(string.contains(rrdFile, "num_")) then
   print('   <tr><th>Min</th><td>' .. os.date("%x %X", minval_bits_time) .. '</td><td>' .. formatValue(round(minval_bits/step), 1) .. '</td></tr>\n')
   print('   <tr><th>Max</th><td>' .. os.date("%x %X", maxval_bits_time) .. '</td><td>' .. formatValue(round(maxval_bits/step), 1) .. '</td></tr>\n')
   print('   <tr><th>Last</th><td>' .. os.date("%x %X", last_time) .. '</td><td>' .. formatValue(round(lastval_bits/step), 1) .. '</td></tr>\n')
   
   print('   <tr><th>Average</th><td colspan=2>' .. formatValue(round(total_bytes*8/(step*num_points), 2)) .. '</td></tr>\n')
   print('   <tr><th>Total Number</th><td colspan=2>' ..  formatValue(round(total_bytes)) .. '</td></tr>\n')
else
   print('   <tr><th>Min</th><td>' .. os.date("%x %X", minval_bits_time) .. '</td><td>' .. bitsToSize(minval_bits/step) .. '</td></tr>\n')
   print('   <tr><th>Max</th><td>' .. os.date("%x %X", maxval_bits_time) .. '</td><td>' .. bitsToSize(maxval_bits/step) .. '</td></tr>\n')
   print('   <tr><th>Last</th><td>' .. os.date("%x %X", last_time) .. '</td><td>' .. bitsToSize(lastval_bits/step)  .. '</td></tr>\n')
   print('   <tr><th>Average</th><td colspan=2>' .. bitsToSize(total_bytes*8/(step*num_points)) .. '</td></tr>\n')
   print('   <tr><th>Total Traffic</th><td colspan=2>' .. bytesToSize(total_bytes) .. '</td></tr>\n')
end

print('   <tr><th>Selection Time</th><td colspan=2><div id=when></div></td></tr>\n')
print('   <tr><th>Minute<br>Top Talkers</th><td colspan=2><div id=talkers></div></td></tr>\n')


print [[
   </table>
   </td></tr>
   </table>
</div>

</div>

<script>


var palette = new Rickshaw.Color.Palette();

var graph = new Rickshaw.Graph( {
				   element: document.getElementById("chart"),
				   width: 600,
				   height: 300,
				   renderer: 'area',
				   series: [

				]]

if(names ~= nil) then
   for elemId=0,(num-1) do
      if(elemId > 0) then
	 print ","
      end
      
      print ("{\nname: '".. names[elemId] .. "',\n")
      
      print("color: palette.color(),\ndata: [\n")
      
      n = 0
      for key, value in pairs(series) do
	 if(n > 0) then
	    print(",\n")
	 end
	 print ("\t{ x: "..  value[0] .. ", y: ".. value[elemId+1] .. " }")
	 n = n + 1
      end
      
      print("\n]}\n")
   end
end

print [[
				   ]
				} );

graph.render();

var chart_legend = document.querySelector('#chart_legend');


function fdate(when) {
      var epoch = when*1000;
      var d = new Date(epoch);

      return(d);
}

function fbits(bits) {
	var sizes = ['bps', 'Kbit', 'Mbit', 'Gbit', 'Tbit'];
	if (bits == 0) return 'n/a';
	var i = parseInt(Math.floor(Math.log(bits) / Math.log(1024)));
	return Math.round(bits / Math.pow(1024, i), 2) + ' ' + sizes[i];
}

function capitaliseFirstLetter(string)
{
   return string.charAt(0).toUpperCase() + string.slice(1);
}

/**
 * Convert number of bytes into human readable format
 *
 * @param integer bytes     Number of bytes to convert
 * @param integer precision Number of digits after the decimal separator
 * @return string
 */
   function formatBytes(bytes, precision)
      {
	 var kilobyte = 1024;
	 var megabyte = kilobyte * 1024;
	 var gigabyte = megabyte * 1024;
	 var terabyte = gigabyte * 1024;

	 if ((bytes >= 0) && (bytes < kilobyte)) {
	    return bytes + ' B';

	 } else if ((bytes >= kilobyte) && (bytes < megabyte)) {
	    return (bytes / kilobyte).toFixed(precision) + ' KB';

	 } else if ((bytes >= megabyte) && (bytes < gigabyte)) {
	    return (bytes / megabyte).toFixed(precision) + ' MB';

	 } else if ((bytes >= gigabyte) && (bytes < terabyte)) {
	    return (bytes / gigabyte).toFixed(precision) + ' GB';

	 } else if (bytes >= terabyte) {
	    return (bytes / terabyte).toFixed(precision) + ' TB';

	 } else {
	    return bytes + ' B';
	 }
      }

var Hover = Rickshaw.Class.create(Rickshaw.Graph.HoverDetail, {
    graph: graph,
    xFormatter: function(x) { return new Date( x * 1000 ); },
    yFormatter: function(bits) { return(fbits(bits)); },
    render: function(args) {
		var graph = this.graph;
		var points = args.points;
		var point = points.filter( function(p) { return p.active } ).shift();

		if (point.value.y === null) return;

		var formattedXValue = fdate(point.value.x); // point.formattedXValue;
		var formattedYValue = fbits(point.value.y); // point.formattedYValue;
		var infoHTML = "";
]]
if (xInfoURL) then
  print [[
		$.ajax({
			type: 'GET',
			url: ']]
  print(xInfoURL)
  print [[',
			data: { epoch: point.value.x },
			async: false,
			success: function(content) {
				var info = jQuery.parseJSON(content);
				infoHTML += "<ul>";
				$.each(info, function(i, n) {
				  infoHTML += "<li>"+capitaliseFirstLetter(i)+" [Avg Traffic/sec]<ol>";
				  var items = 0;
				  $.each(n, function(j, m) {
				    if (items < 3)
				      infoHTML += "<li><a href='host_details.lua?host="+m.label+"'>"+m.label+"</a> ("+fbits((m.value*8)/60)+")</li>";
				    items++;
				  });
				  infoHTML += "</ol></li>";
				});
				infoHTML += "</ul>";
			}
		});
    ]]
end
print [[
		this.element.innerHTML = '';
		this.element.style.left = graph.x(point.value.x) + 'px';

		/*var xLabel = document.createElement('div');
		xLabel.setAttribute("style", "opacity: 0.5; background-color: #EEEEEE; filter: alpha(opacity=0.5)");
		xLabel.className = 'x_label';
		xLabel.innerHTML = formattedXValue + infoHTML;
		this.element.appendChild(xLabel);
		*/
		$('#when').html(formattedXValue);
		$('#talkers').html(infoHTML);


		var item = document.createElement('div');

		item.className = 'item';
		item.innerHTML = this.formatter(point.series, point.value.x, point.value.y, formattedXValue, formattedYValue, point);
		item.style.top = this.graph.y(point.value.y0 + point.value.y) + 'px';
		this.element.appendChild(item);

		var dot = document.createElement('div');
		dot.className = 'dot';
		dot.style.top = item.style.top;
		dot.style.borderColor = point.series.color;
		this.element.appendChild(dot);

		if (point.active) {
			item.className = 'item active';
			dot.className = 'dot active';
		}

		this.show();

		if (typeof this.onRender == 'function') {
			this.onRender(args);
		}

		// Put the selected graph epoch into the legend
		//chart_legend.innerHTML = point.value.x; // Epoch

		this.selected_epoch = point.value.x;

		//event
	}
} );

var hover = new Hover( { graph: graph } );

var legend = new Rickshaw.Graph.Legend( {
					   graph: graph,
					   element: document.getElementById('legend')
					} );

//var axes = new Rickshaw.Graph.Axis.Time( { graph: graph } ); axes.render();

var yAxis = new Rickshaw.Graph.Axis.Y({
    graph: graph,
    tickFormat: Rickshaw.Fixtures.Number.formatKMBT
				      });

yAxis.render();

$("#chart").click(function() {
  if (hover.selected_epoch)
    window.location.href = ']]
print(baseurl .. '&rrd_file=' .. rrdFile .. '&graph_zoom=' .. nextZoomLevel .. '&epoch=')
print[['+hover.selected_epoch;
});

</script>

]]
else
  print("<div class=\"alert alert-danger\"><img src=/img/warning.png> File "..rrdname.." cannot be found</div>")

end
end

-- ########################################################

function create_rrd(name, ds)
   if(not(ntop.exists(name))) then
      if(enable_second_debug == 1) then io.write('Creating RRD ', name, '\n') end
      ntop.rrd_create(
	 name,
	 '--start', 'now',
	 '--step', '1',
	 'DS:' .. ds .. ':DERIVE:5:U:U',
	 'RRA:AVERAGE:0.5:1:86400',    -- raw: 1 day = 86400
	 'RRA:AVERAGE:0.5:3600:2400',  -- 1h resolution (3600 points)   2400 hours = 100 days
	 'RRA:AVERAGE:0.5:86400:365',    -- 1d resolution (86400 points)  365 days
	 'RRA:HWPREDICT:1440:0.1:0.0035:20')
   end
end

function createRRDcounter(path, verbose)
   if(not(ntop.exists(name))) then
      if(verbose) then print('Creating RRD ', name, '\n') end
      ntop.rrd_create(
	 name,
	 '--start', 'now',
	 '--step', '300',
	 'DS:sent:DERIVE:600:U:U',
	 'DS:rcvd:DERIVE:600:U:U',
	 'RRA:AVERAGE:0.5:1:7200',  -- raw: 1 day = 1 * 24 = 24 * 300 sec = 7200
	 'RRA:AVERAGE:0.5:12:2400',  -- 1h resolution (12 points)   2400 hours = 100 days
	 'RRA:AVERAGE:0.5:288:365',  -- 1d resolution (288 points)  365 days
	 'RRA:HWPREDICT:1440:0.1:0.0035:20')
   end
end

-- ########################################################

function createSingleRRDcounter(path, verbose)
   if(not(ntop.exists(path))) then
      if(verbose) then print('Creating RRD ', path, '\n') end
      ntop.rrd_create(
	 path,
	 '--start', 'now',
	 '--step', '300',
	 'DS:num:DERIVE:600:U:U',
	 'RRA:AVERAGE:0.5:1:7200',  -- raw: 1 day = 1 * 24 = 24 * 300 sec = 7200
	 'RRA:AVERAGE:0.5:12:2400',  -- 1h resolution (12 points)   2400 hours = 100 days
	 'RRA:AVERAGE:0.5:288:365',  -- 1d resolution (288 points)  365 days
	 'RRA:HWPREDICT:1440:0.1:0.0035:20')
   end
end

-- ########################################################

function dumpSingleTreeCounters(basedir, label, host, verbose)
   what = host[label]

   if(what ~= nil) then
      for k,v in pairs(what) do
	 for k1,v1 in pairs(v) do
	    -- print("-->"..k1.."/".. type(v1).."<--\n")

	    if(type(v1) == "table") then
	       for k2,v2 in pairs(v1) do

		  dname = fixPath(basedir.."/"..label.."/"..k.."/"..k1)

		  if(not(ntop.exists(dname))) then
		     ntop.mkdir(dname)
		  end

		  fname = dname..fixPath("/"..k2..".rrd")
		  createSingleRRDcounter(fname, verbose)
		  ntop.rrd_update(fname, "N:"..v2)
		  if(verbose) then print("\t"..fname.."\n") end
	       end
	    else
	       dname = fixPath(basedir.."/"..label.."/"..k)

	       if(not(ntop.exists(dname))) then
		  ntop.mkdir(dname)
	       end

	       fname = dname..fixPath("/"..k1..".rrd")
	       createSingleRRDcounter(fname, verbose)
	       ntop.rrd_update(fname, "N:"..v1)
	       if(verbose) then print("\t"..fname.."\n") end
	    end
	 end
      end
   end
end
