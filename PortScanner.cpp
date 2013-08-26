#define SCANBTN    101

#include <winsock.h>
#include <commctrl.h>
#include <stdio.h>

LRESULT CALLBACK WindowProcedure (HWND, UINT, WPARAM, LPARAM);

HTREEITEM addRootToTree(HWND hTV, char* itemText);
HTREEITEM addToRoot(HWND hTV, HTREEITEM hItem, char* itemText, int itemLevel);

void CreateToolTip(HWND hwnd, char* cTip);
void PopulateTree();
void PopulateRebar();
void PopulateStatus();
void UpdateBars();
void StartScan();
DWORD scanner(int start);
void reset();
void StopScan();

char szClassName[ ] = "PortScanner";
HINSTANCE hInstance;
HWND hIP, hScan, hTree, hRebar, hStatus, hProg;
HTREEITEM Open, Closed;

char *ip;
int totalScan = 0;
BOOL done;
HANDLE hThread[656];
int finished = 0;

int WINAPI WinMain (HINSTANCE hThisInstance, HINSTANCE hPrevInstance, LPSTR lpszArgument, int nFunsterStil)
{
    HWND hwnd;
    MSG messages;
    WNDCLASSEX wincl;
    WSADATA wsaData;
    
    done = true;   
    WSAStartup(MAKEWORD(2,2), &wsaData);
    InitCommonControls();
    
    hInstance = hThisInstance;
    
    wincl.hInstance = hThisInstance;
    wincl.lpszClassName = szClassName;
    wincl.lpfnWndProc = WindowProcedure;
    wincl.style = CS_DBLCLKS; 
    wincl.cbSize = sizeof (WNDCLASSEX);

    wincl.hIcon = LoadIcon (NULL, IDI_APPLICATION);
    wincl.hIconSm = LoadIcon (NULL, IDI_APPLICATION);
    wincl.hCursor = LoadCursor (NULL, IDC_ARROW);
    wincl.lpszMenuName = NULL; 
    wincl.cbClsExtra = 0;
    wincl.cbWndExtra = 0; 
    wincl.hbrBackground = (HBRUSH) COLOR_BACKGROUND;

    if (!RegisterClassEx (&wincl)){ return 0; }

    hwnd = CreateWindowEx (0, szClassName,"Port Scanner",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,544,375,HWND_DESKTOP,NULL,hThisInstance,NULL);

    ShowWindow (hwnd, nFunsterStil);

    while (GetMessage (&messages, NULL, 0, 0))
    {
        TranslateMessage(&messages);
        DispatchMessage(&messages);
    }
    
    WSACleanup();
    return messages.wParam;
}

LRESULT CALLBACK WindowProcedure (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) 
    {
        case WM_CREATE: 
             hIP = CreateWindowEx(0, WC_IPADDRESS, 0, WS_CHILD | WS_VISIBLE, 0, 0, 150, 21, hwnd, NULL, hInstance, NULL);
             SendMessage(hIP, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), 0);
             SendMessage(hIP, IPM_SETADDRESS, 0,MAKEIPADDRESS(127,0,0,1));
             
             hScan = CreateWindowEx(0, "button", "Scan", WS_CHILD | WS_VISIBLE, 0, 0, 50, 21, hwnd, (HMENU)SCANBTN, hInstance, NULL);
             SendMessage(hScan, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), 0);
             CreateToolTip(hScan, "Start The Scan");
             
             hTree = CreateWindowEx(0, WC_TREEVIEW, 0, WS_CHILD | WS_VISIBLE | WS_BORDER | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS, 10, 45, 250, 250, hwnd, NULL, hInstance, NULL);
             CreateToolTip(hTree, "Results Of Scan Go Here");
             PopulateTree();
             
             hProg = CreateWindowEx(0, PROGRESS_CLASS, 0, WS_CHILD | WS_VISIBLE | PBS_SMOOTH , 0, 0, 200, 18, hwnd, NULL, hInstance, NULL);
             CreateToolTip(hProg, "Progress Of Scan");
             SendMessage(hProg, PBM_SETRANGE32, 0, 65535);
             SendMessage(hProg, PBM_SETSTEP, 1, 0);
             
             hRebar = CreateWindowEx(WS_EX_TOOLWINDOW, REBARCLASSNAME, 0, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | RBS_VARHEIGHT | CCS_NODIVIDER | RBS_BANDBORDERS, 0,0,0,0, hwnd, NULL, hInstance, NULL);
             PopulateRebar();
             
             hStatus = CreateWindowEx(0, STATUSCLASSNAME, 0, WS_CHILD | WS_VISIBLE, 0, 0, 100, 21, hwnd, NULL, hInstance, NULL);
             PopulateStatus();
             break;
        case WM_COMMAND:
             switch(LOWORD(wParam))
             {
                 case SCANBTN:
                      DWORD ipAddr;
                      LRESULT SM = SendMessage(hIP, IPM_GETADDRESS, 0,(LPARAM)(LPDWORD)&ipAddr);

                	  ipAddr = htonl(ipAddr);
                	  struct in_addr inIPAddr;
                      inIPAddr.s_addr = ipAddr;
                      ip = inet_ntoa(inIPAddr);
                      DWORD dummy;
                      
                      RECT rc;    
                      GetClientRect (hwnd, &rc);
                      rc.bottom = rc.bottom / 2;
                      rc.left = (rc.left / 2) + 150;
                         
                      if(done == true)
                      {
                         reset();
                         done = false;     
                         CreateThread(0, 0, (LPTHREAD_START_ROUTINE)StartScan, 0, 0, &dummy);
                         InvalidateRect(hwnd, &rc, false); 
                      }
                      else if(done == false)
                      {
                           int x = MessageBox(hwnd, "Stop Scan?", "Scanning...", MB_YESNO|MB_ICONWARNING);
                           if(x == IDYES) StopScan();
                           InvalidateRect(hwnd, &rc, false); 
                      }
                      break;
             }
             break;
        case WM_PAINT:
             PAINTSTRUCT ps;
             HDC hdc;
             hdc = BeginPaint(hwnd, &ps);
             SetBkMode(hdc, TRANSPARENT);
             
             RECT rc;    
             GetClientRect (hwnd, &rc);
             rc.bottom = rc.bottom / 2;
             rc.left = (rc.left / 2) + 150;
                 
             if(done == false)
             {
                 DrawText (hdc, "Scanning...", -1, &rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
             }else if(done == true && finished > 0){
                 DrawText (hdc, "Finished!", -1, &rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
             }
             
             EndPaint(hwnd, &ps);
             break;
        case WM_DESTROY:
             PostQuitMessage (0);
             break;
        default: 
             return DefWindowProc (hwnd, message, wParam, lParam);
    }

    return 0;
}

