module.exports = function(address, port) {
    var pos_connect_point = "tcp://" + address + ":" + port;

    var zmq = require('zmq');

    //connect to to position server
    var dealer = zmq.socket('dealer');
    dealer.connect(pos_connect_point);

    return dealer;
}
