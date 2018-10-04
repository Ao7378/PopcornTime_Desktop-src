var _ = require('lodash');
var util = require('util');
var net_utils = require('./net-utils');
var browser = null;
var streamingPort = null;
var deviceIP = null;
var device = null;
var prevVolume = 0.5;

var PrintWarn = function (msg) {
    msg = (msg && typeof msg != 'string' && msg.toString().indexOf('[') == 0) ? JSON.stringify(msg) : msg;
    console.log('<<WARN: %s}>>||\n', msg);
}
var PrintError = function (msg) {
    msg = (msg && typeof msg != 'string' && msg.toString().indexOf('[') == 0) ? JSON.stringify(msg) : msg;
    msg = !msg || msg == '' ? '{}' : msg;
    console.error('<<ERR: %s>>||\n', msg);
}
var PrintMediaStatus = function (msg) {
    var jsn = {};
    if (msg) {
        jsn = {
            "mediaSessionId": 1,
            "playbackRate": msg.rate,
            "playerState": msg.duration ? (msg.rate == 1 ? (msg.readyToPlay ? 'PLAYING' : 'BUFFERING') : 'PAUSED') : 'BUFFERING',
            "currentTime": msg.duration ? msg.position : 0,
            "supportedMediaCommands": 5,
            "volume": {
                "level": 1,
                "muted": false
            },
            "media": {
                "contentId": "",
                "duration": msg.duration ? msg.duration : 0
            }
        }
    }

    console.log('<<MSTAT: %s>>||\n', JSON.stringify(jsn));
}
var PrintDeviceStatus = function (msg) {
    var status = msg ? 'CONNECTED' : 'STOPPED';
    console.log('<<DSTAT: %s>>||\n', status);
}
var standardPrint = function (status) {
    device.status(function (media) {
        PrintMediaStatus(media);
    });
}
var lastPrint = new Date();
var printFinished = true;
var lastMedia = null;
var standardSyncPrint = function (status) {

    if (printFinished) {
        printFinished = false;
        device.status(function (media) {
            lastMedia = media;
            lastPrint = new Date();
            PrintMediaStatus(media);
            printFinished = true;
        });
    }
    if (lastMedia) {
        lastMedia.position = lastMedia.position ? lastMedia.position : 0;
        lastMedia.position = lastMedia.position + (new Date() - lastPrint) / 1000;
        PrintMediaStatus(lastMedia);
    }
    lastPrint = new Date();
}
function CommandProcessor() {
}
CommandProcessor.prototype.deviceType = 'airplay';
CommandProcessor.prototype.processCommand = function (url_parts) {
    try {
        var query = url_parts.query;
        if (url_parts.path.indexOf('/load') != -1) {
            deviceIP = query['deviceIP'];
            browser = require('airplay-js').createBrowser();
            console.log('connecting ...');
            browser.on('deviceOn', function (found_device) {
                if (found_device.info[0] != deviceIP)
                    return;
                device = found_device;

                device.on('error', function (error) {
                    PrintError(error);
                });

                PrintDeviceStatus(true);

                console.log('connected, launching app ...');

                standardPrint();

                var torrentFile = query['torrentFile'];
                if (torrentFile.indexOf('http') != 0) {
                    var listen_url = net_utils.getMatchedLocalIp(deviceIP);
                    torrentFile = util.format('http://%s:%s/getStream/movie.mp4?torrentFile=%s', listen_url, streamingPort, encodeURIComponent(query['torrentFile']));
                }
                device.play(torrentFile, 0, function (status) {
                    standardPrint();
                    if (query['volume']) {
                        prevVolume = parseFloat(query['volume']);
                        device.volume(prevVolume, standardPrint);
                    }
                });
            });
            browser.start();
        }
        else if (url_parts.path.indexOf('/play') != -1) {

            if (!browser || !device) return module.exports.noSession;
            device.rate(1, standardPrint);
        } else if (url_parts.path.indexOf('/pause') != -1) {

            if (!browser || !device) return module.exports.noSession;
            device.rate(0, standardPrint);
        } else if (url_parts.path.indexOf('/stop') != -1) {

            if (!browser || !device) return module.exports.noSession;

            device.stop(function () {
                console.info('video stopped...');
                standardPrint();
                browser.stop();
                browser = null;
                device = null;
                PrintDeviceStatus(true);
            })

        } else if (url_parts.path.indexOf('/mute') != -1) {

            if (!browser || !device) return module.exports.noSession;

            device.volume(query['muted'].indexOf('1') != -1 ? 0 : prevVolume, standardPrint);

        } else if (url_parts.path.indexOf('/volume') != -1) {

            if (!browser || !device) return module.exports.noSession;
            prevVolume = parseFloat(query['volume']);
            device.volume(prevVolume, standardPrint);

        } else if (url_parts.path.indexOf('/seek') != -1) {

            if (!browser || !device) return module.exports.noSession;
            device.scrub(parseFloat(query['seek']), standardPrint);

        } else if (url_parts.path.indexOf('/deviceState') != -1) {

            if (!browser) {
                PrintDeviceStatus(false);
                return module.exports.notInited;
            }

            standardPrint();

        } else if (url_parts.path.indexOf('/mediaState') != -1) {

            if (!browser) {
                PrintDeviceStatus(false);
                return module.exports.notInited;
            }
            standardSyncPrint();
        } else if (url_parts.path.indexOf('/subtitles') != -1) {

            PrintWarn('subtitles not implemented');

        } else if (url_parts.path.indexOf('/fontSize') != -1) {

            PrintWarn('fontSize not implemented');
        }
        return module.exports.success;
    } catch (e) {
        PrintError(e)
        return module.exports.generalError;
    }
}
CommandProcessor.prototype.close = function () {
    try {
        browser.stop();
    } catch (e) {
        console.error('browser.close error', e);
    }
}
module.exports.success = 'OK';
module.exports.notInited = 'Not Initialized';
module.exports.noSession = 'Not session found';
module.exports.generalError = 'Exception raised';
module.exports.Create = function ( streaming_Port) {
    streamingPort = streaming_Port;
    return new CommandProcessor();
};