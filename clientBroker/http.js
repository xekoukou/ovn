var fs = require("fs");

module.exports = function(address, port) {

    //tls certificate

    var tlsOptions = {
        key: fs.readFileSync(__dirname + '/tls/server.key'),
        cert: fs.readFileSync(__dirname + '/tls/server.crt'),
        ca: fs.readFileSync(__dirname + '/tls/ca.crt'),
    };

    var server = require('https').createServer(tlsOptions, function(req, res) {
    });

    //start server
    server.listen(port, address);

    return server;

}
