#include "client_handler.h"
#include <sstream>
#include <stdio.h>
#include <string>
#include "include/cef_browser.h"
#include "include/cef_frame.h"
#include "include/cef_runnable.h"
#include "cefclient/cefclient.h"
#include "cefclient/client_renderer.h"
#include "cefclient/client_binding.h"
#include "defaults.h"

#ifdef CEF_EVENTS_TO_QAPPLICATION
#include <QApplication>
#include <QKeyEvent>
#include <QEvent>
#include <QWidget>
#include <QDesktopServices>
#endif

#include <QDebug>

namespace {

// Custom menu command Ids.
enum client_menu_ids {
  CLIENT_ID_SHOW_DEVTOOLS = MENU_ID_USER_FIRST,
  CLIENT_ID_CLOSE_DEVTOOLS,
};

}  // namespace

int ClientHandler::m_BrowserCount = 0;

ClientHandler::ClientHandler()
  : m_BrowserId(0),
    m_bIsClosing(false),
    m_bFocusOnEditableField(false) {
  if (m_StartupURL.empty())
    m_StartupURL = "http://www.google.com/";
}

ClientHandler::~ClientHandler() {
}

bool ClientHandler::OnProcessMessageReceived(
    CefRefPtr<CefBrowser> browser,
    CefProcessId source_process,
    CefRefPtr<CefProcessMessage> message) {
//  qDebug() << __FILE__ << __FUNCTION__ << __LINE__ << QString( CefString( message->GetName(  )).ToString( ).c_str(  ) );

  if (message_router_->OnProcessMessageReceived(browser, source_process,
                                                message)) {
//    qDebug() << __FILE__ << __FUNCTION__ << __LINE__  << "handled" << message->GetName(  ).c_str(  );

    return true;
  }
//  qDebug() << __FILE__ << __FUNCTION__ << __LINE__  << "unhandled" << message->GetName(  ).c_str(  );

  // Check for messages from the client renderer.
  std::string message_name = message->GetName();
  if (message_name == client_renderer::kFocusedNodeChangedMessage) {
    // A message is sent from ClientRenderDelegate to tell us whether the
    // currently focused DOM node is editable. Use of |m_bFocusOnEditableField|
    // is redundant with CefKeyEvent.focus_on_editable_field in OnPreKeyEvent
    // but is useful for demonstration purposes.
    m_bFocusOnEditableField = message->GetArgumentList()->GetBool(0);
    return true;
  }

  return false;
}

void ClientHandler::OnBeforeContextMenu(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    CefRefPtr<CefContextMenuParams> params,
    CefRefPtr<CefMenuModel> model) {
  if ((params->GetTypeFlags() & (CM_TYPEFLAG_PAGE | CM_TYPEFLAG_FRAME)) != 0) {
    // Clear all context menus.
    //model->Clear();

#ifdef _DEBUGGING
    // Clear all context menus except MENU_ID_VIEW_SOURCE.
    int count = model->GetCount();
    for (int i = count - 1; i >=0; i--) {
        if (model->GetCommandIdAt(i) != MENU_ID_VIEW_SOURCE) {
            model->RemoveAt(i);
        }
    }

    // Add DevTools items to all context menus.
    model->AddItem(CLIENT_ID_SHOW_DEVTOOLS, "&Show DevTools");
    model->AddItem(CLIENT_ID_CLOSE_DEVTOOLS, "Close DevTools");
#else
    int count = model->GetCount();
    for (int i = count - 1; i >=0; i--) { 
      model->RemoveAt(i);
        }

#endif
  }
}

bool ClientHandler::OnContextMenuCommand(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    CefRefPtr<CefContextMenuParams> params,
    int command_id,
    EventFlags event_flags) {
  switch (command_id) {
    case CLIENT_ID_SHOW_DEVTOOLS:
      ShowDevTools(browser);
      return true;
    case CLIENT_ID_CLOSE_DEVTOOLS:
      CloseDevTools(browser);
      return true;
    default:  // Allow default handling, if any.
      return false;
  }
}

bool ClientHandler::OnConsoleMessage(CefRefPtr<CefBrowser> browser,
                                     const CefString& message,
                                     const CefString& source,
                                     int line) {
  return false;  // Output the message to the console.
}

