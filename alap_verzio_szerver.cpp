#include <Winsock2.h>
#include <ws2tcpip.h>
#include <WinInet.h>
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "ws2_32.lib")
#include <Windows.h>
#include <process.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <commctrl.h>

//*********************************
//ÁLTALÁNOS WIN32API VÁLTOZÓK
//*********************************
HANDLE THANDLER;
#define MAX_RESP_LENGTH 8*1024*1024
#define HIBA_00 TEXT("Error:Program initialisation process.")
HINSTANCE hInstGlob;
int SajatiCmdShow;
char szClassName[] = "WindowsApp";
HWND Form1; //Ablak kezelõje
HWND Button1;
#define OBJ_ID100 100

//*********************************
//SZERVERVÁLTOZÓK
//*********************************
FILE* flog;
int vege = 0;
WSADATA wsaData;
WORD wVersionRequested = MAKEWORD(2, 2);
BOOL serverisopen;
SOCKET clientsocket, serversocket;
sockaddr_in serveraddr, clientaddr;
char localinfo[128], htmlresp[MAX_RESP_LENGTH];
int i, nLength = sizeof(SOCKADDR);
char requrl[4096], reqtype[1024], reqvalue[1024], keyword[1024];
char val_str1[64], val_str2[64];
int value1 = 0, value2 = 0;

LRESULT CALLBACK WndProc0(HWND, UINT, WPARAM, LPARAM);
void ShowMessage(LPCTSTR uzenet, LPCTSTR cim, HWND kuldo);


//*********************************
//SZERVERFÜGGVÉNYEK
//*********************************
void varakozas(void* pParams);
void init(void);
void set_systeminfo(char* resp_string);
void log(const char* puffer);
void open_server(void);
void start_server_listening(void);
void close_server(void);

//*********************************
//KÉRÉSEK ELEMZÉSE
//*********************************
void get_request(char* urlrequest, char* type, char* retstr, char sepchar);
int get_keyword_location(char* srcstr, char* keyword);
void get_keyword_value(char* srcstr, char* keyword, int startpoz, char* retstr, char sepchar);
unsigned int get_string_length(char* srcstr);

//*********************************
//A windows program belépési pontja
//*********************************
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
 static TCHAR szAppName[] = TEXT("StdWinClassName");
 HWND hwnd;
 MSG msg;
 WNDCLASS wndclass0;
 SajatiCmdShow = iCmdShow;
 hInstGlob = hInstance;
 LoadLibraryA("COMCTL32.DLL");

 //*********************************
 //Ablak osztálypéldány elõkészítése
 //*********************************
 wndclass0.style = CS_HREDRAW | CS_VREDRAW;
 wndclass0.lpfnWndProc = WndProc0;
 wndclass0.cbClsExtra = 0;
 wndclass0.cbWndExtra = 0;
 wndclass0.hInstance = hInstance;
 wndclass0.hIcon = LoadIcon(NULL, IDI_APPLICATION);
 wndclass0.hCursor = LoadCursor(NULL, IDC_ARROW);
 wndclass0.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);//LTGRAY_BRUSH
 wndclass0.lpszMenuName = NULL;
 wndclass0.lpszClassName = TEXT("WIN0");

 //*********************************
 //Ablak osztálypéldány regisztrációja
 //*********************************
 if (!RegisterClass(&wndclass0))
 {
  MessageBox(NULL, HIBA_00, TEXT("Program Start"), MB_ICONERROR); return 0;
 }

 //*********************************
 //Ablak létrehozása
 //*********************************
 Form1 = CreateWindow(TEXT("WIN0"),
  TEXT("Mini szerver"),
  (WS_OVERLAPPED | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX),
  50,
  50,
  400,
  300,
  NULL,
  NULL,
  hInstance,
  NULL);

 //*********************************
 //Ablak megjelenítése
 //*********************************
 ShowWindow(Form1, SajatiCmdShow);
 UpdateWindow(Form1);

 //*********************************
 //Ablak üzenetkezelésének aktiválása
 //*********************************
 while (GetMessage(&msg, NULL, 0, 0))
 {
  TranslateMessage(&msg);
  DispatchMessage(&msg);
 }
 return msg.wParam;
}

