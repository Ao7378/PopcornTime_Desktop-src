#include "cefclient/client_handler.h"

#include <string>
#include <windows.h>
#include <shlobj.h> 

#include "include/cef_browser.h"
#include "include/cef_frame.h"

void ClientHandler::OnAddressChange(CefRefPtr<CefBrowser> browser,
                                    CefRefPtr<CefFrame> frame,
                                    const CefString& url) {
  REQUIRE_UI_THREAD();
  if (!listener_) return;

  if (m_BrowserId == browser->GetIdentifier() && frame->IsMain()) {
    listener_->OnAddressChange(QString::fromStdWString(url.ToWString()));
  }
}

void ClientHandler::OnTitleChange(CefRefPtr<CefBrowser> browser,
                                  const CefString& title) {
  REQUIRE_UI_THREAD();
  if (!listener_) return;

  if (m_BrowserId == browser->GetIdentifier()) {
    listener_->OnTitleChange(QString::fromStdWString(title.ToWString()));
  }
}

void ClientHandler::SetLoading(bool isLoading) {
  if (!listener_) return;
  listener_->SetLoading(isLoading);
}

void ClientHandler::SetNavState(bool canGoBack, bool canGoForward) {
  if (!listener_) return;
  listener_->SetNavState(canGoBack, canGoForward);
}

void ClientHandler::OnLoadError(CefRefPtr<CefBrowser> browser,
                                CefRefPtr<CefFrame> frame,
                                ErrorCode errorCode,
                                const CefString& errorText,
                                const CefString& failedUrl) {
  REQUIRE_UI_THREAD();

  // Don't display an error for downloaded files.
  if (errorCode == ERR_ABORTED) return;

  // Don't display an error for external protocols that we allow the OS to
  // handle. See OnProtocolExecution().
  if (errorCode == ERR_UNKNOWN_URL_SCHEME) {
    std::string urlStr = frame->GetURL();
    if (urlStr.find("spotify:") == 0) return;
  }

  if (listener_) listener_->OnLoadError( std::string(failedUrl).c_str(),
                                         std::string(errorText).c_str(),
                                         errorCode );
      
  
  // Issue: It will repeated load the url which is a not existed file path on
  // Win32 CEF 3.1650.1522,1544.
  // 1. You need load the file:/// (not exists) several times.
  // 2. It jumps to about:blank after the first time, then load the url again.
  // 3. It also jumps to about:blank and repeated load, but finally displays
  // LoadString() on Win32 CEF 3.1547.1551. Sometimes also repeated load forever.
  // 4. It will directly repeated load if you stop loading m_StartupURL.
  // 5. It doesn't go into OnRenderProcessTerminated().er
  // Also have been tested with its own cefclient project.

    // Display a load error message.
//std::stringstream ss;
////ss << "<html><body bgcolor=\"white\">"
////      "<h2>Failed to load URL " << std::string(failedUrl) <<
////      " with error " << std::string(errorText) << " (" << errorCode <<
////      ").</h2></body></html>";
////frame->LoadString(ss.str(), failedUrl);
//ss << "Failed to load URL " << std::string(failedUrl) <<
//      " with error " << std::string(errorText) << " (" << errorCode << ").";
//qDebug() << __FUNCTION__ << QString::fromStdString(ss.str());

}

