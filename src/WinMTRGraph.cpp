//*****************************************************************************
// FILE:            WinMTRGraph.cpp
//
// DESCRIPTION:     Implementation of modern graph visualization
//
//*****************************************************************************

#include "WinMTRGlobal.h"
#include "WinMTRGraph.h"
#include <algorithm>

using namespace Gdiplus;

// Define color palette for different hops (visually distinct colors)
const Color WinMTRGraph::HopColors[MAX_GRAPH_HOPS] = {
    Color(255, 0, 114, 189),      // Blue
    Color(255, 217, 83, 25),      // Orange
    Color(255, 237, 177, 32),     // Yellow
    Color(255, 126, 47, 142),     // Purple
    Color(255, 119, 172, 48),     // Green
    Color(255, 162, 20, 47),      // Dark Red
    Color(255, 77, 190, 238),     // Cyan
    Color(255, 0, 158, 115),      // Teal
    Color(255, 204, 121, 167),    // Pink
    Color(255, 230, 159, 0),      // Gold
    Color(255, 86, 180, 233),     // Sky Blue
    Color(255, 213, 94, 0),       // Vermillion
    Color(255, 0, 0, 128),        // Navy
    Color(255, 128, 0, 128),      // Magenta
    Color(255, 128, 128, 0),      // Olive
    Color(255, 0, 128, 128),      // Teal Dark
    Color(255, 255, 0, 255),      // Fuchsia
    Color(255, 0, 255, 255),      // Aqua
    Color(255, 192, 192, 192),    // Silver
    Color(255, 128, 0, 0),        // Maroon
    Color(255, 0, 128, 0),        // Dark Green
    Color(255, 0, 0, 255),        // Pure Blue
    Color(255, 255, 0, 0),        // Red
    Color(255, 255, 165, 0),      // Orange Bright
    Color(255, 75, 0, 130),       // Indigo
    Color(255, 238, 130, 238),    // Violet
    Color(255, 165, 42, 42),      // Brown
    Color(255, 244, 164, 96),     // Sandy Brown
    Color(255, 46, 139, 87),      // Sea Green
    Color(255, 218, 112, 214)     // Orchid
};

BEGIN_MESSAGE_MAP(WinMTRGraph, CWnd)
    ON_WM_PAINT()
    ON_WM_SIZE()
    ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

WinMTRGraph::WinMTRGraph()
{
    m_autoScale = TRUE;
    m_maxRTT = 500;  // Default max RTT in ms
    m_gdiplusToken = 0;
    m_selectedHop = -1;  // Show all hops by default
    m_maxSamples = MAX_GRAPH_SAMPLES;  // Default to 5 minutes
    m_currentVisibleHops = 0;  // Will be updated during rendering

    // Initialize GDI+
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);
}

WinMTRGraph::~WinMTRGraph()
{
    if (m_gdiplusToken != 0) {
        GdiplusShutdown(m_gdiplusToken);
    }
}

BOOL WinMTRGraph::Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID)
{
    static CString className = AfxRegisterWndClass(
        CS_HREDRAW | CS_VREDRAW,
        ::LoadCursor(NULL, IDC_ARROW),
        (HBRUSH)(COLOR_WINDOW + 1),
        NULL
    );

    return CWnd::CreateEx(
        WS_EX_CLIENTEDGE,
        className,
        _T("Graph"),
        dwStyle,
        rect,
        pParentWnd,
        nID
    );
}

void WinMTRGraph::AddSample(const int* rttValues, const char* const* hostnames, int numHops)
{
    RTTSample sample;
    sample.timestamp = GetTickCount();
    sample.validHops = numHops;

    for (int i = 0; i < MAX_GRAPH_HOPS; i++) {
        if (i < numHops) {
            sample.rtt[i] = rttValues[i];
            if (hostnames && hostnames[i]) {
                strncpy_s(sample.hostnames[i], 255, hostnames[i], _TRUNCATE);
            } else {
                sample.hostnames[i][0] = '\0';
            }
        } else {
            sample.rtt[i] = -1;
            sample.hostnames[i][0] = '\0';
        }
    }

    m_samples.push_back(sample);

    // Keep only the last m_maxSamples
    while ((int)m_samples.size() > m_maxSamples) {
        m_samples.pop_front();
    }

    Invalidate(FALSE);
}

void WinMTRGraph::ClearData()
{
    m_samples.clear();
    Invalidate();
}

