	
	       showPagination: true,
	        columns: [
			     {
			     title: "Name",
				 field: "column_name",
				 sortable: true,
	 	             css: { 
			        textAlign: 'left'
			     }
				 },
           {
           title: "Source Id",
         field: "column_vlan",
         sortable: true,
                 css: { 
              textAlign: 'center'
           }
         },
			     {
			     title: "Flows Count",
				 field: "column_count",
				 sortable: true,
	 	             css: { 
			        textAlign: 'center'
			     }

				 },			     
			     {
			     title: "Active Since",
				 field: "column_duration",
				 sortable: true,
	 	             css: { 
			        textAlign: 'center'
			     }

				 },			     
			     {
			     title: "Traffic Sent",
				 field: "column_bytes_sent",
				 sortable: true,
	 	             css: { 
			        textAlign: 'right'
			     }
				 },
			     {
			     title: "Traffic Rcvd",
				 field: "column_bytes_rcvd",
				 sortable: true,
	 	             css: { 
			        textAlign: 'right'
			     }
				 }
			     ]
	       });


       </script>

