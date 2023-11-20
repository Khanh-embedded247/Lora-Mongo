const mongoose = require('mongoose');


const dht = new mongoose.Schema({
    temp: { type: String},
    hum: {type:String},
  });
const mq=new mongoose.Schema({
    construction:{type:String},
});
const sensorSchema = new mongoose.Schema({
  
    dht11:[dht],
    mq135:[mq]
  });
const esp32Schema = new mongoose.Schema({
  esp32_id: { type: String},
  sensors: [sensorSchema],
});

const deviceSchema = new mongoose.Schema({
  user: { type: mongoose.Schema.Types.ObjectId, ref: 'User', required: true },
  esp32: esp32Schema,
  fan: { type: String, enum: ['On', 'Off'] },
  led: { type: String, enum: ['On', 'Off'] },
});

const Device = mongoose.model('Device', deviceSchema);

module.exports = Device;
