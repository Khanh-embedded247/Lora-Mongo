const middlewareController = require("../controllers/middlewareController");
const userController = require("../controllers/userController");

const router = require("express").Router();

//GET ALL USERS
router.get("/", middlewareController.verifyToken, userController.getAllUsers);

//DELETE USER
//v1/user/12344
router.delete("/:id",middlewareController.verifyTokenAndAdminAuth, userController.deleteUser);
module.exports = router;