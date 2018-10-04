
var airplay = require('./discovery/airplay-discovery');
var browserAirplay = new airplay.Browser();
browserAirplay.on('deviceOn', function (device) {
    device.print();
});

var ssdp = require('./common/ssdp');
var browserSsdp = new ssdp.Browser({timeout:5000, deviceMeta:{'urn:schemas-upnp-org:device:MediaRenderer:1':{manufacturer:null, type:'dlna'},'urn:dial-multiscreen-org:service:dial:1':{manufacturer:'Google Inc.',type:'googlecast'} }});
browserSsdp.on('deviceOn', function (device) {
    //console.info('location', device);
    device.print();
});