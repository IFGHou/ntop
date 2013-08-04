--
-- (C) 2013 - ntop.org
--

dirs = ntop.getDirs()
package.path = dirs.installdir .. "/scripts/lua/modules/?.lua;" .. package.path

require "lua_utils"

sendHTTPHeader('text/html')

ifname = _GET["if"]
interface.find("any")
peers = interface.getFlowPeers()



print [[

{"center":[0, 0],
"objects":
	[

     ]]

maxval = 0
for key, values in pairs(peers) do
   t = values["sent"]+values["rcvd"]

   if(t > maxval) then maxval = t end
end

min_threshold = .5 --  0.5%
max_num = 100
num = 0
for key, values in pairs(peers) do
      t = values["sent"]+values["rcvd"]
      pctg = (t*100)/maxval

      if(pctg >= min_threshold) then 
	 if(num > 0) then print(",") end
	 print('{\n"host":\n[	\n{\n')
	 print('"lat": '..values["client.latitude"]..',\n')
	 print('"lng": '..values["client.longitude"]..',\n')

	 print('"html": "')
	 if((values["client.city"] ~= nil) and (values["client.city"] ~= "")) then
	    print('City: '..values["client.city"])
	 end

	 if((values["client.country"] ~= nil) and (values["client.country"] ~= "")) then
	    print(" <img src='/img/blank.gif' class='flag flag-".. string.lower(values["client.country"]) .."'>")
	 end
	 print('",\n')

	 print('"name": "'..values["client"]..'"\n')
	 print('},\n{\n')
	 print('"lat": '..values["server.latitude"]..',\n')
	 print('"lng": '..values["server.longitude"]..',\n')
	 
	 print('"html": "')
	 if((values["server.city"] ~= nil) and (values["server.city"] ~= "")) then
	    print('City: '..values["server.city"])
	 end
	 if((values["server.country"] ~= nil) and (values["server.country"] ~= "")) then
	    print(" <img src='/img/blank.gif' class='flag flag-".. string.lower(values["server.country"]) .."'>")
	 end
	 print('",\n')


	 print('"name": "'..values["server"]..'"\n')
	 print('}\n],\n"flusso": '.. pctg..',"html":"Flow '.. key .. '"\n')
	 print('}\n')
	 num = num + 1
	 
	 if(num > max_num) then break end
      end
end


print [[
	]
}

]]