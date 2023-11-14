const jwt = require("jsonwebtoken");
const middlewareController = {

    //verifyToken
    verifyToken: (req, res, next) => {
        const token = req.headers.token;
        if (token) {
            //Bearer 12345
            const accessToken = token.split(" ")[1];
            jwt.verify(accessToken, "secretkey", (err, user) => {
                if (err) {
                    res.status(403).json({ error: "Token is not valid" });
                }
                req.user = user;
                next();
            });

        }
        else {
            res.status(401).json({ error: "You're not authenticated" });
        }
    },

    verifyTokenAndAdminAuth: (req, res, next) => {
        middlewareController.verifyToken(req, res, () => {
            if (req.user.id == req.params.id || req.user.admin) {
                next();
            }
            else {
                res.status(403).json("You are not allowed to delete other ");
            }

        });
    }
}
module.exports = middlewareController;