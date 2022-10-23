import * as React from 'react';
import ReactDOM from 'react-dom';
import axios from 'axios';
import AppBar from '@mui/material/AppBar';
import Box from '@mui/material/Box';
import Toolbar from '@mui/material/Toolbar';
import Typography from '@mui/material/Typography';
import Button from '@mui/material/Button';
import GrassIcon from '@mui/icons-material/Grass';

import {
  Chart as ChartJS,
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  Tooltip
} from 'chart.js';
import {Line} from 'react-chartjs-2';
import 'chartjs-adapter-luxon';

class Reminder extends React.Component {
  constructor(props) {
    super(props);
    this.state = {
      detectionCountData: null
    };
  }

  componentDidMount() {
    this.fetchDataFromServer(60);
  }

  fetchDataFromServer(days) {
    axios.get(`../get_ir_detection_count/`)
        .then((response) => {
          this.setState({detectionCountData: response.data.data});
        })
        .catch((error) => {
          alert(`加载每日A数失败！原因：\n${(error.response !== undefined) ? JSON.stringify(error.response): error}`);
        });
  }

  movingAverage(arr) {
    const res = [];
    let sum = 0;
    let count = 0;
    for (let i = 0; i < arr.length; i++) {
      const el = arr[i];
      sum += el;
      count++;
      const curr = sum / count;
      res[i] = curr;
    };
    return res;
  }

  render() {
    if (this.state.detectionCountData == null) {
      return null;
    }

    ChartJS.register(
        CategoryScale,
        LinearScale,
        PointElement,
        LineElement,
        Tooltip
    );
    const dateLabels = this.state.detectionCountData.map((e) => e.Date);
    const detectionCounts = this.state.detectionCountData.map((e) => e.RecordCount);

    const options = {
      responsive: true,
      maintainAspectRatio: false,
      interaction: {
        mode: 'nearest',
        axis: 'x',
        intersect: false
      },
      scales: {
        xAxis: {
          grid: {display: false},
          type: 'time',
          time: {
            parser: 'yyyy-MM-dd',
            displayFormats: {
              'day': 'yyyy-MM-dd',
              'month': 'yyyy-MM'
            },
            tooltipFormat: 'yyyy-MM-dd'
          },
          ticks: {
            autoSkip: true,
            maxRotation: 0,
            minRotation: 0,
            maxTicksLimit: 4
          },
          display: true
        },
        yAxis: {
          grid: {display: true},
          ticks: {beginAtZero: false},
          position: 'right'
        }
      },
      plugins: {
        legend: {display: false}
      }
    };
    const data = {
      labels: dateLabels,
      datasets: [
        {
          label: '原始数据',
          data: detectionCounts,
          borderColor: 'rgb(25, 118, 210, 0.75)',
          backgroundColor: 'rgba(25, 118, 210, 0.25)',
          borderWidth: 0.8,
          tension: 0.1,
          pointRadius: 0
        },
        {
          label: '移动平均',
          data: this.movingAverage(detectionCounts),
          borderColor: 'rgb(25, 118, 210, 0.75)',
          backgroundColor: 'rgba(25, 118, 210, 0.25)',
          borderWidth: 4.5,
          tension: 0.5,
          pointRadius: 0.25
        }
      ]
    };
    return (
      <Line options={options} data={data} style={{height: '70vh'}}/>
    );
  }
}

class NavBar extends React.Component {
  constructor(props) {
    super(props);
    this.state = {
      user: null
    };
  }

  componentDidMount() {
    axios.get('../get_logged_in_user/')
        .then((response) => {
          this.setState({
            user: response.data.data
          });
        })
        .catch((error) => {
          console.log(error);
          alert(`${error}`);
        });
  }
  render() {
    return (
      <Box sx={{flexGrow: 1, mb: '1rem'}}>
        <AppBar position="static">
          <Toolbar>
            <GrassIcon sx={{display: {md: 'flex'}, mr: 1}} />
            <Typography
              variant="h6"
              noWrap
              component="a"
              href="/"
              sx={{
                mr: 2,
                display: {md: 'flex'},
                fontWeight: 700,
                letterSpacing: '.1rem',
                color: 'inherit',
                textDecoration: 'none'
              }}
            >
              阳台活动计数器
            </Typography>
            <Typography variant="h6" component="div" sx={{flexGrow: 1}}>
            </Typography>
            <Button color="inherit">{this.state.user}</Button>
          </Toolbar>
        </AppBar>
      </Box>
    );
  }
}

class Index extends React.Component {
  constructor(props) {
    super(props);
    this.state = {
      currDate: new Date()
    };
  }

  render() {
    return (
      <>
        <NavBar />
        <div style={{maxWidth: '1280px', display: 'block', marginLeft: 'auto', marginRight: 'auto'}}>
          <Reminder></Reminder>
        </div>
      </>
    );
  }
}

const container = document.getElementById('root');
ReactDOM.render(<Index />, container);