void WinMTRGraph::OnPaint()
{
    CPaintDC dc(this);
    CRect clientRect;
    GetClientRect(&clientRect);

    // Create bitmap for double buffering
    CDC memDC;
    memDC.CreateCompatibleDC(&dc);
    CBitmap bitmap;
    bitmap.CreateCompatibleBitmap(&dc, clientRect.Width(), clientRect.Height());
    CBitmap* pOldBitmap = memDC.SelectObject(&bitmap);

    // Create GDI+ Graphics object from memory DC
    Graphics graphics(memDC.m_hDC);
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);

    DrawGraph(graphics, clientRect);

    // Copy to screen
    dc.BitBlt(0, 0, clientRect.Width(), clientRect.Height(), &memDC, 0, 0, SRCCOPY);
    memDC.SelectObject(pOldBitmap);
}

void WinMTRGraph::DrawGraph(Graphics& graphics, const CRect& clientRect)
{
    // Fill background
    SolidBrush bgBrush(Color(255, 32, 32, 32));  // Dark background
    graphics.FillRectangle(&bgBrush, 0, 0, clientRect.Width(), clientRect.Height());

    if (m_samples.empty()) {
        // Draw "No Data" message
        Font font(L"Arial", 16);
        StringFormat format;
        format.SetAlignment(StringAlignmentCenter);
        format.SetLineAlignment(StringAlignmentCenter);
        SolidBrush textBrush(Color(255, 128, 128, 128));
        RectF rect(0, 0, (REAL)clientRect.Width(), (REAL)clientRect.Height());
        graphics.DrawString(L"No Data - Start Tracing", -1, &font, rect, &format, &textBrush);
        return;
    }

    // Calculate graph area (leave space for legend on right only when showing multiple hops)
    CRect graphRect = clientRect;

    // When a single hop is selected, use full width. Otherwise, reserve space for legend.
    if (m_selectedHop < 0) {
        graphRect.right -= 260;  // Space for wider legend with hostnames
        graphRect.DeflateRect(40, 30, 10, 40);  // Margins
    } else {
        // Single hop selected - use full width
        graphRect.DeflateRect(40, 30, 40, 40);  // Margins on all sides
    }

    DrawGrid(graphics, graphRect);
    DrawData(graphics, graphRect);

    // Only draw legend when showing all hops
    if (m_selectedHop < 0) {
        DrawLegend(graphics, clientRect);
    }
}

void WinMTRGraph::DrawGrid(Graphics& graphics, const CRect& graphRect)
{
    Pen gridPen(Color(255, 64, 64, 64), 1);
    Pen axisPen(Color(255, 128, 128, 128), 2);
    Font font(L"Arial", 9);
    SolidBrush textBrush(Color(255, 200, 200, 200));
    StringFormat format;

    // Determine max RTT for Y-axis
    int maxRTT = m_maxRTT;
    if (m_autoScale && !m_samples.empty()) {
        maxRTT = 0;
        for (const auto& sample : m_samples) {
            for (int i = 0; i < sample.validHops; i++) {
                if (sample.rtt[i] > maxRTT) {
                    maxRTT = sample.rtt[i];
                }
            }
        }
        maxRTT = (int)(maxRTT * 1.1);  // Add 10% margin
        if (maxRTT < 50) maxRTT = 50;
    }

    // Draw vertical grid lines (time)
    int numVLines = 10;
    for (int i = 0; i <= numVLines; i++) {
        int x = graphRect.left + (graphRect.Width() * i / numVLines);
        graphics.DrawLine(&gridPen, x, graphRect.top, x, graphRect.bottom);
    }

    // Draw horizontal grid lines (RTT)
    int numHLines = 10;
    for (int i = 0; i <= numHLines; i++) {
        int y = graphRect.bottom - (graphRect.Height() * i / numHLines);
        graphics.DrawLine(&gridPen, graphRect.left, y, graphRect.right, y);

        // Draw Y-axis labels
        CString label;
        label.Format(_T("%d ms"), maxRTT * i / numHLines);
        RectF labelRect((REAL)(graphRect.left - 35), (REAL)(y - 8), 30, 16);
        format.SetAlignment(StringAlignmentFar);
        CT2W wLabel(label);
        graphics.DrawString(wLabel, -1, &font, labelRect, &format, &textBrush);
    }

    // Draw axes
    graphics.DrawLine(&axisPen, graphRect.left, graphRect.bottom, graphRect.right, graphRect.bottom);  // X-axis
    graphics.DrawLine(&axisPen, graphRect.left, graphRect.top, graphRect.left, graphRect.bottom);      // Y-axis

    // Draw X-axis label
    format.SetAlignment(StringAlignmentCenter);
    RectF xLabelRect((REAL)graphRect.left, (REAL)(graphRect.bottom + 10), (REAL)graphRect.Width(), 20);
    graphics.DrawString(L"Time (samples)", -1, &font, xLabelRect, &format, &textBrush);

    // Draw Y-axis label (rotated)
    GraphicsState state = graphics.Save();
    graphics.TranslateTransform((REAL)(graphRect.left - 35), (REAL)(graphRect.top + graphRect.Height() / 2));
    graphics.RotateTransform(-90);
    format.SetAlignment(StringAlignmentCenter);
    RectF yLabelRect(-50, 0, 100, 20);
    graphics.DrawString(L"RTT (ms)", -1, &font, yLabelRect, &format, &textBrush);
    graphics.Restore(state);
}

