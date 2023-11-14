const User = require("../models/User");
const jwt = require("jsonwebtoken");
const bcrypt = require("bcrypt");//che mk khi nhap

let refreshTokens = [];
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

    },
    //GENERATE ACCESS TOKEN
    generateAccessToken: (user) => {
        return jwt.sign({
            id: user.id,
            admin: user.admin,
        }, 
            "secretkey",
            { expiresIn: "20s" }
        );
    },

    //GENERATE REFRESH TOKEN 
    generateRefreshToken: (user) => {
        return jwt.sign({
            id: user.id,
            admin: user.admin,
        },
            "refreshKey",
            { expiresIn: "365d" }
        );
    },

    //LOGIN
    loginUser: async (req, res) => {
        try {
            const user = await User.findOne({ username: req.body.username });
            if (!user) {
                return res.status(404).json("Wrong username!");
            }
            const validPassword = await bcrypt.compare(
                req.body.password,
                user.password
            )

            if (!validPassword) {
                return res.status(404).json("Wrong Password");

            } 
            if (user && validPassword) {
                const accessToken = authorController.generateAccessToken(user);
                const refreshToken = authorController.generateRefreshToken(user);
                refreshTokens.push(refreshToken);
                res.cookie("refreshToken", refreshToken, {
                    httpOnly: true,
                    secure: false,
                    path: "/",
                    sameSite: "strict",
                })
                const { password, ...others } = user._doc;
                res.status(200).json({ ...others, accessToken });
            }
        } catch (err) {
            res.status(500).json(err);
        }
    },

    requestRefreshToken: async (req, res) => {
        //Take refresh token from user
        const refreshToken = req.cookies.refreshToken;
        res.status(200).json(refreshToken);
        if (!refreshToken) {
            return res.status(401).json("You're not authenticated ");
        }
        if(!refreshTokens.includes(refreshToken)){
            return res.status(403).json("Refresh token is not valid");
        }
        jwt.verify(refreshToken, "refreshKey", (err, user) => {
            if (err) {
                console.log(err);
            }
            refreshTokens=refreshTokens.filter((token)=>token !==refreshToken);
            //Create new accessToken,refreshToken
            const newAccessToken = authorController.generateAccessToken(user);
            const newRefreshToken = authorController.generateRefreshToken(user);
            refreshTokens.push(newRefreshToken);
            res.cookie("refreshToken", newRefreshToken, {
                httpOnly: true,
                secure: false,
                path: "/",
                sameSite: "strict",
            });
            res.status(200).json({ accessToken: newAccessToken });
        });

    },
    //LOG OUT
    userLogout:async(req,res)=>{
        res.clearCookie("refreshToken");
        refreshTokens=refreshTokens.filter(token=>token !==req.cookies.refreshToken);
        res.status(200).json("Logged out");
    }

};
//STORE TOKEN
//(1)local storage->easy attack (xss)
//(2)httponlyCOOKIES :CSRF->SAMESITE

//(3)REDUX STORE ->ACCESSTOKEN
//HTTPONLY COOKIES ->REFRESHTOKEN

//BFF PATTERN(BACKEND FOR FRONTEND)

module.exports = authorController;