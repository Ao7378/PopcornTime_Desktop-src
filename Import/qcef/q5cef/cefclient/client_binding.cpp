#include "client_binding.h"
#include "cefclient/client_transfer.h"
#include "cefclient/message_event.h"
#include "cefclient/client_app_js.h"

//#include <QDebug>

extern CefRefPtr<ClientHandler> g_handler;

namespace client_binding
{

namespace
{


// Handle messages in the browser process.
class Handler : public CefMessageRouterBrowserSide::Handler
{
public:
    Handler() { }

    // Called due to cefQuery execution in binding.html.
    virtual bool OnQuery( CefRefPtr<CefBrowser> browser,
                          CefRefPtr<CefFrame> frame,
                          int64 query_id,
                          const CefString& request,
                          bool persistent,
                          CefRefPtr<Callback> callback ) OVERRIDE
    {
//      qDebug() << __FILE__ << __FUNCTION__ << __LINE__ << QString( std::string( request ).c_str() );

        QString req = QString::fromStdWString( request.ToWString() );
        const int divOffset = req.indexOf( ":" );
        if ( !g_handler.get() || !g_handler->listener() || divOffset < 1 )
        {
//          qDebug() << "invalid req" << req;
            callback->Failure( 0, "" );
            return false;
        }
        QString func = req.left( divOffset );
        QString args = req.mid( divOffset + 1 );
        QVariantList message_args;
        message_args += args;

        MessageEvent *e = new MessageEvent( func, message_args );
        g_handler->listener()->OnMessageEvent( e );
//      qDebug() << "func" << func << message_args;
        callback->Success( "" );
        return true;


        // Only handle messages from the test URL.
//  const std::string& url = frame->GetURL();
//  if (url.find(kTestUrl) != 0)
//    return false;

//  const std::string& message_name = request;
//  if (message_name.find(kTestMessageName) == 0) {
//    // Reverse the string and return.
//    std::string result = message_name.substr(sizeof(kTestMessageName));
//    std::reverse(result.begin(), result.end());
//    callback->Success(result);
        return true;
//  }

        return false;
    }


};

}  // namespace

void CreateMessageHandlers( ClientHandler::MessageHandlerSet& handlers ) { handlers.insert( new Handler ); }

}  // namespace client_binding