void WinMTRGraph::DrawData(Graphics& graphics, const CRect& graphRect)
{
    if (m_samples.size() < 2) return;

    // Determine which hops have at least one response and a valid hostname
    bool hopHasData[MAX_GRAPH_HOPS] = {false};
    bool hopHasValidHostname[MAX_GRAPH_HOPS] = {false};

    for (const auto& sample : m_samples) {
        for (int i = 0; i < sample.validHops; i++) {
            if (sample.rtt[i] >= 0) {
                hopHasData[i] = true;
            }
            // Check if this hop has a valid hostname (not empty = not 100% loss)
            if (sample.hostnames[i][0] != '\0') {
                hopHasValidHostname[i] = true;
            }
        }
    }

    // Count visible hops (those with data and valid hostnames) for color spacing
    // and create a mapping from hop index to visible position
    m_currentVisibleHops = 0;
    int maxValidHops = 0;
    for (const auto& sample : m_samples) {
        if (sample.validHops > maxValidHops) {
            maxValidHops = sample.validHops;
        }
    }

    int hopToPositionMap[MAX_GRAPH_HOPS];
    for (int i = 0; i < MAX_GRAPH_HOPS; i++) {
        hopToPositionMap[i] = -1;  // Not visible
    }

    for (int i = 0; i < maxValidHops; i++) {
        if (hopHasData[i] && hopHasValidHostname[i]) {
            hopToPositionMap[i] = m_currentVisibleHops;
            m_currentVisibleHops++;
        }
    }

    // Determine max RTT for scaling (only from selected hop if one is selected)
    int maxRTT = m_maxRTT;
    if (m_autoScale) {
        maxRTT = 0;
        for (const auto& sample : m_samples) {
            for (int i = 0; i < sample.validHops; i++) {
                // Only consider this hop if: it has data, valid hostname, and either we're showing all or it's selected
                if (hopHasData[i] && hopHasValidHostname[i] && (m_selectedHop < 0 || m_selectedHop == i)) {
                    if (sample.rtt[i] > maxRTT) {
                        maxRTT = sample.rtt[i];
                    }
                }
            }
        }
        maxRTT = (int)(maxRTT * 1.1);
        if (maxRTT < 50) maxRTT = 50;
    }

    // Draw lines for each hop
    for (int hop = 0; hop < maxValidHops; hop++) {
        // Skip hops without data
        if (!hopHasData[hop]) continue;

        // Skip hops without valid hostname (100% packet loss)
        if (!hopHasValidHostname[hop]) continue;

        // Skip if a specific hop is selected and this isn't it
        if (m_selectedHop >= 0 && m_selectedHop != hop) continue;

        // Get color using position mapping for maximum contrast
        int hopPosition = hopToPositionMap[hop];
        Color lineColor = GetHopColorByPosition(hopPosition, m_currentVisibleHops);
        Pen pen(lineColor, 2.0f);
        pen.SetLineJoin(LineJoinRound);

        std::vector<PointF> points;

        // Calculate start index to show only the last m_maxSamples
        size_t startIdx = 0;
        if ((int)m_samples.size() > m_maxSamples) {
            startIdx = m_samples.size() - m_maxSamples;
        }

        for (size_t i = startIdx; i < m_samples.size(); i++) {
            if (hop < m_samples[i].validHops && m_samples[i].rtt[hop] >= 0) {
                float x = graphRect.left + (graphRect.Width() * (float)(i - startIdx) / (float)(m_maxSamples - 1));
                float y = graphRect.bottom - (graphRect.Height() * (float)m_samples[i].rtt[hop] / (float)maxRTT);

                // Clamp Y to graph bounds
                if (y < graphRect.top) y = (float)graphRect.top;
                if (y > graphRect.bottom) y = (float)graphRect.bottom;

                points.push_back(PointF(x, y));
            } else {
                // Break in data - draw what we have and start new segment
                if (points.size() > 1) {
                    graphics.DrawLines(&pen, &points[0], (INT)points.size());
                }
                points.clear();
            }
        }

        // Draw remaining points
        if (points.size() > 1) {
            graphics.DrawLines(&pen, &points[0], (INT)points.size());
        }
    }
}