void ClientHandler::OnBeforeDownload(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefDownloadItem> download_item,
    const CefString& suggested_name,
    CefRefPtr<CefBeforeDownloadCallback> callback) {
  REQUIRE_UI_THREAD();
}

void ClientHandler::OnDownloadUpdated(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefDownloadItem> download_item,
    CefRefPtr<CefDownloadItemCallback> callback) {
  REQUIRE_UI_THREAD();
  /*if (download_item->IsComplete()) {
  }*/
}

bool ClientHandler::OnDragEnter(CefRefPtr<CefBrowser> browser,
                                CefRefPtr<CefDragData> dragData,
                                DragOperationsMask mask) {
  REQUIRE_UI_THREAD();

  // Forbid dragging of link URLs.
  if (mask & DRAG_OPERATION_LINK)
    return true;

  return false;
}

void ClientHandler::OnRequestGeolocationPermission(
      CefRefPtr<CefBrowser> browser,
      const CefString& requesting_url,
      int request_id,
      CefRefPtr<CefGeolocationCallback> callback) {
  // Allow geolocation access from all websites.
  callback->Continue(true);
}


bool ClientHandler::OnPreKeyEvent(CefRefPtr<CefBrowser> browser,
                                  const CefKeyEvent& event,
                                  CefEventHandle os_event,
                                  bool* is_keyboard_shortcut) {
#ifdef CEF_EVENTS_TO_QAPPLICATION
  Qt::KeyboardModifiers modifiers;
  if (event.modifiers & EVENTFLAG_SHIFT_DOWN) modifiers |= Qt::ShiftModifier;
#ifdef Q_OS_MACX
  if (event.modifiers & EVENTFLAG_COMMAND_DOWN) modifiers |= Qt::ControlModifier;
  if (event.modifiers & EVENTFLAG_CONTROL_DOWN) modifiers |= Qt::MetaModifier;
#else
  if (event.modifiers & EVENTFLAG_CONTROL_DOWN) modifiers |= Qt::ControlModifier;
#endif
  if (event.modifiers & EVENTFLAG_ALT_DOWN) modifiers |= Qt::AltModifier;
  if (event.unmodified_character !='*') return false;
    
  MessageEvent *me = new MessageEvent( QKeyEvent( QEvent::KeyPress, event.unmodified_character, modifiers ) );
  listener_->OnMessageEvent( me );
  return true;
#else
  return false;
#endif
}

bool ClientHandler::OnBeforePopup(CefRefPtr<CefBrowser> browser,
                                  CefRefPtr<CefFrame> frame,
                                  const CefString& target_url,
                                  const CefString& target_frame_name,
                                  const CefPopupFeatures& popupFeatures,
                                  CefWindowInfo& windowInfo,
                                  CefRefPtr<CefClient>& client,
                                  CefBrowserSettings& settings,
                                  bool* no_javascript_access) {
//qDebug() << __FILE__ << __FUNCTION__ << __LINE__;
  if (browser->GetHost()->IsWindowRenderingDisabled()) {
    // Cancel popups in off-screen rendering mode.
    return true;
  }
  QString url( CefString( target_url ).ToString().c_str() );
  if (url.contains( VPN_URL_AFFILIATE )) {
    QRect rect = QApplication::activeWindow()->geometry();
    qDebug() << __FILE__ << __FUNCTION__ << __LINE__ << "Affiliate" << url;
    windowInfo.width = VPN_WINDOW_WIDTH;
    windowInfo.height = VPN_WINDOW_HEIGHT;
    windowInfo.x = rect.x() + rect.width() / 2 - windowInfo.width / 2;
    windowInfo.y = rect.y() + rect.height() / 2 - windowInfo.height / 2;
#ifdef GANALYTICS
    TAppGAnalytics::sendEvent( "Application", "VPNCreateWindow" );
#endif
    return false;
  }
#ifdef _DEBUGGING
  if (url == VPN_URL_BASE ) {
    qDebug() << __FILE__ << __FUNCTION__ << __LINE__ << "final" << url;
    MessageEvent *e = new MessageEvent( "PopupUrl", QVariantList() << QString( VPN_URL_DEBUG ) );
    listener_->OnMessageEvent( e );
    return true;
  }
#endif

  if (url.contains( VPN_URL_BASE )) {
    qDebug() << __FILE__ << __FUNCTION__ << __LINE__ << "openUrl" << url;
    QDesktopServices::openUrl( url );
//  QRect rect = QApplication::activeWindow()->geometry();
//  qDebug() << __FILE__ << __FUNCTION__ << __LINE__ << "Other" << url;
//  windowInfo.width = 800;
//  windowInfo.height = 600;
//  windowInfo.x = rect.x() + rect.width() / 2 - windowInfo.width / 2;
//  windowInfo.y = rect.y() + rect.height() / 2 - windowInfo.height / 2;
    return true;
  }
  if (url.contains( "http://" ) && url.contains( "@" )) {
    qDebug() << __FILE__ << __FUNCTION__ << __LINE__ << "final" << url;
    QVariantList message_args;
    message_args += url;

    MessageEvent *e = new MessageEvent( "PopupUrl", message_args );
    listener_->OnMessageEvent( e );
  }
  else qDebug() << __FILE__ << __FUNCTION__ << __LINE__ << "skipped" << url;
  return true;
}

