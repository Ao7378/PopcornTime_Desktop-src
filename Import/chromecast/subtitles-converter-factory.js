var str = require('string');
module.exports.getConvertor = function(ext)
{
    ext = str(ext.toLowerCase()).trim().s;
    switch(ext)
    {
        case '.srt': return require('./subtitlesConverters/srtConverter').Create();
        default : return null;
    }

}