//*********************************
//Az ablak callback függvénye: eseménykezelés
//*********************************
LRESULT CALLBACK WndProc0(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
 HDC hdc;
 PAINTSTRUCT ps;

 switch (message)
 {
  //*********************************
  //Ablak létrehozásakor közvetlenül
  //*********************************
 case WM_CREATE:
  /*Init*/;
  init();
  Button1 = CreateWindow(TEXT("button"), TEXT("Szerver bekapcsolása")
   , WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_MULTILINE, 50, 50, 170, 30
   , hwnd, (HMENU)(OBJ_ID100), ((LPCREATESTRUCT)lParam)->hInstance, NULL);  
  //open_server();
  return 0;
  //*********************************
  //vezérlõelemek mûködtetése
  //*********************************
 case WM_COMMAND:
  switch (LOWORD(wParam))
  {
  case OBJ_ID100:
   //**********************************************
   //Fõ vezérlési szerkezet indítása, külön szálon
   //**********************************************
   THANDLER = (HANDLE)_beginthread(varakozas, 0, NULL);
   break;
  }

  return 0;
  //*********************************
//Ablak átméretezése
//*********************************
 case WM_SIZE:
  return 0;

  //*********************************
  //Ablak kliens területének újrarajzolása
  //*********************************
 case WM_PAINT:
  hdc = BeginPaint(hwnd, &ps);
  EndPaint(hwnd, &ps);
  return 0;
  //*********************************
  //Ablak bezárása
  //*********************************
 case WM_CLOSE:
  vege = 1;
  close_server();
  DestroyWindow(hwnd);
  return 0;
  //*********************************
  //Ablak megsemmisítése
  //*********************************
 case WM_DESTROY:
  PostQuitMessage(0);
  return 0;
 }
 return DefWindowProc(hwnd, message, wParam, lParam);
}

//*********************************
//Üzenetablak OK gombbal
//*********************************
void ShowMessage(LPCTSTR uzenet, LPCTSTR cim, HWND kuldo)
{
 MessageBox(kuldo, uzenet, cim, MB_OK);
}

//***********************
//Naplózás megvalósítása
//***********************
void log(const char* puffer)
{
 FILE* fp = NULL;
 SYSTEMTIME datumido;
 GetLocalTime(&datumido);

 if (vege == 1) return;
 char logbuffer[2048];
 int meret = sizeof(&puffer);

 fopen_s(&fp, "LOG.txt", "at");
 if (fp == NULL) return;

 fprintf_s(fp, "%i.%i.%i %i:%i:%i.%i %s\r\n", datumido.wYear, datumido.wMonth, datumido.wDay, datumido.wHour, datumido.wMinute, datumido.wSecond, datumido.wMilliseconds, puffer);

 fclose(fp);
}

//******************************
//Szerver inicializálása
//******************************
void open_server(void)
{
 serverisopen = false;
 clientsocket = INVALID_SOCKET;
 serversocket = INVALID_SOCKET;

 if (WSAStartup(wVersionRequested, &wsaData) != 0)
 {
  WSACleanup();
  log("Winsock init error...");
  return;
 }

 log("Winsock init OK.");
 log(wsaData.szDescription);

 gethostname(localinfo, sizeof(localinfo));
 log(localinfo);
 log("PORT: 80");

 serversocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
 if (serversocket == INVALID_SOCKET)
 {
  WSACleanup();
  log("Couldn't create server socket...");
 }
 log("Server socket created...");

 serveraddr.sin_family = AF_INET;
 inet_pton(AF_INET, "127.0.0.1", &serveraddr.sin_addr.s_addr);
 serveraddr.sin_port = htons(80);
 if (bind(serversocket, (SOCKADDR*)&serveraddr, sizeof(serveraddr)) == SOCKET_ERROR)
 {
  WSACleanup();
  serversocket = INVALID_SOCKET;
  log("Couldn't bind server socket...");
  return;
 }
 else log("Binding server socket OK...");
}

