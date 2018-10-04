var fs = require('fs');
var readline = require('readline');
var str = require('string');
var util = require('util');
var EventEmitter  = require('events').EventEmitter;
var captions = require('node-captions');

var srtConverter =  function () {
    EventEmitter.call(this);
    var self = this;
}
util.inherits(srtConverter, EventEmitter);
srtConverter.prototype.convertFile = function (fileName) {
    var self = this;
    try {


        var srt = fileName;
        var vtt = srt.replace('.srt', '.vtt');
        //var lang = data.language;
        var encoding = 'utf8';
        captions.srt.read(srt, {encoding: encoding}, function (err, data) {
            if (err) {
                console.error('Error parsing file "%s", error %s', fileName, err);
                self.emit('error', 'Error parsing file ' + fileName);
            }else {
                fs.writeFile(vtt,  captions.vtt.generate(captions.srt.toJSON(data)), {encoding: encoding}, function (err) {
                    if (err) {
                        console.error('Error parsing file "%s", error %s', fileName, err);
                        self.emit('error', 'Error parsing file ' + fileName);
                    } else {
                        self.emit('ready', vtt);;
                    }
                });
            }
        });

    } catch (e) {
        console.error('Error parsing file "%s"', fileName, e);
        self.emit('error', 'Error parsing file ' + fileName);
    }
}
module.exports.Create = function () {
    return new srtConverter();
};
