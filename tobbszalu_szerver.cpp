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
#define HIBA_00 TEXT("Error:Program initialisation process.")
#define WINSTYLE_DIALOG (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU)
#define IDC_STATIC -1
#define OBJ_ID100 100
#define OBJ_ID101 101
#define OBJ_ID102 102
#define OBJ_ID103 103
#define OBJ_ID104 104
HWND Form1;
HWND Label1;
HWND Edit1;
HWND Edit2;
HWND Label2;
HWND Label3;
HWND Edit3;
HWND Label4;
HWND Edit4;
HWND Button1;
HWND Label5;

HINSTANCE hInstGlob;
int SajatiCmdShow;
char szClassName[] = "WindowsApp";

//*********************************
//SZERVERVÁLTOZÓK
//*********************************
int vege = 0, server_started = 0;
HANDLE THANDLER;
#define RECBUFLEN 4096
int request_count = 0;
#define MAX_RESP_LENGTH 10*1024
#define MIN_RESP_LENGTH 1024
WSADATA wsaData;
WORD wVersionRequested = MAKEWORD(2, 2);
BOOL serverisopen;
SOCKET clientsocket, serversocket;
sockaddr_in serveraddr, clientaddr;
char localinfo[128];
int i, nLength = sizeof(SOCKADDR);
FILE* flog;

void varakozas(void* pParams);
void thread_muvelet(void* pParams);

LRESULT CALLBACK WndProc0(HWND, UINT, WPARAM, LPARAM);
void ShowMessage(LPCTSTR uzenet, LPCTSTR cim, HWND kuldo);

//*********************************
//SZERVERFÜGGVÉNYEK
//*********************************
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
 //Ablak osztálypéldány előkészítése
 //*********************************
 wndclass0.style = CS_HREDRAW | CS_VREDRAW;
 wndclass0.lpfnWndProc = WndProc0;
 wndclass0.cbClsExtra = 0;
 wndclass0.cbWndExtra = 0;
 wndclass0.hInstance = hInstance;
 wndclass0.hIcon = LoadIcon(NULL, IDI_APPLICATION);
 wndclass0.hCursor = LoadCursor(NULL, IDC_ARROW);
 wndclass0.hbrBackground = (HBRUSH)GetStockObject(LTGRAY_BRUSH);//LTGRAY_BRUSH
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
  TEXT("INTERAKTÍV SZERVER - Stopped..."),
  WINSTYLE_DIALOG,
  500,
  500,
  300,
  420,
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

  Label1 = CreateWindow(TEXT("static"), TEXT("IP address:")
   , WS_CHILD | WS_VISIBLE, 10, 15, 120, 30
   , hwnd, (HMENU)(IDC_STATIC), ((LPCREATESTRUCT)lParam)->hInstance, NULL);
  Edit1 = CreateWindow(TEXT("edit"), TEXT("127.0.0.1")
   , WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, 10, 50, 120, 30
   , hwnd, (HMENU)(OBJ_ID100), ((LPCREATESTRUCT)lParam)->hInstance, NULL);
  SendMessage(Edit1, EM_SETLIMITTEXT, 15, 0);
  Edit2 = CreateWindow(TEXT("edit"), TEXT("getsysinfo")
   , WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, 10, 135, 120, 30
   , hwnd, (HMENU)(OBJ_ID101), ((LPCREATESTRUCT)lParam)->hInstance, NULL);
  SendMessage(Edit2, EM_SETLIMITTEXT, 20, 0);
  Label2 = CreateWindow(TEXT("static"), TEXT("Request #1:")
   , WS_CHILD | WS_VISIBLE, 10, 100, 120, 30
   , hwnd, (HMENU)(IDC_STATIC), ((LPCREATESTRUCT)lParam)->hInstance, NULL);
  Label3 = CreateWindow(TEXT("static"), TEXT("Request #2:")
   , WS_CHILD | WS_VISIBLE, 10, 180, 120, 30
   , hwnd, (HMENU)(IDC_STATIC), ((LPCREATESTRUCT)lParam)->hInstance, NULL);
  Edit3 = CreateWindow(TEXT("edit"), TEXT("getsum")
   , WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, 10, 215, 120, 30
   , hwnd, (HMENU)(OBJ_ID102), ((LPCREATESTRUCT)lParam)->hInstance, NULL); //getsum.V1=5?V2=9.txt
  SendMessage(Edit3, EM_SETLIMITTEXT, 20, 0);
  Label4 = CreateWindow(TEXT("static"), TEXT("Label4")
   , WS_CHILD | WS_VISIBLE, 10, 260, 120, 30
   , hwnd, (HMENU)(IDC_STATIC), ((LPCREATESTRUCT)lParam)->hInstance, NULL);
  Edit4 = CreateWindow(TEXT("edit"), TEXT("checkserver")
   , WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, 10, 295, 120, 30
   , hwnd, (HMENU)(OBJ_ID103), ((LPCREATESTRUCT)lParam)->hInstance, NULL);
  SendMessage(Edit4, EM_SETLIMITTEXT, 20, 0);
  Button1 = CreateWindow(TEXT("button"), TEXT("START SERVER")
   , WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_MULTILINE, 150, 15, 120, 310
   , hwnd, (HMENU)(OBJ_ID104), ((LPCREATESTRUCT)lParam)->hInstance, NULL);
  Label5 = CreateWindow(TEXT("static"), TEXT("Incoming requests: 0")
   , WS_CHILD | WS_VISIBLE, 10, 340, 260, 30
   , hwnd, (HMENU)(IDC_STATIC), ((LPCREATESTRUCT)lParam)->hInstance, NULL);

  //open_server();
  return 0;
  //*********************************
  //vezérlőelemek működtetése
  //*********************************
 case WM_COMMAND:
  switch (LOWORD(wParam))
  {
  case OBJ_ID104:
   //******************
   //SZERVER INDÍTÁSA
   //******************
   if (server_started == 0)
   {
    vege = 0;
    open_server();
    SetWindowTextA(Button1, "STOP SERVER");
    server_started = 1;
    THANDLER = (HANDLE)_beginthread(varakozas, 0, NULL);
   }
   //******************
   //SZERVER LEÁLLÍTÁSA
   //******************
   else if (server_started == 1)
   {
    SetWindowTextA(Form1, "INTERAKTÍV SZERVER - Stopped...");
    SetWindowTextA(Button1, "START SERVER");
    server_started = 0;
    vege = 1;
    close_server();
   }

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
 //strcpy_s(logbuffer, puffer);
 fopen_s(&fp, "LOG.txt", "at");
 if (fp == NULL) return;
 //fwrite(puffer,sizeof(puffer),1,flog);
 fprintf_s(fp, "%i.%i.%i %i:%i:%i.%i %s\r\n", datumido.wYear, datumido.wMonth, datumido.wDay, datumido.wHour, datumido.wMinute, datumido.wSecond, datumido.wMilliseconds, puffer);
 fclose(fp);
}

