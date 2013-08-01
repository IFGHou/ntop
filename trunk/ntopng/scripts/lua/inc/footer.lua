--
-- (C) 2013 - ntop.org
--

require "os"

print [[ <hr>

      <div class="footer">


<div class="row-fluid show-grid">
	 <div class="span4">&copy; 1998-]]

print(os.date("%Y"))
print [[ - <A HREF="http://www.ntop.org">ntop.org</A> <br><font color=lightgray>Generated by ntopng ]]

info = ntop.getInfo()

print ("v."..info["version"].." for user ")
print(_SESSION["user"])
print [[</font></div>
  <div class="span1"> <A href="/lua/if_stats.lua"><span class="network-load-chart">0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0</span></a></div>
  <div class="span6"><div id="network-load"></div></div></div>


</div> <!-- /row --> 
</div><!-- /footer --> 


<script>
  // Updating charts.
var updatingChart = $(".network-load-chart").peity("line", { width: 64 })
var prev_bytes   = 0;
var prev_packets = 0;
var prev_epoch   = 0;

function addCommas(nStr)
{
   nStr += '';
   x = nStr.split('.');
   x1 = x[0];
   x2 = x.length > 1 ? '.' + x[1] : '';
   var rgx = /(\d+)(\d{3})/;
   while (rgx.test(x1)) {
      x1 = x1.replace(rgx, '$1' + ',' + '$2');
   }
   return x1 + x2;
}

function bytesToVolume(bytes) {
      var sizes = ['Bytes', 'KB', 'MB', 'GB', 'TB'];
      if (bytes == 0) return '0 Bytes';
      var i = parseInt(Math.floor(Math.log(bytes) / Math.log(1024)));
      return (bytes / Math.pow(1024, i)).toFixed(2) + ' ' + sizes[i];
   };


function bytesToSize(bytes) {
      var sizes = ['bps', 'Kbps', 'Mbps', 'Gbps', 'Tbps'];
      if (bytes == 0) return '0 bps';
      var i = parseInt(Math.floor(Math.log(bytes) / Math.log(1024)));
      if (i == 0) return bytes + ' ' + sizes[i]; 
      return (bytes / Math.pow(1024, i)).toFixed(2) + ' ' + sizes[i];
   };



setInterval(function() {
		  $.ajax({
			    type: 'GET',
			    url: '/lua/network_load.lua',
			    data: { if: "any" },
			    success: function(content) {
					   var rsp = jQuery.parseJSON(content);
					   if(prev_bytes > 0) {
					   var values = updatingChart.text().split(",")
   					   var bytes_diff = rsp.bytes-prev_bytes;
   					   var packets_diff = rsp.packets-prev_packets;
					   var epoch_diff = rsp.epoch - prev_epoch;

					   if(epoch_diff == 0) {
					      epoch_diff = 1;
					   }

					   if(bytes_diff > 0) {
					      values.shift()
					      values.push(bytes_diff)
					      
					      updatingChart.text(values.join(",")).change()
					   }

					   pps = packets_diff / epoch_diff;

					   $('#network-load').html(""+bytesToSize((bytes_diff*8)/epoch_diff)+" [" + addCommas(pps) + " pps]["+addCommas(rsp.num_hosts)+" hosts]["+addCommas(rsp.num_flows)+" flows][uptime "+rsp.uptime+"]");
   					}
					   prev_bytes = rsp.bytes;
					   prev_packets  = rsp.packets;
					   prev_epoch = rsp.epoch;
					}
				     });
			 }, 1000)

</script>

    </div> <!-- /container -->

  </body>
	 </html> ]]
