// Get current sensor readings when the page loads  
window.addEventListener('load', getReadings);

// Create Temperature Chart
var chartT = new Highcharts.Chart({
  time:{ 
    useUTC: false
  },
  chart:{ 
    renderTo:'chart-temperature' 
  },
  series: [
    {
      name: 'BME280'
    }
  ],
  title: { 
    text: undefined
  },
  plotOptions: {
    line: { 
      animation: false,
      dataLabels: { 
        enabled: true 
      }
    }
  },
  xAxis: {
    type: 'datetime',
    dateTimeLabelFormats: { second: '%H:%M:%S' }
  },
  yAxis: {
    title: { 
      text: 'Temperature Celsius Degrees' 
    }
  },
  credits: { 
    enabled: true 
  }
});
  
// Create Humidity Chart
var chartH = new Highcharts.Chart({
  time:{ 
    useUTC: false
  },
  chart:{ 
    renderTo:'chart-humidity' 
  },
  series: [{
    name: 'BME280'
  }],
  title: { 
    text: undefined
  },    
  plotOptions: {
    line: { 
      animation: false,
      dataLabels: { 
        enabled: true 
      }
    },
    series: { 
      color: '#50b8b4' 
    }
  },
  xAxis: {
    type: 'datetime',
    dateTimeLabelFormats: { second: '%H:%M:%S' }
  },
  yAxis: {
    title: { 
      text: 'Humidity (%)' 
    }
  },
  credits: { 
    enabled: false 
  }
});

//Plot temperature in the temperature chart
function plotTemperature(value) {
  var x = (new Date()).getTime()
  var y = Number(value);
  if(chartT.series[0].data.length > 40) {
    chartT.series[0].addPoint([x, y], true, true, true);
  } else {
    chartT.series[0].addPoint([x, y], true, false, true);
  }
}

//Plot humidity in the humidity chart
function plotHumidity(value) {
  var x = (new Date()).getTime()
  var y = Number(value);
  if(chartH.series[0].data.length > 40) {
    chartH.series[0].addPoint([x, y], true, true, true);
  } else {
    chartH.series[0].addPoint([x, y], true, false, true);
  }
}

// Function to get current readings on the webpage when it loads for the first time
function getReadings(){
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var myObj = JSON.parse(this.responseText);
      console.log(myObj);
      var temp = myObj.temperature;
      var hum = myObj.humidity;
      plotTemperature(temp);
      plotHumidity(hum);
    }
  }; 
  xhr.open("GET", "/readings", true);
  xhr.send();
}

if (!!window.EventSource) {
  var source = new EventSource('/events');
  
  source.addEventListener('open', function(e) {
    console.log("Events Connected");
  }, false);

  source.addEventListener('error', function(e) {
    if (e.target.readyState != EventSource.OPEN) {
      console.log("Events Disconnected");
    }
  }, false);
  
  source.addEventListener('message', function(e) {
    console.log("message", e.data);
  }, false);
  
  source.addEventListener('new_readings', function(e) {
    console.log("new_readings", e.data);
    var myObj = JSON.parse(e.data);
    console.log(myObj);
    plotTemperature(myObj.temperature);
    plotHumidity(myObj.humidity);
  }, false);
}
