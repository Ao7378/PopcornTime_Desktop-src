var _ = require('lodash');
var path = require('path');
var util = require('util');
var net_utils = require('./../net-utils');
var xmlb = require('xmlbuilder');
var browser = null;
var streamingPort = null;
var listen_url = null;
var deviceIP = null;
var device = null;
var prevVolume = 0.5;
var client = null;
var query = null;
var  session = null;
var getClient = function(cb) {
    if (client)
        return cb(client);
    deviceIP = query['deviceIP'];

    var ssdp = require('./../common/ssdp');
    var browserSsdp = new ssdp.Browser({timeout:5000, deviceMeta:{'urn:schemas-upnp-org:device:MediaRenderer:1':{manufacturer:null, type:'dlna'} }});
    browserSsdp.on('deviceOn', function (found_device) {
        if (found_device.ip[0] != deviceIP)
            return;
        browserSsdp.stop();
        device = found_device;
        var MediaRendererClient = require('upnp-mediarenderer-client');
        client = new MediaRendererClient(device.url);
        return cb(client);
    });
}

var PrintWarn = function (msg) {
    msg = (msg && typeof msg != 'string' && msg.toString().indexOf('[') == 0) ? JSON.stringify(msg) : msg;
    console.log('<<WARN: %s}>>||\n', msg);
}
var PrintError = function (msg) {
    msg = (msg && typeof msg != 'string' && msg.toString().indexOf('[') == 0) ? JSON.stringify(msg) : msg;
    msg = !msg || msg == '' ? '{}' : msg;
    console.error('<<ERR: %s>>||\n', msg);
}
var  getTime = function(time){
   // 00:00:00
    try{
    if(!time)
    return time;
    var parts = time.split(':').map(Number);
    return parts[0] * 3600 + parts[1] * 60 + parts[2];
    }catch(e){ return 0;}
}
var PrintMediaStatus = function (msg) {
    var jsn = {};
    if (msg) {

        jsn = {
            "mediaSessionId": 1,
            "playbackRate": 144,
            "playerState": msg.TransportState == 'PLAYING'? 'PLAYING' :(msg.TransportState == 'PAUSED_PLAYBACK'? 'PAUSED' : 'BUFFERING'),
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
    var status = client ? 'CONNECTED' : 'STOPPED';
    console.log('<<DSTAT: %s>>||\n', status);
}
var standardPrint = function (err, status) {
    if(err) {
        PrintError(err);
        return;
    }
    PrintMediaStatus(session);
    console.info('standardPrint', status);
    return;
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
CommandProcessor.prototype.load = function(url_video, url_subtitle){
    var metadata = null;
    if (url_subtitle) {
        metadata = xmlb.create('DIDL-Lite', {
            'headless': true
        })
            .att({
                'xmlns': 'urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/',
                'xmlns:dc': 'http://purl.org/dc/elements/1.1/',
                'xmlns:upnp': 'urn:schemas-upnp-org:metadata-1-0/upnp/',
                'xmlns:dlna': 'urn:schemas-dlna-org:metadata-1-0/',
                'xmlns:sec': 'http://www.sec.co.kr/',
                'xmlns:xbmc': 'urn:schemas-xbmc-org:metadata-1-0/'
            })
            .ele('item', {
                'id': '0',
                'parentID': '-1',
                'restricted': '1'
            })
            .ele('dc:title', {}, 'Popcorn Time Video')
            .insertAfter('res', {
                'protocolInfo': 'http-get:*:video/mp4:*',
                'xmlns:pv': 'http://www.pv.com/pvns/',
                'pv:subtitleFileUri': url_subtitle,
                'pv:subtitleFileType': 'srt'
            }, url_video)
            .insertAfter('res', {
                'protocolInfo': 'http-get:*:text/srt:'
            }, url_subtitle)
            .insertAfter('res', {
                'protocolInfo': 'http-get:*:smi/caption'
            }, url_subtitle)
            .insertAfter('sec:CaptionInfoEx', {
                'sec:type': 'srt'
            }, url_subtitle)
            .insertAfter('sec:CaptionInfo', {
                'sec:type': 'srt'
            }, url_subtitle)
            .insertAfter('upnp:class', {}, 'object.item.videoItem.movie')
            .end({
                pretty: false
            });
    } else {
        metadata = xmlb.create('DIDL-Lite', {
            'headless': true
        })
            .att({
                'xmlns': 'urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/',
                'xmlns:dc': 'http://purl.org/dc/elements/1.1/',
                'xmlns:upnp': 'urn:schemas-upnp-org:metadata-1-0/upnp/',
                'xmlns:dlna': 'urn:schemas-dlna-org:metadata-1-0/',
                'xmlns:sec': 'http://www.sec.co.kr/',
                'xmlns:xbmc': 'urn:schemas-xbmc-org:metadata-1-0/'
            })
            .ele('item', {
                'id': '0',
                'parentID': '-1',
                'restricted': '1'
            })
            .ele('dc:title', {}, 'Popcorn Time Video')
            .insertAfter('res', {
                'protocolInfo': 'http-get:*:video/mp4:*',
            }, url_video)
            .insertAfter('upnp:class', {}, 'object.item.videoItem.movie')
            .end({
                pretty: false
            });
    }
    client.load(url_video, {
        metadata: metadata,
        autoplay: true,
        contentType: 'video/vnd.dlna.mpeg-tts'
    }, standardPrint);
}
CommandProcessor.prototype.parseSubTitles = function(subtitlesFile, cb) {
    if (subtitlesFile && subtitlesFile != '') {
        var ext = path.extname(subtitlesFile).toLowerCase();
        ext = ext == '' ? '.srt' : ext;
        if (ext == '.vtt') {
            subtitlesFile = util.format('http://%s:%s/getSubtitle/suntitle.vtt?torrentFile=%s', listen_url, streamingPort, encodeURIComponent(subtitlesFile));
            cb(subtitlesFile);
        } else {
            var subConverter = require('./../subtitles-converter-factory').getConvertor(ext);
            if (subConverter) {
                subConverter.on('error', function (error) {
                    console.error(error);
                    subtitlesFile = null;
                    cb(subtitlesFile);
                });
                subConverter.on('ready', function (result) {
                    subtitlesFile = util.format('http://%s:%s/getSubtitle/suntitle.vtt?torrentFile=%s', listen_url, streamingPort, encodeURIComponent(result));
                    cb(subtitlesFile);
                });
            }
            subConverter.convertFile(subtitlesFile);
        }
    } else {
        cb(subtitlesFile);
    }
}

CommandProcessor.prototype.deviceType = 'dlna';
CommandProcessor.prototype.processCommand = function (url_parts) {
    var self = this;
    try {
        query = url_parts.query;
        if (url_parts.path.indexOf('/load') != -1) {
            client && client.stop();
            client = null;
            getClient(function(client){
                var subtitlesFile = null;
                client.on('error', function (error) {
                    PrintError(error);
                });
                client.on('status', function(status) {
                    // Reports the full state of the AVTransport service the first time it fires,
                    // then reports diffs. Can be used to maintain a reliable copy of the
                    // service internal state.
                    if(! session ||status.AVTransportURI){
                        session = status;
                        session.duration = 0;
                        session.position = 0;
                        session.subtitlesFile = subtitlesFile;

                    }else{
                        session.TransportState = status.TransportState;
                        session.subtitlesFile = subtitlesFile;
                    }
                    if(status.CurrentTrackDuration){
                        var duration = getTime(status.CurrentTrackDuration);
                        if(session.duration < duration){
                            session.duration = duration;
                        }
                    }
                    standardPrint(null, session);
                    console.log('status:',status);
                });
                client.stop();
                PrintDeviceStatus(true);

                console.log('connected, launching app ...');

             //   standardPrint();
                listen_url = net_utils.getMatchedLocalIp(deviceIP);
                var torrentFile = query['torrentFile'];
                if (torrentFile.indexOf('http') != 0) {

                    torrentFile = util.format('http://%s:%s/getStream/movie.mp4?torrentFile=%s', listen_url, streamingPort, encodeURIComponent(query['torrentFile']));
                }


                subtitlesFile = query['subtitles'];
                subtitlesFile = !subtitlesFile ? '' : subtitlesFile;
                self.parseSubTitles(subtitlesFile, function(subfile){
                    subtitlesFile = subfile;
                    self.load(torrentFile, subfile);
                });

            });
            //browser.start();
        }
        else if (url_parts.path.indexOf('/play') != -1) {
            getClient(function(client){
            if (!client) return module.exports.noSession;
            client.play(standardPrint);
            });
        } else if (url_parts.path.indexOf('/pause') != -1) {
            getClient(function(client){
                if (!client) return module.exports.noSession;
                client.pause(standardPrint);
            });

        } else if (url_parts.path.indexOf('/stop') != -1) {
            getClient(function(client){
                if (!client) return module.exports.noSession;

                client.stop(standardPrint);
            });


        } else if (url_parts.path.indexOf('/mute') != -1) {
            PrintWarn('mute not implemented');
            return module.exports.notSupported;

        } else if (url_parts.path.indexOf('/volume') != -1) {
            PrintWarn('volume not implemented');
            return module.exports.notSupported;

        } else if (url_parts.path.indexOf('/seek') != -1) {
            getClient(function(client){
                if (!client) return module.exports.noSession;
                client.seek(parseFloat(query['seek']), standardPrint);
            });


        } else if (url_parts.path.indexOf('/deviceState') != -1) {

            PrintDeviceStatus(null);

        } else if (url_parts.path.indexOf('/mediaState') != -1) {
            getClient(function(client){
                if (!client || !session) return module.exports.noSession;
                client.getPosition(function(err, duration){
                    if(err)
                        return standardPrint(err, null);
                    session.duration = duration;
                    client.getPosition(function(err, pos){
                        if(err)
                        return standardPrint(err, null);
                        session.position = pos;
                        standardPrint(null, session);

                    });
                });

            });

        } else if (url_parts.path.indexOf('/subtitles') != -1) {
            getClient(function(client){
                if (!client || !session) return module.exports.noSession;

                self.load(session.AVTransportURI, session.subtitlesFile);
                var subtitlesFile = query['subtitles'];
                subtitlesFile = !subtitlesFile ? '' : subtitlesFile;
                self.parseSubTitles(subtitlesFile, function(subfile){
                    session.subtitlesFile = subfile;
                    self.load(session.AVTransportURI, session.subtitlesFile);
                });

            });


        } else if (url_parts.path.indexOf('/fontSize') != -1) {
            PrintWarn('fontSize not implemented');
            return module.exports.notSupported;
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
module.exports.notSupported = 'Exception raised';
module.exports.Create = function ( streaming_Port) {
    streamingPort = streaming_Port;
    return new CommandProcessor();
};