//******************************
//Szerver inicializálása
//******************************
void open_server(void)
{
 char myip_address[20];
 serverisopen = false;
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
 GetWindowTextA(Edit1, myip_address, 20);
 inet_pton(AF_INET, myip_address, &serveraddr.sin_addr.s_addr);//127.0.0.1
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
//Bejövő kérések fogadása és feldolgozása
//****************************************
void start_server_listening(void)
{
 int i;
 int iResult;
 int iSendResult;
 int recvbuflen = 4096;
 char recvbuf[4096];

 clientsocket = INVALID_SOCKET;

 for (i = 0; i < recvbuflen; ++i) recvbuf[i] = '\0';
 if (serversocket == INVALID_SOCKET) return;

 SetWindowTextA(Form1, "INTERAKTÍV SZERVER - Online...");
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
 //clientsocket = accept(serversocket, NULL, NULL);
 if (clientsocket == INVALID_SOCKET)
  log("Server socket failed to accept connection...");
 else
 {
  log("CONNECTION OK...");
  ++request_count;

  _beginthread(thread_muvelet, 0, (void*)clientsocket);
  clientsocket == INVALID_SOCKET;
 }
}

//****************************************
//ÜZENETFELDOLGOZÁS KÜLÖN SZÁLON
//****************************************
void thread_muvelet(void* pParams)
{
 int iResult;
 int iSendResult;
 char recvbuf[RECBUFLEN];
 int i = 0;
 int value1 = 0, value2 = 0;
 SOCKET kliens_socket = (SOCKET)pParams;
 char htmlresp[10*1024], val_str1[64], val_str2[64];
 char requrl[1024], reqvalue[1024], keyword[1024];
 char reqname1[20], reqname2[20], reqname3[20];
 char tmpbuf[1024], tmpbuf2[1024];

 GetWindowTextA(Edit2,reqname1,20);
 GetWindowTextA(Edit3, reqname2, 20);
 GetWindowTextA(Edit4, reqname3, 20);
 for (i = 0; i < RECBUFLEN; ++i) recvbuf[i] = '\0';

 log("\n*****Collecting request******");
 do {

  iResult = recv(kliens_socket, recvbuf, RECBUFLEN - 2, 0);
  if (iResult > 0) {
   log("Reading request...");
   int meret = sizeof(recvbuf);
   log("******************");
   log("Client request:");
   log("******************");
   log(recvbuf);
   log("******************");
   shutdown(kliens_socket, SD_RECEIVE);
   strcpy_s(requrl, _countof(requrl), (const char*)recvbuf);
   strcpy_s(keyword, _countof(keyword), "GET /");
   get_request(requrl, keyword, reqvalue, '.');
   log("******************");
   log("Server response:");
   log("******************");

   //*************************
   // GETSYSINFO
   //*************************
   if (strcmp(reqvalue, reqname1) == 0) //getsysinfo
   {
    set_systeminfo(htmlresp);
   }

   //*************************
   // GETSUM
   //*************************
   else if (strcmp(reqvalue, reqname2) == 0) //getsum
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
   else if (strcmp(reqvalue, reqname3) == 0) //checkserver
   {
    strcpy_s(htmlresp, "|OK|");
   }

   //*************************
   // FAVICON SZŰRÉS
   //*************************
   else if (strcmp(reqvalue, "favicon") == 0)
   {
    --request_count;
    strcpy_s(htmlresp, "Browser connection accepted...");
   }

   //*************************
   // MINDEN MÁS ESET
   //*************************
   else
   {
    strcpy_s(htmlresp, "<!doctype HTML>\n<html><h3>Unknown command!</h3></html>\0");
   }

   //*************************
   // KÉRÉSEK SZÁMLÁLÁSA
   //*************************
   _itoa_s(request_count, tmpbuf2, sizeof(tmpbuf2), 10);
   strcpy_s(tmpbuf, _countof(tmpbuf), "Incoming requests : ");
   strcat_s(tmpbuf, _countof(tmpbuf), tmpbuf2);
   SetWindowTextA(Label5, tmpbuf);

   //*************************
   // VÁLASZ ELKÜLDÉSE
   //*************************
   iResult = send(kliens_socket, htmlresp, (int)strlen(htmlresp) + 1, 0);
   log(htmlresp);
   log("******************");

   if (iResult == SOCKET_ERROR)
    log("Sending response FAILED...");
   else
   {
    log("Response sent OK...");
    shutdown(kliens_socket, SD_SEND);
   }
   closesocket(kliens_socket);
   //closesocket(serversocket);
   //WSACleanup();
   break;
  }
  else if (iResult == 0)
   log("Connection closing...");
  else {
   log("Reading request failed with error");
   closesocket(kliens_socket);
   WSACleanup();
   return;
  }

 } while (iResult > 0);

 log("*****End of request******");
}

//**************************
//A SZERVER LEÁLL
//**************************
void close_server(void)
{
 log("\nEnding...");

 log("WSA cleanup...");
 shutdown(clientsocket, SD_SEND);
 if (serverisopen == true) closesocket(serversocket);
 if (clientsocket != INVALID_SOCKET) closesocket(clientsocket);
 shutdown(serversocket, SD_SEND);
 shutdown(serversocket, SD_RECEIVE);
 WSACleanup();
 serverisopen = false;
 /*clientsocket = INVALID_SOCKET;
 serversocket = INVALID_SOCKET;*/
 log("Sockets closed...");
}

//***********************
//Fő vezérlési szerkezet
//***********************
void varakozas(void* pParams)
{
 while (1)
 {
  if (vege == 1) break;
  start_server_listening();
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
 char tempstr[2048];
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
 strcat_s(resp_string, MAX_RESP_LENGTH, (const char*)cname);
 strcat_s(resp_string, MAX_RESP_LENGTH, "|Installed RAM: ");

 GetPhysicallyInstalledSystemMemory((PULONGLONG)&TotalMemoryInKilobytes);
 i = TotalMemoryInKilobytes;
 _itoa_s(i / 1024 / 1024, tempstr, sizeof(tempstr), 10);
 strcat_s(resp_string, MAX_RESP_LENGTH, tempstr);
 strcat_s(resp_string, MAX_RESP_LENGTH, "|");
}