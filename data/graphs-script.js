function updateChart(chart, newData) {
    chart.data.labels.push('');
    chart.data.datasets[0].data.push(newData);
  
    if (chart.data.labels.length > 15) {
      chart.data.labels.shift();
      chart.data.datasets[0].data.shift();
    }
  
    var maxData = Math.max(...chart.data.datasets[0].data);
    var minData = Math.min(...chart.data.datasets[0].data);
    var dataRange = maxData - minData;
    var tickPadding = dataRange * 0.01;
  
    chart.options.scales.yAxes[0].ticks.min = Math.floor(minData - tickPadding);
    chart.options.scales.yAxes[0].ticks.max = Math.ceil(maxData + tickPadding);
  
    chart.update();
  }
  
  function createChart(ctx, label, color) {
    return new Chart(ctx, {
      type: "line",
      data: {
        labels: [],
        datasets: [{
          fill: false,
          lineTension: 0,
          backgroundColor: color,
          borderColor: color,
          label: label,
          data: []
        }]
      },
      options: {
        legend: {
          display: true,
          labels: {
            fontSize: 14,
            fontWeight: 'bold'
          }
        },
        scales: {
          yAxes: [{
            ticks: {
              precision: 2,
              callback: function (value, index, values) {
                return value.toFixed(2);
              }
            }
          }],
          xAxes: [{
            ticks: {
              fontSize: 14
            }
          }]
        }
      }
    });
  }
  
  function getStoredChartData(chartName) {
    var storedData = localStorage.getItem(chartName);
    return storedData ? JSON.parse(storedData) : { labels: [], data: [] };
  }
  
  function setStoredChartData(chartName, data) {
    localStorage.setItem(chartName, JSON.stringify(data));
  }
  
  var temperatureCtx = document.getElementById('temperatureChart').getContext('2d');
  var temperatureData = getStoredChartData('temperatureChart');
  var temperatureChart = createChart(temperatureCtx, 'Temperature', "rgba(255, 99, 132, 1)");
  temperatureChart.data.labels = temperatureData.labels;
  temperatureChart.data.datasets[0].data = temperatureData.data;
  
  var humidityCtx = document.getElementById('humidityChart').getContext('2d');
  var humidityData = getStoredChartData('humidityChart');
  var humidityChart = createChart(humidityCtx, 'Humidity', "rgba(54, 162, 235, 1)");
  humidityChart.data.labels = humidityData.labels;
  humidityChart.data.datasets[0].data = humidityData.data;
  
  var pm1Ctx = document.getElementById('pm1Chart').getContext('2d');
  var pm1Data = getStoredChartData('pm1Chart');
  var pm1Chart = createChart(pm1Ctx, 'PM1', "rgba(153, 102, 255, 1)");
  pm1Chart.data.labels = pm1Data.labels;
  pm1Chart.data.datasets[0].data = pm1Data.data;
  
  var pm25Ctx = document.getElementById('pm25Chart').getContext('2d');
  var pm25Data = getStoredChartData('pm25Chart');
  var pm25Chart = createChart(pm25Ctx, 'PM2.5', "rgba(75, 192, 192, 1)");
  pm25Chart.data.labels = pm25Data.labels;
  pm25Chart.data.datasets[0].data = pm25Data.data;
  
  var pm10Ctx = document.getElementById('pm10Chart').getContext('2d');
  var pm10Data = getStoredChartData('pm10Chart');
  var pm10Chart = createChart(pm10Ctx, 'PM10', "rgba(255, 159, 64, 1)");
  pm10Chart.data.labels = pm10Data.labels;
  pm10Chart.data.datasets[0].data = pm10Data.data;
  
  var ppmCtx = document.getElementById('ppmChart').getContext('2d');
  var ppmData = getStoredChartData('ppmChart');
  var ppmChart = createChart(ppmCtx, 'PPM', "rgba(255, 0, 0, 1)");
  ppmChart.data.labels = ppmData.labels;
  ppmChart.data.datasets[0].data = ppmData.data;
  
  function updateChartsWithData(myObj) {
    updateChart(temperatureChart, myObj.dhttemp);
    updateChart(humidityChart, myObj.humidity);
    updateChart(pm25Chart, myObj.pm2_5);
    updateChart(pm1Chart, myObj.pm1);
    updateChart(pm10Chart, myObj.pm10);
    updateChart(ppmChart, myObj.air_quality);
  
    setStoredChartData('temperatureChart', {
      labels: temperatureChart.data.labels,
      data: temperatureChart.data.datasets[0].data
    });
  
    setStoredChartData('humidityChart', {
      labels: humidityChart.data.labels,
      data: humidityChart.data.datasets[0].data
    });
  
    setStoredChartData('pm1Chart', {
      labels: pm1Chart.data.labels,
      data: pm1Chart.data.datasets[0].data
    });
  
    setStoredChartData('pm25Chart', {
      labels: pm25Chart.data.labels,
      data: pm25Chart.data.datasets[0].data
    });
  
    setStoredChartData('pm10Chart', {
      labels: pm10Chart.data.labels,
      data: pm10Chart.data.datasets[0].data
    });
  
    setStoredChartData('ppmChart', {
      labels: ppmChart.data.labels,
      data: ppmChart.data.datasets[0].data
    });
  }
  
  function getDataAndUpdateCharts() {
    var xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function () {
      if (this.readyState === 4 && this.status === 200) {
        var myObj = JSON.parse(this.responseText);
        updateChartsWithData(myObj);
      }
    };
    xhr.open("GET", "/readings", true);
    xhr.send();
  }
  
  setInterval(getDataAndUpdateCharts, 1000);
  getDataAndUpdateCharts();
  
  function goBack() {
    window.location.href = 'index.html';
  }
  