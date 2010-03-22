#include <windows.h>
#include <dshow.h>

#include <stdio.h>

extern "C" {
void play_movie( HWND hwnd )
{
    IGraphBuilder *pGraph;
    IMediaControl *pMediaControl;
    IMediaEvent   *pEvent;
    IBasicVideo   *pBasic;
    IVideoWindow    *pVidWin = NULL;
    RECT grc;
    long width, height;

    CoInitialize(NULL);
    
    // Create the filter graph manager and query for interfaces.
    CoCreateInstance( 
	CLSID_FilterGraph, 
	NULL, 
	CLSCTX_INPROC_SERVER, 
	IID_IGraphBuilder, 
	(void **)&pGraph);
    
    pGraph->QueryInterface(IID_IVideoWindow, (void **)&pVidWin);

    pGraph->QueryInterface(IID_IMediaControl, (void **)&pMediaControl);
    pGraph->QueryInterface(IID_IMediaEvent, (void **)&pEvent);
    pGraph->QueryInterface(IID_IBasicVideo, (void**)&pBasic );

    // Build the graph. IMPORTANT: Change string to a file on your system.
    pGraph->RenderFile(L"e:\\alpha\\running.avi", NULL);

    pBasic->GetVideoSize( &width, &height );  
    printf( "video frames are %d x %d\n", width, height );

    pVidWin->put_Owner((OAHWND)hwnd);
    pVidWin->put_WindowStyle(WS_CHILD | WS_CLIPSIBLINGS);

    GetClientRect( hwnd, &grc );
    pVidWin->SetWindowPosition(10, 10, width, height);
    printf( "window is %d x %d\n", grc.right, grc.bottom );

    // Run the graph.
    pMediaControl->Run();

    // Wait for completion. 
    long evCode;
    pEvent->WaitForCompletion(INFINITE, &evCode);

    pVidWin->put_Visible(OAFALSE);
    pVidWin->put_Owner(NULL);
    
    // Clean up.
    pBasic->Release();
    pVidWin->Release();
    pMediaControl->Release();
    pEvent->Release();
    pGraph->Release();
    CoUninitialize();
}
}
