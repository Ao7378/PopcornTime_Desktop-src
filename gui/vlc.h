#ifndef __VLC_H_INCL__
    #define __VLC_H_INCL__

    #include <QString>
    #include <QObject>

class VlcInstance;
class VlcMedia;
class VlcVideoDelegate;
class VMediaPlayer;
class QStringList;

class VLC : public QObject
{
    Q_OBJECT

public:
    VLC( QObject *parent = 0 );
    ~VLC();
    static inline bool isInitialized() { return _instance && _player; }
    static bool hasDebugFile();
    static void resetInstance( QStringList params );
    static VLC* VLCObject() { return object; }


    static VMediaPlayer *_player;
    static QString defaultFile;

    signals:
    void mediaPlayerReplaced();
    
private:
    friend class VMediaPlayer;
    static VLC *object;
    static VlcInstance *_instance;
};

#endif // __VLC_H_INCL__