void StopScan()
{
     for(int i = 0; i < 656; i ++) TerminateThread(hThread[i], 0);
     done = true;
}

void StartScan()
{    
     DWORD dummy;
     
     int i = 0, x = 0;
     for(i = 0; i < 656; )
     {
         hThread[i] = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)scanner, (void*)x, 0, &dummy);
         if(hThread[i] != NULL)
         {
            i++;
            x += 100;
            WaitForSingleObject(hThread[i], 200); // 1/5 second pause between each start
         }
         
     }
     
     for(i = 0; i < 656; i++) WaitForSingleObject(hThread[i], INFINITE); CloseHandle(hThread[i]);
     TreeView_SortChildren(hTree, Open, false);
     TreeView_SortChildren(hTree, Closed, false);
     done = true;
     if(totalScan == 65535) finished = 1;
}

DWORD scanner(int start)
{
   SOCKET sock;
   struct sockaddr_in port;
   
   char buffer[200];
   
   for(int x = start; x < (start + 100); x++)
   {
       itoa(x, buffer, 10);     
       sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);    
       if(x > 65535) break;
       
       port.sin_family = AF_INET;
       port.sin_addr.s_addr = inet_addr(ip);
       port.sin_port = htons(x);
         
       int ret = connect(sock, (sockaddr*)&port, sizeof(port));
       if (ret == SOCKET_ERROR)
       {    
          HTREEITEM error = addToRoot(hTree, Closed, buffer, 1);
          switch(WSAGetLastError()){
              case WSAENETDOWN:
                   addToRoot(hTree, error, "Network is down", 2);
                   break;
              case WSAENETUNREACH:
                   addToRoot(hTree, error, "Network is unreachable", 2);
                   break;
              case WSAENETRESET:
                   addToRoot(hTree, error, "Network dropped connection on reset", 2);
                   break;
              case WSAECONNABORTED:
                   addToRoot(hTree, error, "Software caused connection abort", 2);
                   break;
              case WSAECONNRESET:
                   addToRoot(hTree, error, "Connection reset by peer", 2);
                   break;
              case WSAETIMEDOUT:
                   addToRoot(hTree, error, "Connection timed out", 2);
                   break;
              case WSAECONNREFUSED:
                   addToRoot(hTree, error, "Connection refused", 2);
                   break;
              case WSAEHOSTDOWN:
                   addToRoot(hTree, error, "Host is down", 2);
                   break;
              case WSAHOST_NOT_FOUND:
                   addToRoot(hTree, error, "Host not found", 2);
                   break;
              default:
                   addToRoot(hTree, error, "Unknown reason", 2);
                   break;
          }
            
       }
       else
       {
          addToRoot(hTree, Open, buffer, 1);
       }
       
       totalScan++;  
       UpdateBars();
       closesocket(sock);
       Sleep(100); // Dont eat toooo much memory.
   }
}

