const express = require("express");
const cors = require("cors");
const dotenv = require("dotenv");
const http = require('http');
const mongoose = require("mongoose");
const cookieParser = require("cookie-parser");
const authRoute = require("./routes/auth");
const userRoute = require("./routes/user")
const mqtt = require('mqtt');
const Device=require('./models/Device');
dotenv.config();
const app = express();

mongoose.connect("mongodb://localhost:27017/do_an")
  .then(() => {
    console.log("Connected to MongoDB");
  })
  .catch((error) => {
    console.error("Error connecting to MongoDB:", error);
  });

app.use(cors());
app.use(cookieParser());
app.use(express.json());

//ROUTES
app.use("/v1/auth", authRoute);
app.use("/v1/user", userRoute)

// MQTT connection
const mqttClient = mqtt.connect('mqtt://mqtt_broker_url'); 

// mqttClient.on('connect', () => {
//   console.log('Connected to MQTT broker');
//   mqttClient.subscribe('', (data) => {
  
//   // mqttCLinet.sub('create_esp', () => )
//   // });

//   // 
// });

mqttClient.on('message', (topic, message) => {
  // Xử lý dữ liệu nhận được từ esp32
  const data = JSON.parse(message.toString());
  updateDeviceData(data);
});

// Hàm cập nhật dữ liệu thiết bị trong MongoDB
async function updateDeviceData(data) {
  try {
    const { esp32_id, sensors, fan, led } = data;

    // Tìm thiết bị theo esp32_id
    const device = await Device.findOne({ 'esp32.esp32_id': esp32_id });

    if (device) {
      // Cập nhật dữ liệu cảm biến
      device.esp32.sensors = sensors;

      // Cập nhật trạng thái quạt và đèn
      device.fan = fan;
      device.led = led;

      await device.save();
      console.log(`Updated device data for esp32_id: ${esp32_id}`);
    } else {
      console.log(`Device with esp32_id ${esp32_id} not found`);
    }
  } catch (error) {
    console.error('Error updating device data:', error);
  }
}

const port = process.env.PORT || 3000;

app.listen(port, () => {
  console.log(`Server is running on port ${port}`);
});


//json web token
