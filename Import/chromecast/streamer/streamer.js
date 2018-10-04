var http = require("http");
var vidStreamer = require("./vid-streamer");

var sendInfo = function (msg) {
    process.send({status: 'info', message:msg, error:null});
}
var sendError = function (msg, e) {
    process.send({status: 'error', message:msg, error:e});
}
vidStreamer.sendInfo = sendInfo;
vidStreamer.sendError = sendError;
var server = http.createServer(function (request, response) {
    try {
            vidStreamer(request, response);
    } catch (e) {
        response.writeHead(500, {'Content-Type': 'text/plain'});
        response.end('500 - server error\n');
        sendError('Error on request processing',e);
    }

});
server.listen(0, function () {
    process.send({status: 'ok', port: server.address().port});
});

