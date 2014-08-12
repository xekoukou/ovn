if (process.argv.length != 6) {
    console.log("Please provide the ip and port to which nodejs will connect to the ovn_server and the ip and port of the https server");
    return -1;
}



var server = require("./http.js")(process.argv[4], process.argv[5]);

var io = require("./socketIO.js")(server);

var dealer = require("./zmq.js")(process.argv[2], process.argv[3]);


var logic = require("./logic.js");

io.of('/graph').on('connection', function(socket) {
console.log("One user just connected");
    logic.io(socket, io, dealer);

    socket.on('disconnect', function() {

        console.log("one user disconnected");
    });

});


logic.zmq(dealer, io);
