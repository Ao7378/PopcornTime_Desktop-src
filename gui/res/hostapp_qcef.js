// This is the JavaScript code for bridging to native functionality
// Note: All functions are asynchronous. 

var hostApp;
if (!hostApp) {
   var obj = {
      persistent: false,
      onSuccess: function(response){},
      onFailure: function(error_code, error_message){}
   };
   hostApp = {
      sendWinAction: function(data)    { obj.request = 'sendWinAction:' + data;              window.cefQuery(obj);},
      getTorrent: function(data)       { obj.request = 'getTorrent:' + JSON.stringify(data); window.cefQuery(obj);},
      url_request: function(data)      { obj.request = 'url_request:' + data;                window.cefQuery(obj);},
      cancelTorrent: function(data)    { obj.request = 'cancelTorrent:' + data;              window.cefQuery(obj);},
      setConfig: function(data)        { obj.request = 'setConfig:' + data;                  window.cefQuery(obj);},
      setConfigVars: function()        { obj.request = 'setConfigVars:' + JSON.stringify(app.config.hostApp); window.cefQuery(obj);},
      setCleanOnExit: function(data)   { obj.request = 'setCleanOnExit:' + data;             window.cefQuery(obj);},
      setUserFontSize: function(data)  { obj.request = 'setUserFontSize:' + data;            window.cefQuery(obj);},
      openBrowser: function(data)      { obj.request = 'openBrowser:' + data;                window.cefQuery(obj);},
      closePlayer: function()          { obj.request = 'closePlayer:';                       window.cefQuery(obj);},
      vpn_connect: function()          { obj.request = 'vpn_connect:';                       window.cefQuery(obj);},
      vpn_disconnect: function()       { obj.request = 'vpn_disconnect:';                    window.cefQuery(obj);},
      OpenTempDir: function()          { obj.request = 'OpenTempDir:';                       window.cefQuery(obj);},
      setTempPath: function(data)      { obj.request = 'setTempPath:' + data;                window.cefQuery(obj);},
      clearCache: function()           { obj.request = 'clearCache:';                        window.cefQuery(obj);},
   };
   hostApp.settings  = {
      load: function(callback)         { obj.request = 'settingsLoad:' + callback;           window.cefQuery(obj);},
      save: function(data)             { obj.request = 'settingsSave:' + data;               window.cefQuery(obj);},
   };
}


//var hostApp;
//if (!hostApp) { alert('no_hostapp'); }
//else { alert('hostapp'); }

