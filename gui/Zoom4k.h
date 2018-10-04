#ifndef __ZOOM4K_H_INCL__
    #define __ZOOM4K_H_INCL__

class Zoom4k
{
public:
    static int zoomFactorPercent();
    static int zoomFactorInt() { return zoomFactorPercent() / 100; }
    static bool is4k() { return m_4k; }

private:
    static void calculate();
    static int currentZoomFactorPercent, currentWidth;
    static bool m_4k;
};

#endif // __ZOOM4K_H_INCL__
