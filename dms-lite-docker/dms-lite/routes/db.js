var express = require('express');
const sqlite3 = require('sqlite3').verbose();

var db;

// Test DB connection
console.log("TESTING DB CONNECTION...")
openDB();
closeDB();

// This will be updated
// CREATE TABLE cluster_data(time_stamp datetime, device_id TEXT, message_id TEXT, payload TEXT, path TEXT);

function openDB() {
   db = new sqlite3.Database('./db/data.db', (err) => {
      if (err) {
         console.error(err.message);
      }
      console.log('Connected to data.db');
   });
}

function closeDB() {
   db.close((err) => {
      if (err) {
         return console.error(err.message);
      }
      console.log('Close the database connection.');
   });
}

function getAllData() {
   return new Promise((resolve, reject) => {
      openDB();
      let sql = 'SELECT time_stamp, device_id, topic, message_id, payload, path, hops, duck_type  FROM cluster_data ORDER BY time_stamp DESC LIMIT 100'
      console.log(sql)
      db.all(sql, (err, rows) => {
         if (err) {
            reject(err);
         }
         resolve(rows);
      });
      closeDB();
   });
}

function getDataByDuckId(duckId) {
   return new Promise((resolve, reject) => {
      openDB();
      let sql = 'SELECT time_stamp, device_id, topic, message_id, payload, path, hops, duck_type FROM cluster_data WHERE duck_id = ? '
      console.log(sql)

      db.all(sql, [duckId], (err, rows) => {
         if (err) {
            reject(err);
         }
         resolve(rows);
      });
      closeDB();
   });
}

function getUniqueDucks() {
   return new Promise((resolve, reject) => {
      openDB();
      let sql = 'SELECT DISTINCT device_id FROM cluster_data;'
      
      db.all(sql, (err, rows) => {
         console.log(rows);
         if (err) {
            reject(err);
         }
         resolve(rows);
      });
      //closeDB();
   });
}

function getLastCount(count) {
   return new Promise((resolve, reject) => {
      openDB();
      let sql = 'SELECT time_stamp, device_id, topic, message_id, payload, path, hops, duck_type  FROM cluster_data DESC LIMIT ? '

      db.all(sql, [count], (err, rows) => {
         if (err) {
            reject(err);
         }
         resolve(rows);
      });
      closeDB();
   });
}

function getDuckPlusData() {
   return new Promise((resolve, reject) => {
      openDB();
      let sql = 'SELECT time_stamp, device_id, topic, message_id, payload, path, hops, duck_type  FROM ( SELECT ROW_NUMBER() OVER ( PARTITION BY device_id ORDER BY time_stamp DESC ) RowNum, time_stamp, device_id, topic, message_id, payload, path, hops, duck_type  FROM cluster_data ) WHERE RowNum = 1; '

      db.all(sql, (err, rows) => {
         if (err) {
            reject(err);
         }
         resolve(rows);
      });
      closeDB();
   });
}


module.exports = {getAllData, getDataByDuckId, getUniqueDucks, getLastCount, getDuckPlusData};
