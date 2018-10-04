module.exports.getDevice = function(id, type, name, ip, url) {
    var d = {
        id: id,
        deviceType: type,
        name: name,
        ip: [ip],
        url: url? url: (ip + ':' + 7000),
        print: function () {
            console.log('<<DEVICE_FOUND: %s>>||\r\n', JSON.stringify(d));
        }
    }
    return d;
}
