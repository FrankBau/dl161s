<!DOCTYPE html>
<head>
  <meta charset="utf-8"/>
  <script type="text/javascript"  src="dygraph-combined.js"></script>
</head>
<body>

    <form id="myForm" onchange="myFunction()">
      <div id="myDropdown" class="dropdown-content">
        <select id="mySelect">
	  <?php
		$files = scandir ( "/www/pages/logs", SCANDIR_SORT_DESCENDING );
		foreach ($files as $file) {
		  if( ($file != "..") && ($file != ".") ) {
			echo "<option>$file</option>\n";
		  }
		}
	  ?>
        </select>
      </div>
   </form>

   <div id="graphdiv1" style="width: 100%"></div>
 
   <input type="button" value="Zoom letzte Stunde" onclick="zoom1hr()"/>
   <input type="button" value="Zoom letzte 10 Minuten" onclick="zoom10min()"/>
   <input type="button" value="Zoom all" onclick="unzoom()"/>
   <input type=button onClick="location.href='./logs'" value='Log Files'>   

   <script type="text/javascript">
	var e = document.getElementById("mySelect");
	chart = new Dygraph(document.getElementById("graphdiv1"),
		"logs/" + e.value,
		{
			title: "Schallmessung " + e.value,
			delimiter: ";",
			labels: [ "Zeitstempel", "dB(A)" ],
			legend: "follow",
			showRangeSelector: true,
			interactionModel: Dygraph.defaultInteractionModel,
			//dateWindow: [Date.now()-60*60*1000, Date.now()],
			xlabel: "Uhrzeit",
			ylabel: "Schallpegel dB(A)",
			valueRange: [20.0, 130.0],
			"dB(A)": {
				color: "rgb(255,0,0)"
			}
		}
	);

       function myFunction() {
                var e = document.getElementById("mySelect");
                chart.updateOptions(
			{
				file: "logs/" + e.value,
				title: "Schallmessung " + e.value,
				dateWindow: null,
			}
		);
        }

	function zoom10min() {
		chart.updateOptions({dateWindow: null });
		var Ende = chart.xAxisRange()[1];
		chart.updateOptions({
			dateWindow: [(Ende-10*60*1000), Ende]
		}
		);
	}  

	function zoom1hr() {
		chart.updateOptions({dateWindow: null });
		var Ende = chart.xAxisRange()[1];
		chart.updateOptions({
			dateWindow: [(Ende-60*60*1000), Ende]
		});
	}

	function unzoom() {
		chart.updateOptions({
			dateWindow: null,
			//valueRange: null
		});
	}
    </script>
  </body>
</html>