void ClientHandler::OnAfterCreated(CefRefPtr<CefBrowser> browser) {
  REQUIRE_UI_THREAD();

  if (!message_router_) {
    // Create the browser-side router for query handling.
    CefMessageRouterConfig config;
    message_router_ = CefMessageRouterBrowserSide::Create(config);

    // Register handlers with the router.
    CreateMessageHandlers(message_handler_set_);
    MessageHandlerSet::const_iterator it = message_handler_set_.begin();
    for (; it != message_handler_set_.end(); ++it)
      message_router_->AddHandler(*(it), false);
  }

  AutoLock lock_scope(this);
  if (!m_Browser.get())   {
    // We need to keep the main child window, but not popup windows
    m_Browser = browser;
    m_BrowserId = browser->GetIdentifier();
    if (listener_) {
      listener_->OnAfterCreated();
    }
  } else if (browser->IsPopup()) {
    // Add to the list of popup browsers.
    m_PopupBrowsers.push_back(browser);

    // Give focus to the popup browser.
    browser->GetHost()->SetFocus(true);
  }

  m_BrowserCount++;
}

bool ClientHandler::DoClose(CefRefPtr<CefBrowser> browser) {
  REQUIRE_UI_THREAD();

  // Closing the main window requires special handling. See the DoClose()
  // documentation in the CEF header for a detailed destription of this
  // process.
  if (m_BrowserId == browser->GetIdentifier()) {
    // Notify the browser that the parent window is about to close.
    browser->GetHost()->ParentWindowWillClose();

    // Set a flag to indicate that the window close should be allowed.
    m_bIsClosing = true;
  }

  // Allow the close. For windowed browsers this will result in the OS close
  // event being sent.
  return false;
}

void ClientHandler::OnBeforeClose(CefRefPtr<CefBrowser> browser) {
  REQUIRE_UI_THREAD();

  message_router_->OnBeforeClose(browser);

  if (m_BrowserId == browser->GetIdentifier()) {
    // Free the browser pointer so that the browser can be destroyed
    m_Browser = NULL;
  } else if (browser->IsPopup()) {
    // Remove from the browser popup list.
    BrowserList::iterator bit = m_PopupBrowsers.begin();
    for (; bit != m_PopupBrowsers.end(); ++bit) {
      if ((*bit)->IsSame(browser)) {
        m_PopupBrowsers.erase(bit);
        break;
      }
    }
  }

  if (--m_BrowserCount == 0) {
    // All browser windows have closed.
    // Remove and delete message router handlers.
    MessageHandlerSet::const_iterator it = message_handler_set_.begin();
    for (; it != message_handler_set_.end(); ++it) {
      message_router_->RemoveHandler(*(it));
      delete *(it);
    }
    message_handler_set_.clear();
    message_router_ = NULL;

    NotifyAllBrowserClosed();
  }
}

void ClientHandler::OnLoadingStateChange(CefRefPtr<CefBrowser> browser,
                                         bool isLoading,
                                         bool canGoBack,
                                         bool canGoForward) {
  REQUIRE_UI_THREAD();

  //SetLoading(isLoading); // For overall browser load status.
  SetNavState(canGoBack, canGoForward);
}