//****************************************
//Bejövõ kérések fogadása és feldolgozása
//****************************************
void start_server_listening(void)
{
 int i;
 int iResult;
 int iSendResult;
 int recvbuflen = 4096;
 char recvbuf[4096];
 
 for (i = 0; i < recvbuflen; ++i) recvbuf[i] = '\0';
 if (serversocket == INVALID_SOCKET) return;

 SetWindowTextA(Form1, "Bejövõ kérések figyelése...");

 if (listen(serversocket, 1) == SOCKET_ERROR) //max 1 kapcsolat
 {
  WSACleanup();
  serversocket = INVALID_SOCKET;
  log("Server socket FAILED to listen...");
  return;
 }
 else
 {
  log("Server socket is listening...");
  serverisopen = true;
 }

 clientsocket = accept(serversocket, (SOCKADDR*)&clientaddr, (LPINT)&nLength);

 if (clientsocket == INVALID_SOCKET)
  log("Server socket failed to accept connection...");
 else
 {
  log("CONNECTION OK...");
  SetWindowTextA(Form1, "Kliens kapcsolódott!");
  log("Collecting request...");

  do {
   log("Reading request...");
   iResult = recv(clientsocket, recvbuf, recvbuflen - 2, 0);

   if (iResult > 0) {    
    int meret = sizeof(recvbuf);
    log("******************");
    log("Client request:");
    log("******************");
    log(recvbuf);
    log("******************");
    shutdown(clientsocket, SD_RECEIVE);
    strcpy_s(requrl, _countof(requrl), (const char*)recvbuf);
    strcpy_s(keyword, _countof(keyword), "GET /");
    get_request(requrl, keyword, reqvalue, '.');
    log("******************");
    log("Server response:");
    log("******************");

    //*************************
    // GETSYSINFO
    //*************************
    if (strcmp(reqvalue, "getsysinfo") == 0)
    {
     set_systeminfo(htmlresp);     
    }

    //*************************
    // GETSUM
    //*************************
    else if (strcmp(reqvalue, "getsum") == 0)
    {
     strcpy_s(keyword, _countof(keyword), "V1=");
     get_request(requrl, keyword, val_str1, '?');
     value1 = atoi(val_str1);
     strcpy_s(keyword, _countof(keyword), "V2=");
     get_request(requrl, keyword, val_str2, '.');
     value2 = atoi(val_str2);
     value1 += value2;
     _itoa_s(value1, keyword, sizeof(keyword), 10);
     strcpy_s(htmlresp, "<!doctype HTML>\n<html><h3>SUM: ");
     strcat_s(htmlresp, keyword);
     strcat_s(htmlresp, "</h3></html>\0");
    }

    //*************************
    // CHECKSERVER
    //*************************
    else if (strcmp(reqvalue, "checkserver") == 0)
    {
     strcpy_s(htmlresp, "|OK|");
    }

    //*************************
    // FAVICON SZÛRÉS
    //*************************
    else if (strcmp(reqvalue, "favicon") == 0)
    {
     strcpy_s(htmlresp, "Browser connection accepted...");
    }

    //*************************
    // MINDEN MÁS ESET
    //*************************
    else
    {
     strcpy_s(htmlresp, "<!doctype HTML>\n<html><h3>Unknown command!</h3></html>\0");
    }
    log(htmlresp);
    log("******************");

    iResult = send(clientsocket, htmlresp, (int)strlen(htmlresp) + 1, 0);
    if (iResult == SOCKET_ERROR)
     log("Sending response FAILED...");
    else
    {
     log("Response sent OK...");
     shutdown(clientsocket, SD_SEND);
    }
    closesocket(clientsocket);
    closesocket(serversocket);
    WSACleanup();
    break;
   }

   else if (iResult == 0)
    log("Connection closing...");

   else {
    log("Reading request failed with error");
    closesocket(clientsocket);
    WSACleanup();
    return;
   }

  } while (iResult > 0);
 }
}

//**************************
//A SZERVER LEÁLL
//**************************
void close_server(void)
{
 log("Close server...");
 log("WSA cleanup...");
 shutdown(clientsocket, SD_SEND);
 if (serverisopen == true) closesocket(serversocket);
 if (clientsocket != INVALID_SOCKET) closesocket(clientsocket);
 WSACleanup();
 serverisopen = false;
 clientsocket = INVALID_SOCKET;
 serversocket = INVALID_SOCKET;
 log("Sockets closed...");
}

//***********************
//Fõ vezérlési szerkezet
//***********************
void varakozas(void* pParams)
{
 ShowWindow(Button1, SW_HIDE);
 while (1)
 {
  if (vege == 1) break;
  open_server();
  if (vege == 1) break;
  start_server_listening();
  if (vege == 1) break;
  close_server();
 }
}

