// This is the JavaScript code for bridging to native functionality
// Note: All functions are asynchronous. 


var baseUrl = "%webChannelBaseUrl%";
//alert("Connecting to WebSocket server at " + baseUrl + ".");
var ws = new WebSocket(baseUrl);

ws.onclose = function(){ alert("web channel closed");};
ws.onerror = function(error){ alert("web channel error: " + error);};
ws.onopen = function(){
// alert("WebSocket connected, setting up QWebChannel.");
   new QWebChannel(ws, function(channel){
//                 alert("web channel opened");
                   // make dialog object accessible globally
                   window.myhostApp = channel.objects.hostApp;
                  app.setConfig();
                  });
};

var hostApp;
if (!hostApp) {
   var obj = {
      persistent: false,
      onSuccess: function(response){},
      onFailure: function(error_code, error_message){}
   };
   hostApp = {
      sendWinAction: function(data)    { myhostApp.sendWinAction(data);},
      getTorrent: function(data)       { myhostApp.getTorrent(JSON.stringify(data));},
      url_request: function(data)      { myhostApp.url_request(data);},
      cancelTorrent: function(data)    { myhostApp.cancelTorrent(data);},
      setConfig: function(data)        { myhostApp.setConfig(data);},
      setConfigVars: function()        { myhostApp.setConfigVars(JSON.stringify(app.config.hostApp));},
      setCleanOnExit: function(data)   { myhostApp.setCleanOnExit(data);},
      setUserFontSize: function(data)  { myhostApp.setUserFontSize(data);},
      openBrowser: function(data)      { myhostApp.openBrowser(data);},
      closePlayer: function()          { myhostApp.closePlayer();},
      vpn_connect: function()          { myhostApp.vpn_connect();},
      vpn_disconnect: function()       { myhostApp.vpn_disconnect();},
      OpenTempDir: function()          { myhostApp.OpenTempDir();},
      setTempPath: function()          { myhostApp.setTempPath();},
   }
   hostApp.settings  = {
      load: function(callback)         { myhostApp.settingsLoad(callback);},
      save: function(data)             { myhostApp.settingsSave(data);}
   };
}










