WinMTR Graph
==============
**WinMTR Graph** is a fork of [WinMTR (Redux)](https://github.com/White-Tiger/WinMTR) which itself is a fork of [Appnor's WinMTR](http://winmtr.net/) ([sourceforge](http://sourceforge.net/projects/winmtr/)).
It adds graph visualization to WinMTR.
[graph.png]

### Download (binaries)
* [**view all available**](https://github.com/codename-B/WinMTRGraph/releases)

#### Differences to [WinMTR](http://winmtr.net/) 0.98
- `[x]` - removed Windows 2000 support <br>
- `[x]` + added IPv6 support <br>
- `[x]` + clickable entries when stopped.. *(why the heck wasn't it possible before?)* <br>
- `[x]` * added start delay of about 30ms for each hop *(870ms before the 30th hop gets queried) <br>
this should improve performance and reduces network load* <br>
- `[x]` ! fixed trace list freeze *(list didn't update while tracing, happens when tracing just one hop)* <br>
- `[x]` * theme support *(more fancy look :P)* <br>
- `[x]` * new icon <br>
- `[x]` * graph for visualization <br>
- `[ ]` ! CTRL+A works for host input <br>
- `[ ]` + host history: pressing del key or right mouse will remove selected entry <br>

### Requirements
* Windows Vista+
* Microsoft Visual C++ 2010 Redistributables
([32bit](http://microsoft.com/en-us/download/details.aspx?id=5555) |
[64bit](http://microsoft.com/en-us/download/details.aspx?id=14632)) or use static build
