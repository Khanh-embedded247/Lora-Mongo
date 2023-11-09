const authorController = require("../controllers/authControllers");

const router =require("express").Router();

router.post("/register",authorController.registerUser);

module.exports =router;