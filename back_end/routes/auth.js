const authorController = require("../controllers/authControllers");
const middlewareController = require("../controllers/middlewareController");

const router = require("express").Router();
//REGISTER
router.post("/register", authorController.registerUser);

//LOGIN
router.post("/login", authorController.loginUser);

//REFRESH
router.post("/refresh",authorController.requestRefreshToken);

//Log out
router.post("/logout",middlewareController.verifyToken, authorController.userLogout);

module.exports = router;