void WinMTRGraph::DrawLegend(Graphics& graphics, const CRect& clientRect)
{
    if (m_samples.empty()) return;

    Font font(L"Arial", 8);
    SolidBrush textBrush(Color(255, 200, 200, 200));

    // Determine which hops have at least one response and get their latest hostname
    bool hopHasData[MAX_GRAPH_HOPS] = {false};
    bool hopHasValidHostname[MAX_GRAPH_HOPS] = {false};
    char latestHostname[MAX_GRAPH_HOPS][255];
    for (int i = 0; i < MAX_GRAPH_HOPS; i++) {
        latestHostname[i][0] = '\0';
    }

    // Iterate from most recent sample backwards
    for (auto it = m_samples.rbegin(); it != m_samples.rend(); ++it) {
        for (int i = 0; i < it->validHops; i++) {
            if (it->rtt[i] >= 0 && !hopHasData[i]) {
                hopHasData[i] = true;
                if (it->hostnames[i][0] != '\0') {
                    strncpy_s(latestHostname[i], 255, it->hostnames[i], _TRUNCATE);
                    hopHasValidHostname[i] = true;
                }
            }
        }
    }

    // Find max valid hops
    int maxValidHops = 0;
    for (const auto& sample : m_samples) {
        if (sample.validHops > maxValidHops) {
            maxValidHops = sample.validHops;
        }
    }

    // Count visible hops and create position mapping (same as in DrawData)
    int visibleHopCount = 0;
    int hopToPositionMap[MAX_GRAPH_HOPS];
    for (int i = 0; i < MAX_GRAPH_HOPS; i++) {
        hopToPositionMap[i] = -1;
    }
    for (int i = 0; i < maxValidHops; i++) {
        if (hopHasData[i] && hopHasValidHostname[i]) {
            hopToPositionMap[i] = visibleHopCount;
            visibleHopCount++;
        }
    }

    int legendX = clientRect.right - 250;  // Wider legend for hostnames
    int legendY = 40;
    int lineHeight = 18;

    // Draw legend title
    RectF titleRect((REAL)legendX, (REAL)(legendY - 20), 240, 16);
    graphics.DrawString(L"Active Hops", -1, &font, titleRect, NULL, &textBrush);

    // Draw legend items (only for hops with data and valid hostnames)
    int itemCount = 0;
    for (int i = 0; i < maxValidHops && itemCount < 20; i++) {
        // Skip hops without data
        if (!hopHasData[i]) continue;

        // Skip hops without valid hostname (100% packet loss)
        if (latestHostname[i][0] == '\0') continue;

        // Skip if a specific hop is selected and this isn't it
        if (m_selectedHop >= 0 && m_selectedHop != i) continue;

        int y = legendY + itemCount * lineHeight;

        // Get color using position mapping for maximum contrast
        int hopPosition = hopToPositionMap[i];
        Color lineColor = GetHopColorByPosition(hopPosition, visibleHopCount);
        Pen pen(lineColor, 3.0f);
        graphics.DrawLine(&pen, legendX, y + 6, legendX + 20, y + 6);

        // Draw hostname
        CString label = latestHostname[i];

        RectF labelRect((REAL)(legendX + 25), (REAL)y, 220, 16);
        CT2W wLabel(label);
        graphics.DrawString(wLabel, -1, &font, labelRect, NULL, &textBrush);

        itemCount++;
    }

    if (itemCount == 0) {
        // No hops with data yet
        RectF noDataRect((REAL)legendX, (REAL)legendY, 240, 16);
        graphics.DrawString(L"Waiting for responses...", -1, &font, noDataRect, NULL, &textBrush);
    }
}

