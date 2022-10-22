// Create express app
const express = require('express');
const app = express();
const configs = require('./configs.js').configs;
const fs = require('fs');
const https = require('https');
const path = require('path');
const sqlite3 = require('sqlite3').verbose();
const basicAuth = require('express-basic-auth');
const databasePath = path.join(__dirname, '..', '..', 'detection-records.sqlite');


https.createServer(
    {
      key: fs.readFileSync(configs.ssl.key),
      cert: fs.readFileSync(configs.ssl.crt)
    },
    app
).listen(configs.port, function() {
  console.log(
      `ism.js listening on https://0.0.0.0:${configs.port}!`
  );
});

app.use(basicAuth({
  users: configs.users,
  challenge: true // <--- needed to actually show the login dialog!
}));
app.use('/', express.static(path.join(__dirname, 'public/')));

app.get('/get_logged_in_user/', (req, res, next) => {
  res.json({
    'status': 'success',
    'data': req.auth.user
  });
});

app.get('/get_ir_detection_count/', (req, res, next) => {
  const db = new sqlite3.Database(databasePath, (err) => {
    if (err) {
      res.status(500).json({
        'status': 'error',
        'message': 'Cannot open database'
      });
      console.error(err.message);
    } else {
      const sql = `
        SELECT DATE(timestamp) AS 'Date', COUNT(*) AS 'RecordCount' FROM DetectionRecords GROUP BY DATE(timestamp)
      `;
      const params = [];
      db.all(sql, params, (err, rows) => {
        if (err) {
          res.status(500).json({
            'status': 'error',
            'message': err.message
          });
          return;
        }
        res.json({
          'status': 'success',
          'data': rows
        });
      });
    }
  });
  db.close();
});


app.use('/', (req, res) => {
  return res.redirect('/html/index.html');
});

app.get('*', function(req, res) {
  res.status(404).send('This is a naive 404 page--the URL you try to access does not exist!');
});
