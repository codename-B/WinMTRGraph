//*****************************************************************************
// FILE:            WinMTRGraph.h
//
// DESCRIPTION:     Modern graph visualization component for real-time RTT
//                  display with one line per hop (similar to PingPlotter)
//
//*****************************************************************************

#ifndef WINMTRGRAPH_H_
#define WINMTRGRAPH_H_

#include <vector>
#include <deque>
#include <gdiplus.h>

#pragma comment(lib, "gdiplus.lib")

#define MAX_GRAPH_SAMPLES 300  // 5 minutes at 1 sample/second
#define MAX_GRAPH_HOPS 30

struct RTTSample {
    DWORD timestamp;  // GetTickCount() value
    int rtt[MAX_GRAPH_HOPS];  // RTT for each hop (-1 = no data)
    char hostnames[MAX_GRAPH_HOPS][255];  // Hostname for each hop
    int validHops;
};

//*****************************************************************************
// CLASS:  WinMTRGraph
//
// Modern graph control that displays RTT over time with colored lines
//*****************************************************************************
class WinMTRGraph : public CWnd
{
public:
    WinMTRGraph();
    virtual ~WinMTRGraph();

    BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);

    // Add a new RTT sample for all hops with hostnames
    void AddSample(const int* rttValues, const char* const* hostnames, int numHops);

    // Clear all graph data
    void ClearData();

    // Set auto-scale mode
    void SetAutoScale(BOOL autoScale) { m_autoScale = autoScale; }

    // Set max RTT for manual scale
    void SetMaxRTT(int maxRTT) { m_maxRTT = maxRTT; }

    // Set selected hop (or -1 for all hops)
    void SetSelectedHop(int hopIndex) { m_selectedHop = hopIndex; Invalidate(); }

    // Copy graph to clipboard as bitmap
    BOOL CopyToClipboard();

    // Export graph to file
    BOOL ExportToFile(const CString& filePath);

    // Set time resolution (number of samples to display)
    void SetTimeResolution(int maxSamples) {
        if (maxSamples > 0 && maxSamples <= 86400) {  // Max 24 hours at 1/sec
            m_maxSamples = maxSamples;
            Invalidate();
        }
    }

protected:
    afx_msg void OnPaint();
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);

    DECLARE_MESSAGE_MAP()

private:
    void DrawGraph(Gdiplus::Graphics& graphics, const CRect& clientRect);
    void DrawGrid(Gdiplus::Graphics& graphics, const CRect& graphRect);
    void DrawData(Gdiplus::Graphics& graphics, const CRect& graphRect);
    void DrawLegend(Gdiplus::Graphics& graphics, const CRect& clientRect);

    Gdiplus::Color GetHopColor(int hopIndex);
    Gdiplus::Color GetHopColorByPosition(int hopPosition, int totalVisibleHops);
    int GetColorIndexForHop(int hopPosition, int totalVisibleHops);

    std::deque<RTTSample> m_samples;
    BOOL m_autoScale;
    int m_maxRTT;
    ULONG_PTR m_gdiplusToken;
    int m_selectedHop;  // -1 = show all, >= 0 = show only selected hop
    int m_maxSamples;   // Maximum samples to display (time resolution)
    int m_currentVisibleHops;  // Number of currently visible hops (for color spacing)

    // Colors for different hops (up to 30)
    static const Gdiplus::Color HopColors[MAX_GRAPH_HOPS];
};

#endif // WINMTRGRAPH_H_