BOOL WinMTRGraph::CopyToClipboard()
{
    CRect clientRect;
    GetClientRect(&clientRect);

    // Create a memory DC and bitmap
    CDC* pDC = GetDC();
    CDC memDC;
    memDC.CreateCompatibleDC(pDC);
    CBitmap bitmap;
    bitmap.CreateCompatibleBitmap(pDC, clientRect.Width(), clientRect.Height());
    CBitmap* pOldBitmap = memDC.SelectObject(&bitmap);

    // Create GDI+ Graphics and draw the graph
    Graphics graphics(memDC.m_hDC);
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);
    DrawGraph(graphics, clientRect);

    // Get bitmap handle
    HBITMAP hBitmap = (HBITMAP)bitmap.Detach();

    // Copy to clipboard
    BOOL success = FALSE;
    if (OpenClipboard()) {
        EmptyClipboard();
        if (SetClipboardData(CF_BITMAP, hBitmap)) {
            success = TRUE;
        }
        CloseClipboard();
    }

    // Cleanup
    memDC.SelectObject(pOldBitmap);
    ReleaseDC(pDC);

    if (!success && hBitmap) {
        DeleteObject(hBitmap);
    }

    return success;
}

BOOL WinMTRGraph::ExportToFile(const CString& filePath)
{
    CRect clientRect;
    GetClientRect(&clientRect);

    // Create a bitmap in memory
    Bitmap bitmap(clientRect.Width(), clientRect.Height(), PixelFormat32bppARGB);
    Graphics graphics(&bitmap);
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);

    // Draw the graph
    DrawGraph(graphics, clientRect);

    // Get encoder CLSID for PNG
    CLSID pngClsid;
    UINT num = 0;
    UINT size = 0;
    GetImageEncodersSize(&num, &size);
    if (size == 0) return FALSE;

    ImageCodecInfo* pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
    if (pImageCodecInfo == NULL) return FALSE;

    GetImageEncoders(num, size, pImageCodecInfo);
    BOOL found = FALSE;
    for (UINT j = 0; j < num; ++j) {
        if (wcscmp(pImageCodecInfo[j].MimeType, L"image/png") == 0) {
            pngClsid = pImageCodecInfo[j].Clsid;
            found = TRUE;
            break;
        }
    }
    free(pImageCodecInfo);

    if (!found) return FALSE;

    // Save to file
    CT2W wFilePath(filePath);
    Status status = bitmap.Save(wFilePath, &pngClsid, NULL);

    return (status == Ok);
}

int WinMTRGraph::GetColorIndexForHop(int hopPosition, int totalVisibleHops)
{
    // For maximum contrast, space colors evenly across the palette
    // based on the number of visible hops
    if (totalVisibleHops <= 1) {
        return 0;  // Just use the first color
    }

    // Calculate the spacing to maximize color contrast
    // We have MAX_GRAPH_HOPS colors in our palette
    int spacing = MAX_GRAPH_HOPS / totalVisibleHops;
    int colorIndex = (hopPosition * spacing) % MAX_GRAPH_HOPS;

    return colorIndex;
}

Color WinMTRGraph::GetHopColorByPosition(int hopPosition, int totalVisibleHops)
{
    if (hopPosition >= 0 && totalVisibleHops > 0) {
        int colorIndex = GetColorIndexForHop(hopPosition, totalVisibleHops);
        if (colorIndex >= 0 && colorIndex < MAX_GRAPH_HOPS) {
            return HopColors[colorIndex];
        }
    }
    return HopColors[0];  // Default to first color
}

Color WinMTRGraph::GetHopColor(int hopIndex)
{
    // This is used by legacy code or when position mapping isn't available
    if (hopIndex >= 0 && hopIndex < MAX_GRAPH_HOPS) {
        return HopColors[hopIndex];
    }
    return Color(255, 128, 128, 128);  // Default gray
}

void WinMTRGraph::OnSize(UINT nType, int cx, int cy)
{
    CWnd::OnSize(nType, cx, cy);
    Invalidate();
}

BOOL WinMTRGraph::OnEraseBkgnd(CDC* pDC)
{
    return TRUE;  // Prevent flicker
}