//***********************
//KÉRÉSEK PARSE-OLÁSA
//***********************
void get_request(char* urlrequest, char* type, char* retstr, char sepchar)
{
 int i;

 i = get_keyword_location(urlrequest, type);
 get_keyword_value(urlrequest, type, i, retstr, sepchar);
 i = i;
}

//********************************
//KULCSSZÓ HELYÉNEK MEGÁLLAPÍTÁSA
//********************************
int get_keyword_location(char* srcstr, char* keyword)
{
 int i = 0, j = 0, retval = -1, MAX_i = get_string_length(srcstr) - get_string_length(keyword);

 while (i < MAX_i)
 {
  for (j = 0; j < get_string_length(keyword) - 1; ++j)
  {
   if (srcstr[i + j] == keyword[j]) retval = 0;
   else {
    retval = -1; break;
   }
  }
  if (retval != -1) {//megvan a keyword helye
   retval = i;
   break;
  }
  ++i;
 }
 return retval + get_string_length(keyword);
}

//*********************
//KULCSÉRTÉK KINYERÉSE
//*********************
void get_keyword_value(char* srcstr, char* keyword, int startpoz, char* retstr, char sepchar)
{
 int i = startpoz, j = 0;

 for (; srcstr[i] != sepchar; ++i)
 {
  retstr[j++] = srcstr[i];
 }
 retstr[j] = 0;
}

//********************************
//SZTRING HOSSZÁNAK MEGHATÁROZÁSA
//********************************
unsigned int get_string_length(char* srcstr)
{
 unsigned int i = 0;
 while (srcstr[i] != '\0') ++i;
 return i;
}

//************************
//PROGRAM INICIALIZÁCIÓJA
//************************
void init(void)
{
 fopen_s(&flog, "LOG.txt", "wt");
 fclose(flog);
}

//*************************************
//GETSYSINFO ÜZENET DINAMIKUS TARTALMA
//*************************************
void set_systeminfo(char* resp_string)
{
 int i;
 char tempstr[256];
 char cname[64];
 LONGLONG TotalMemoryInKilobytes = 0;

 DWORD len = MAX_COMPUTERNAME_LENGTH + 1;
 SYSTEMTIME datumido;

 GetLocalTime(&datumido);

 GetComputerNameA(cname, &len);

 strcpy_s(resp_string, MAX_RESP_LENGTH, "System time: ");
 _itoa_s(datumido.wYear, tempstr, sizeof(tempstr), 10);
 strcat_s(resp_string, MAX_RESP_LENGTH, tempstr);
 strcat_s(resp_string, MAX_RESP_LENGTH, ".");
 _itoa_s(datumido.wMonth, tempstr, sizeof(tempstr), 10);
 strcat_s(resp_string, MAX_RESP_LENGTH, tempstr);
 strcat_s(resp_string, MAX_RESP_LENGTH, ".");
 _itoa_s(datumido.wDay, tempstr, sizeof(tempstr), 10);
 strcat_s(resp_string, MAX_RESP_LENGTH, tempstr);
 strcat_s(resp_string, MAX_RESP_LENGTH, " ");
 _itoa_s(datumido.wHour, tempstr, sizeof(tempstr), 10);
 strcat_s(resp_string, MAX_RESP_LENGTH, tempstr);
 strcat_s(resp_string, MAX_RESP_LENGTH, ":");
 _itoa_s(datumido.wMinute, tempstr, sizeof(tempstr), 10);
 strcat_s(resp_string, MAX_RESP_LENGTH, tempstr);
 strcat_s(resp_string, MAX_RESP_LENGTH, ":");
 _itoa_s(datumido.wSecond, tempstr, sizeof(tempstr), 10);
 strcat_s(resp_string, MAX_RESP_LENGTH, tempstr);

 strcat_s(resp_string, MAX_RESP_LENGTH, "|Computer name: ");
 strcat_s(resp_string, MAX_RESP_LENGTH, (const char *)cname);
 strcat_s(resp_string, MAX_RESP_LENGTH, "|Installed RAM: ");

 GetPhysicallyInstalledSystemMemory((PULONGLONG)&TotalMemoryInKilobytes);
 i = TotalMemoryInKilobytes;
 _itoa_s(i / 1024 / 1024, tempstr, sizeof(tempstr), 10);
 strcat_s(resp_string, MAX_RESP_LENGTH, tempstr);
 strcat_s(resp_string, MAX_RESP_LENGTH, "|");
}