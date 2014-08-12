module.exports = function(server) {

    var io = require('socket.io').listen(server);

    return io;
}
