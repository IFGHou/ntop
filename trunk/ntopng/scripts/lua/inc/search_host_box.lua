--
-- (C) 2013 - ntop.org
--

print [[
	 <li><form action="/lua/host_details.lua">
	 <input id="search_typeahead" type="text" name="host" class="search-query span2" placeholder="Search Host" data-provide="typeahead">
	 </form>
	 </li>

	 <script type='text/javascript'>
	 $('#search_typeahead').typeahead({
	     source: function (query, process) {
	             return $.get('/lua/find_host.lua', { query: query }, function (data) {
		                 return process(data.results);
		});
	 }
	});
	</script>

   ]]

