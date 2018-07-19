// stdafx.h : 자주 사용하지만 자주 변경되지는 않는
// 표준 시스템 포함 파일 또는 프로젝트 관련 포함 파일이
// 들어 있는 포함 파일입니다.
//

#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"Winmm.lib")
#pragma comment(lib,"Pdh.lib")

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>
#include <WinSock2.h>
#include <Windows.h>
#include <ws2tcpip.h>
#include <string.h>
#include <math.h>
#include <process.h>
#include <mstcpip.h>
#include <pdh.h>

#include <list>
#include <map>
#include <forward_list>
#include <stack>
#include <Pdh.h>



// TODO: 프로그램에 필요한 추가 헤더는 여기에서 참조합니다.

#include "rapidjson\document.h"
#include "rapidjson\writer.h"
#include "rapidjson\stringbuffer.h"



#include "APIHook.h"
#include "CCrashDump.h"
#include "processCPUCheck.h"
#include "profiling.h"

#include "log.h"
#include "memoryPool.h"
#include "memoryPoolTLS.h"
#include "Sbuf.h"
#include "lockFreeQueue.h"
#include "lockFreeStack.h"
#include "winQueue.h"

#include "GameProtocol.h"

#include "MMODoc.h"

#include "profiling.h"
#include "session.h"
#include "player.h"
#include "LanClient.h"
#include "monitorClient.h"
#include "MMOServer.h"
#include "GAMEServer.h"

