const User = require("../models/User");
const bcrypt = require("bcrypt");//che mk khi nhap

const authorController = {
    //Register
    registerUser: async (req, res) => {
        try {
            // kiem tra xem user do da ton tai chua
            const existedUser = await User.findOne({ email: req.body.email });
            if (existedUser) {
                return res.status(200).json({
                    message: 'User existed!'
                })
            } else {
                const salt = await bcrypt.genSalt(10);
                const hashed = await bcrypt.hash(req.body.password, salt);

                //Create new user
                const newUser = await new User({
                    username: req.body.username,
                    email: req.body.email,
                    password: hashed,
                }).save();

                //Save to database
                return res.status(200).json({
                    message: 'Success!',
                    user: newUser
                });
            }

        } catch (err) {
            return res.status(500).json(err);
        }

    }
}

module.exports = authorController;