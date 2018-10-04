
var util = require('util');
var events = require('events');
var http = require('http');
var url = require('url');

var _ = require('lodash');
var ssdp = require('node-ssdp').Client;
var netUtils = require('./net-utils.js');
var debugMod = require('debug');
var debug = debugMod('ssdp-wrapper');
var log = debugMod('ssdp-wrapper:log');
log.log = console.log.bind(console);
var error = debugMod('ssdp-wrapper:error');
var Device = require('./general-device');


var internal = {
    SSDP_ALL : 'ssdp:all',
    TIME_OUT : 15000,
    options:null,
    createConnection:function(ip, callback) {

        var ssdpBrowser = new ssdp({unicastHost: ip});
        log('Bind ssdpBrowser to ', ip);
        ssdpBrowser.on('response', function (headers, statusCode, rinfo) {
            if (statusCode != 200)
                return;
            if (!headers['LOCATION'])
                return;
            log('LOCATION:', headers['LOCATION']);
            log('headers:', JSON.stringify(headers));
            callback(null, headers);

        }.bind(this));
        ssdpBrowser.on('error', function (err) {
            error('chromecast Err', err)
            callback(err);
        }.bind(this));

        /**/
        ssdpBrowser.search(internal.options.serviceType || internal.SSDP_ALL);
        // ssdpBrowser.search('urn:schemas-upnp-org:device:MediaRenderer:1');
        // ssdpBrowser.search('urn:dial-multiscreen-org:service:dial:1');
        // ssdpBrowser.search();
        return ssdpBrowser;
    },
    getDeviceInfo:function(location, diviceMeta, callback ) {
        var request = http.get(location, function (res) {
            var body = '';
            res.on('data', function (chunk) {
                body += chunk;
            });
            res.on('end', function () {
                if (diviceMeta.manufacturer && body.search('<manufacturer>'+diviceMeta.manufacturer+'</manufacturer>') == -1)
                    return;
                var match = body.match(/<friendlyName>(.+?)<\/friendlyName>/);
                if (!match || match.length != 2)
                    return;
                var urlMatch = body.match(/<URLBase>(.+?)<\/URLBase>/);
                var deviceUrl = urlMatch? urlMatch[1]:location;
                var ip = url.parse(deviceUrl).hostname;
                var name = match ? match[1] : diviceMeta.type;
                var deviceId = diviceMeta.type + '-' + ip;
                if (!Device.devices[deviceId]) {
                    Device.devices[deviceId] = Device.getDevice(diviceMeta.type, name, ip, location);
                    callback(null, Device.devices[deviceId]);
                }
            });
        });
        request.on('error', function (err) {
            callback(err);
        });

    },
    getUUID: function (usn) {
    var udn = usn;
    var s = usn.split("::");
    if (s.length > 0) {
        udn = s[0];
    }

    if (S(udn).startsWith("uuid:")) {
        udn = udn.substring(5);
    }

    return udn;
}

}


var browser = function(options) {
    events.EventEmitter.call(this);
    internal.options = options || {};
    this.init();
};

util.inherits( browser, events.EventEmitter );

exports.Browser = browser;

browser.prototype.stop = function(  ) {
    clearInterval(this.timerId);
    for (var i = 0; i < this.connections.length; i++) {
        try {
            this.connections[i].ssdpBrowser._stop();
        } catch (e) {
            error('Error on _stop', e);
        }
    }
}
browser.prototype.init = function( ) {
    var self = this;
    self.on('error', function( err ){
        error('descovery error:', err)
    });
    this.connections = netUtils.getLocalIps();
    this.connections.push({iaddrass:{address:'0.0.0.0'}});
    for(var i = 0; i<this.connections.length; i++ ) {
        var ip = 'NOT SET';
        try {
            ip =this.connections[i].iaddrass.address;
            this.connections[i].ssdpBrowser = internal.createConnection(ip, function (err, headers) {
                try {
                if (err) {
                    error('discovery error', err);
                    return self.emit('error', err);
                }
                var diviceMeta = internal.options.deviceMeta[headers['ST']];
                var location = headers['LOCATION'];
                if (!diviceMeta || !location)
                    return;
                internal.getDeviceInfo(location, diviceMeta, function (err, device) {
                    if (err) {
                        error('discovery error', err);
                        return self.emit('error', err);
                    }
                    self.emit('deviceOn', device);
                });
                } catch (e) {
                    error('discovery error', e);
                    return self.emit('error', e);
                }
            });
        } catch (e) {
            error('Error on createConnection, ip=' +ip, e);
        }
    }
    console.info('Ssdp discovering ... ');
    self.timerId = setTimeout(function () {
        this.stop();
        console.info('SSdp discovering finished..,');
    }.bind(this), internal.options.timeout || internal.TIME_OUT);

};