void ClientHandler::OnLoadStart(CefRefPtr<CefBrowser> browser,
                                CefRefPtr<CefFrame> frame) {
  REQUIRE_UI_THREAD();

  if (m_BrowserId == browser->GetIdentifier() && frame->IsMain()) {
    SetLoading(true);
  }
}

void ClientHandler::OnLoadEnd(CefRefPtr<CefBrowser> browser,
                              CefRefPtr<CefFrame> frame,
                              int httpStatusCode) {
  REQUIRE_UI_THREAD();

  if (m_BrowserId == browser->GetIdentifier() && frame->IsMain()) {
    SetLoading(false);
  }
}

CefRefPtr<CefResourceHandler> ClientHandler::GetResourceHandler(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    CefRefPtr<CefRequest> request) {
  return NULL;
}

bool ClientHandler::OnQuotaRequest(CefRefPtr<CefBrowser> browser,
                                   const CefString& origin_url,
                                   int64 new_size,
                                   CefRefPtr<CefQuotaCallback> callback) {
  static const int64 max_size = 1024 * 1024 * 20;  // 20mb.

  // Grant the quota request if the size is reasonable.
  callback->Continue(new_size <= max_size);
  return true;
}

void ClientHandler::OnProtocolExecution(CefRefPtr<CefBrowser> browser,
                                        const CefString& url,
                                        bool& allow_os_execution) {
}

void ClientHandler::OnRenderProcessTerminated(CefRefPtr<CefBrowser> browser,
                                              TerminationStatus status) {
  message_router_->OnRenderProcessTerminated(browser);

  // Load the startup URL if that's not the website that we terminated on.
  CefRefPtr<CefFrame> frame = browser->GetMainFrame();
  std::string url = frame->GetURL();
  std::transform(url.begin(), url.end(), url.begin(), tolower);

  std::string startupURL = GetStartupURL();
  if (startupURL != "chrome://crash" && !url.empty() &&
      url.find(startupURL) != 0) {
    frame->LoadURL(startupURL);
  }
}

void ClientHandler::CloseAllBrowsers(bool force_close) {
  if (!CefCurrentlyOn(TID_UI)) {
    // Execute on the UI thread.
    CefPostTask(TID_UI,
        NewCefRunnableMethod(this, &ClientHandler::CloseAllBrowsers,
                             force_close));
    return;
  }

  if (!m_PopupBrowsers.empty()) {
    // Request that any popup browsers close.
    BrowserList::const_iterator it = m_PopupBrowsers.begin();
    for (; it != m_PopupBrowsers.end(); ++it)
      (*it)->GetHost()->CloseBrowser(force_close);
  }

  if (m_Browser.get()) {
    // Request that the main browser close.
    m_Browser->GetHost()->CloseBrowser(force_close);
  }
}

//void ClientHandler::ShowDevTools(CefRefPtr<CefBrowser> browser) {
//  std::string devtools_url = browser->GetHost()->GetDevToolsURL(true);
//  if (!devtools_url.empty()) {
//    if (m_OpenDevToolsURLs.find(devtools_url) ==
//               m_OpenDevToolsURLs.end()) {
//      // Open DevTools in a popup window.
//      m_OpenDevToolsURLs.insert(devtools_url);
//      browser->GetMainFrame()->ExecuteJavaScript(
//          "window.open('" +  devtools_url + "');", "about:blank", 0);
//    }
//  }
//}


void ClientHandler::ShowDevTools(CefRefPtr<CefBrowser> browser) {
  CefWindowInfo windowInfo;
  CefBrowserSettings settings;

#if defined(OS_WIN)
  windowInfo.SetAsPopup(browser->GetHost()->GetWindowHandle(), "DevTools");
#endif

  browser->GetHost()->ShowDevTools(windowInfo, this, settings);
}

void ClientHandler::CloseDevTools(CefRefPtr<CefBrowser> browser) {
  browser->GetHost()->CloseDevTools();
}

void ClientHandler::CreateMessageHandlers(MessageHandlerSet& handlers) {
  client_binding::CreateMessageHandlers( handlers );
}