void reset()
{
   TreeView_DeleteAllItems(hTree);
   PopulateTree();
   SendMessage(hProg, PBM_STEPIT, 0, 0);
   SendMessage(hStatus, SB_SETTEXT, 0, (LPARAM)"0 of 65535 Scanned");
   done = true;
}

void UpdateBars()
{
     char buf[100];
     SendMessage(hProg, PBM_STEPIT, 0, 0);
     sprintf(buf, "%i of 65535 Scanned", totalScan);
     SendMessage(hStatus, SB_SETTEXT, 0, (LPARAM)buf);
}

void PopulateStatus()
{
   int statwidths[] = {300, -1};
   
   SendMessage(hStatus, SB_SETPARTS, sizeof(statwidths)/sizeof(int), (LPARAM)statwidths);
   SendMessage(hStatus, SB_SETTEXT, 0, (LPARAM)"0 of 65535 Scanned");
   SendMessage(hStatus, SB_SETTEXT, 1, (LPARAM)"(C)Todd Withers");
}

void PopulateRebar()
{
     REBARBANDINFO rbBand = { sizeof(REBARBANDINFO) };
     rbBand.fMask  =  RBBIM_STYLE | RBBIM_TEXT | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_SIZE;
     rbBand.fStyle = RBBS_CHILDEDGE | RBBS_GRIPPERALWAYS | RBBS_NOGRIPPER;

     RECT rc;
     GetWindowRect(hIP, &rc);
     rbBand.lpText = "IP Address";
     rbBand.hwndChild = hIP;
     rbBand.cxMinChild = 0;
     rbBand.cyMinChild = rc.bottom - rc.top;
     rbBand.cx = 230;
     SendMessage(hRebar, RB_INSERTBAND, (WPARAM)0, (LPARAM)&rbBand);
             
     GetWindowRect(hScan, &rc);
     rbBand.lpText = "";
     rbBand.hwndChild = hScan;
     rbBand.cxMinChild = 0;
     rbBand.cyMinChild = rc.bottom - rc.top;
     rbBand.cx = 100;
     SendMessage(hRebar, RB_INSERTBAND, (WPARAM)1, (LPARAM)&rbBand);
     
     GetWindowRect(hProg, &rc);        
     rbBand.lpText = "";
     rbBand.hwndChild = hProg;
     rbBand.cxMinChild = 0;
     rbBand.cyMinChild = rc.bottom - rc.top;
     rbBand.cx = 100;
     SendMessage(hRebar, RB_INSERTBAND, (WPARAM)2, (LPARAM)&rbBand);
}

void PopulateTree()
{
     Open = addRootToTree(hTree, "Open Ports");
     Closed = addRootToTree(hTree, "Closed Ports");
}

HTREEITEM addToRoot(HWND hTV, HTREEITEM hItem, char* itemText, int itemLevel)
{
   TVITEM tvi;
   TVINSERTSTRUCT tvins;
   
   tvi.mask = TVIF_TEXT | TVIF_PARAM; 
   tvi.pszText = itemText; 
   tvi.cchTextMax = strlen(itemText);
   
    tvi.lParam = itemLevel; 
    tvins.item = tvi; 
    tvins.hParent = hItem;
    
    SendMessage(hTV, TVM_INSERTITEM, 0, (LPARAM)&tvins);
}

HTREEITEM addRootToTree(HWND hTV, char* itemText)
{
   TVITEM tvi;
   TVINSERTSTRUCT tvins;
   static HTREEITEM hPrev = (HTREEITEM)TVI_FIRST;
   
   tvi.mask = TVIF_TEXT | TVIF_PARAM; 
   tvi.pszText = itemText; 
   tvi.cchTextMax = strlen(itemText);
   
   tvi.lParam = 0; 
   tvins.item = tvi; 
   tvins.hInsertAfter = hPrev; 
   tvins.hParent = TVI_ROOT;
    
   hPrev = (HTREEITEM)SendMessage(hTV, TVM_INSERTITEM, 0, (LPARAM)&tvins); 
    
   return hPrev;
}


void CreateToolTip(HWND hwnd, char* cTip)
{
    HWND hTip = CreateWindowEx(0, TOOLTIPS_CLASS, NULL, WS_POPUP |TTS_ALWAYSTIP , CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, hwnd, NULL, hInstance, NULL);                          
                              
    TOOLINFO toolInfo;
    toolInfo.cbSize = sizeof(toolInfo);
    toolInfo.hwnd = hwnd;
    toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
    toolInfo.uId = (UINT_PTR)hwnd;
    toolInfo.lpszText = cTip;
    SendMessage(hTip, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);
}
