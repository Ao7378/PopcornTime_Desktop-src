#ifndef WEBVIEW_H
    #define WEBVIEW_H

    #include <QWebEngineView>
    #include <QSharedPointer>
    #include <QWebSocketServer>

class BrowserMainWindow;
class PopupWindow;

class WebEnginePage : public QWebEnginePage
{
    Q_OBJECT;

    signals:
    void loadingUrl( const QUrl& url );
    void popupUrl( QString url );

public:
    WebEnginePage( QObject *parent = 0 );

protected:
    virtual bool certificateError( const QWebEngineCertificateError& certificateError );

private:
    friend class WebEngineView;

    // set the webview mousepressedevent
    Qt::KeyboardModifiers m_keyboardModifiers;
    Qt::MouseButtons m_pressedButtons;
    QUrl m_loadingUrl;
    bool justOpened = true;
};

class WebEngineView : public QWebEngineView
{
    Q_OBJECT

public:
    WebEngineView( QWidget *parent = 0 );
    WebEnginePage* webPage() const { return m_page; }

    static QPointer<QObject> hostApp;

protected:
    void mousePressEvent( QMouseEvent *event );
    void mouseReleaseEvent( QMouseEvent *event );
    void contextMenuEvent( QContextMenuEvent *event );

    public slots:
    void executeJs( QString script );
    void attachJsObject( QString objName, QString bindScriptName, QObject *obj );

private:
    WebEnginePage *m_page;
    QSharedPointer<QWebSocketServer> server;
};

#endif
