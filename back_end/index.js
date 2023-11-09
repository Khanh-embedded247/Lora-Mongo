const express = require("express");
const cors = require("cors");
const dotenv = require("dotenv");
const http = require('http');
const mongoose = require("mongoose");
const cookieParser = require("cookie-parser");
const authRoute = require("./routes/auth");
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

console.log('hello');


const server = http.createServer(app);

server.listen(3000, () => {
  console.log("Server is running!");
});

//AUTHENTICATION
//AUTHORIZATION

// //ROUTES
// app.use("/v1/auth", authRoute);