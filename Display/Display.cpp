#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <sstream>
#include <string.h>
#include <list>
#include <iomanip>
#include <time.h>
#include <process.h>	//_beginthreadex() e _endthreadex()
#include <conio.h>		// _getch


#define _CHECKERROR	1		// Ativa função CheckForError
#include "../Include/checkforerror.h"

typedef unsigned (WINAPI *CAST_FUNCTION)(LPVOID);	//Casting para terceiro e sexto parâmetros da função
typedef unsigned *CAST_LPDWORD;

using namespace std;

list <string> list1, 
			  list2;

HANDLE l1Mutex,						// Handle para o mutex 1
	   l2Mutex,						// Handle para o mutex 2
	   hCLPThread,					// Handle para thread que representa leitura de CLP
	   hPCPThread,					// Handle para thread que representa leitura de PCP
	   hCaptureThread,				// Handle para thread que retira mensagens da primeira lista
	   hProdThread,					// Handle para thread que retira mensagens da segunda lista e imprime na tela
	   hEscThread,					// Handle para thread que aborta a execução
	   hTimer1,						// Handle para timer 1
	   hTimer2,						// Handle para timer 2
	   hMessageList1,				// Handle para evento que sinaliza que uma mensagem foi depositada na lista 1
	   hMessageList2,				// Handle para evento que sinaliza que uma mensagem foi depositada na lista 2
	   hCLPSemaphore,				// Handle para semáforo da tarefa de leitura do CLP
	   hPCPSemaphore,				// Handle para semáforo da tarefa de leitura do PCP
	   hDisplaySemaphore,			// Handle para semáforo da tarefa de exibição de eventos
	   hMessageSemaphore,			// Handle para semáforo da tarefa de retirada de mensagens
	   hEscEvent,					// Handle para evento que aborta a execução
	   hPipeEvent,					// Handle para evento de sincronização do pipe
	   hPipe;						// Handle para o pipe

int offset = 0,
	modo = 0;

DWORD WINAPI EscFunc();	            // Declaração da função que encerra a aplicação
DWORD WINAPI ThreadCLP();		    // Thread representando leitura de CLP
DWORD WINAPI ThreadPCP();		    // Thread representando leitura de PCP
DWORD WINAPI ThreadCapture();	    // Thread que retira mensagens da primeira lista
DWORD WINAPI ThreadProd();		    // Thread que retira mensagens da segunda lista e imprime na tela

int main() {
	system("chcp 1252");			// Comando para apresentar caracteres especiais no console

	DWORD dwIdCLP, 
		  dwIdPCP, 
		  dwIdCapture, 
		  dwIdProd,
		  dwReturn,
		  dwExitCode,
		  dwThreadId,
          dwSentBytes;
  
	BOOL bStatus;
	LPTSTR lpszPipename = "\\\\.\\pipe\\ManagementPipe";

	// Criação dos mutexes
	l1Mutex = CreateMutex(NULL, FALSE, "MutexList1");
	l2Mutex = CreateMutex(NULL, FALSE, "MutexList2");

	// Criação dos eventos
	hTimer1 = CreateEvent(NULL, FALSE, FALSE, "CLPTimer");
	hTimer2 = CreateEvent(NULL, FALSE, FALSE, "PCPTimer");
	hMessageList1 = CreateEvent(NULL, FALSE, FALSE, "MessageList1");
	hMessageList2 = CreateEvent(NULL, FALSE, FALSE, "MessageList2");

	// Pega os handles para os semáforos que bloquea/desbloquea as tarefas
	hCLPSemaphore = OpenSemaphore(SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, NULL, "CLP");
	hPCPSemaphore = OpenSemaphore(SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, NULL, "PCP");
	hDisplaySemaphore = OpenSemaphore(SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, NULL, "Display");
	hMessageSemaphore = OpenSemaphore(SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, NULL, "Message");

	// Pega o handle para o evento de sincronização do pipe
	hPipeEvent = OpenEvent(SYNCHRONIZE, FALSE, "PipeEventDisplay");

	// Aguarda que o pipe seja criado pelo processo Management
	printf("Aguardando criação do pipe\n");
	WaitForSingleObject(hPipeEvent, INFINITE);

	// Abre instância para utilizar o pipe
	hPipe = CreateFile(
		lpszPipename,   // nome do pipe 
		GENERIC_WRITE,  // acesso para escrita 
		0,              // sem compartilhamento 
		NULL,           // lpSecurityAttributes
		OPEN_EXISTING,  // dwCreationDistribution 
		0,              // dwFlagsAndAttributes 
		NULL);          // hTemplate

	// Aguarda que o pipe esteja disponível
	WaitNamedPipe(lpszPipename, NMPWAIT_USE_DEFAULT_WAIT);

	// Criação da tarefa de leitura do CLP
	hCLPThread = (HANDLE)_beginthreadex(
		NULL, 
		0, 
		(CAST_FUNCTION)ThreadCLP, 
		NULL, 
		0, 
		(CAST_LPDWORD)&dwIdCLP
	);
	CheckForError(hCLPThread);

	// Criação da tarefa de leitura do PCP
	hPCPThread = (HANDLE)_beginthreadex(
		NULL,
		0,
		(CAST_FUNCTION)ThreadPCP,
		NULL,
		0,
		(CAST_LPDWORD)&dwIdPCP
	);
	CheckForError(hPCPThread);

	// Criação da tarefa que retira mensagens da primeira lista
	hCaptureThread = (HANDLE)_beginthreadex(
		NULL,
		0,
		(CAST_FUNCTION)ThreadCapture,
		NULL,
		0,
		(CAST_LPDWORD)&dwIdCapture
	);
	CheckForError(hCaptureThread);

	// Criação da tarefa que retira mensagens da segunda lista e imprime na tela
	hProdThread = (HANDLE)_beginthreadex(
		NULL,
		0,
		(CAST_FUNCTION)ThreadProd,
		NULL,
		0,
		(CAST_LPDWORD)&dwIdProd
	);
	CheckForError(hProdThread);

	// Criação da tarefa que encerra o processo
	hEscThread = (HANDLE)_beginthreadex(
		NULL,
		0,
		(CAST_FUNCTION)EscFunc,
		NULL,
		0,
		(CAST_LPDWORD)&dwThreadId
	);
	CheckForError(hEscThread);

	// Aguarda que o objeto que encerra a aplicação seja sinalizado
	dwReturn = WaitForSingleObject(hEscThread, INFINITE);
	CheckForError(dwReturn);
	
	// Fecha handles das threads
	dwReturn = GetExitCodeThread(hCLPThread, &dwExitCode);
	CheckForError(dwReturn);
	CloseHandle(hCLPThread);

	dwReturn = GetExitCodeThread(hPCPThread, &dwExitCode);
	CheckForError(dwReturn);
	CloseHandle(hPCPThread);

	dwReturn = GetExitCodeThread(hCaptureThread, &dwExitCode);
	CheckForError(dwReturn);
	CloseHandle(hCaptureThread);

	dwReturn = GetExitCodeThread(hProdThread, &dwExitCode);
	CheckForError(dwReturn);
	CloseHandle(hProdThread);

	dwReturn = GetExitCodeThread(hEscThread, &dwExitCode);
	CheckForError(dwReturn);
	CloseHandle(hEscThread);

	// Fecha handle dos mutexes
	CloseHandle(l1Mutex);
	CloseHandle(l2Mutex);

	// Fecha handles dos eventos
	CloseHandle(hPipeEvent);
	CloseHandle(hTimer1);
	CloseHandle(hTimer2);
	CloseHandle(hMessageList1);
	CloseHandle(hMessageList2);

	// Fecha handle do pipe
	CloseHandle(hPipe);

	// Fecha handles dos semáforos
	CloseHandle(hCLPSemaphore);
	CloseHandle(hPCPSemaphore);
	CloseHandle(hDisplaySemaphore);
	CloseHandle(hMessageSemaphore);

	return EXIT_SUCCESS;

}

DWORD WINAPI EscFunc() {
	// Pega o handle para o evento que aborta a execução
	hEscEvent = OpenEvent(SYNCHRONIZE, FALSE, "EscEvent");

	WaitForSingleObject(hEscEvent, INFINITE);

	// Fecha handle do evento
	CloseHandle(hEscEvent);

	ExitProcess(EXIT_SUCCESS);
}

DWORD WINAPI ThreadCLP() {
	int NSEQ = 1,
		tempo,
		hora,
		minuto,
		segundos;

	float TZona1,
		  TZona2,
		  TZona3,
		  volume,
		  pressao;

	char msg[54];
	std::ostringstream ss;
	time_t theTime;
	struct tm atime;
	srand(time(NULL));

	while (1) {
		// Aguarda o semáforo para sincronizar com Keyboard
		WaitForSingleObject(hCLPSemaphore, INFINITE);

		// Aguarda o timer 1
		WaitForSingleObject(hTimer1, 500);

		TZona1 = (rand() % 100000) / 10;
		TZona2 = (rand() % 100000) / 10;
		TZona3 = (rand() % 100000) / 10;
		volume = (rand() % 10000) / 10;
		pressao = (rand() % 10000) / 10;
		tempo = (rand() % 1000);

		theTime = time(NULL);
		localtime_s(&atime, &theTime);
		hora = atime.tm_hour;
		minuto = atime.tm_min;
		segundos = atime.tm_sec;

		sprintf_s(msg, "%06i/%06.1f/%06.1f/%06.1f/%05.1f/%05.1f/%04i/%02i:%02i:%02i", NSEQ, TZona1, TZona2, TZona3, volume, pressao, tempo, hora, minuto, segundos);
		
		// Aguarda o mutex 1
		WaitForSingleObject(l1Mutex, INFINITE);

		auto it = list1.begin();

		if (list1.size() == 200) {

			// Libera mutex 1
			ReleaseMutex(l1Mutex);
			while (list1.size() > 180)
				// Sinaliza que mensagem foi depositada na lista 1
				PulseEvent(hMessageList1);

			// Aguarda o mutex 1
			WaitForSingleObject(l1Mutex, INFINITE);
		}		
		
		list1.push_back(msg);

		// Sinaliza que mensagem foi depositada na lista 1
		PulseEvent(hMessageList1);

		// Libera mutex 1
		ReleaseMutex(l1Mutex);
		
		NSEQ++;

		// Libera semáforo
		ReleaseSemaphore(hCLPSemaphore, 1, NULL);
	}

	return EXIT_SUCCESS;
}



DWORD WINAPI ThreadPCP() {
	int NSEQ = 1,
		slot1,
		slot2,
		slot3,
		hora,
		minuto,
		segundos,
		i = 0;

	char op1_aux[8],
		 op2_aux[8],
		 op3_aux[8],
		 msg[56],
		 alphanum[] = "123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

	time_t theTime;
	struct tm atime;
	srand(time(NULL));

	while (1) {
		// Aguarda o semáforo para sincronizar com Keyboard
		WaitForSingleObject(hPCPSemaphore, INFINITE);
		float wait_time = (rand() % 400 + 100) / 100;

		// Aguarda o timer 2
		WaitForSingleObject(hTimer2, 1000 * wait_time);
		slot1 = (rand() % 10000);
		slot2 = (rand() % 10000);
		slot3 = (rand() % 10000);

		for (i = 0; i < 8; i++) {
			op1_aux[i] = alphanum[rand() % 35];
			op2_aux[i] = alphanum[rand() % 35];
			op3_aux[i] = alphanum[rand() % 35];
		}
		theTime = time(NULL);
		localtime_s(&atime, &theTime);
		hora = atime.tm_hour;
		minuto = atime.tm_min;
		segundos = atime.tm_sec;

		sprintf_s(msg, "%04i|%02i:%02i:%02i|%c%c%c%c%c%c%c%c|%04i|%c%c%c%c%c%c%c%c|%04i|%c%c%c%c%c%c%c%c|%04i", NSEQ, hora, minuto, segundos, op1_aux[0], op1_aux[1], op1_aux[2], op1_aux[3], op1_aux[4], op1_aux[5], op1_aux[6], op1_aux[7], slot1, op2_aux[0], op2_aux[1], op2_aux[2], op2_aux[3], op2_aux[4], op2_aux[5], op2_aux[6], op2_aux[7], slot2, op3_aux[0], op3_aux[1], op3_aux[2], op3_aux[3], op3_aux[4], op3_aux[5], op3_aux[6], op3_aux[7], slot3);
		
		// Aguarda o mutex 1
		WaitForSingleObject(l1Mutex, INFINITE);
		if (list1.size() == 200) {
			// Libera mutex 1
			ReleaseMutex(l1Mutex);
			while (list1.size() > 180)
				// Sinaliza que mensagem foi depositada na lista 1
				PulseEvent(hMessageList1);

			// Aguarda o mutex 1
			WaitForSingleObject(l1Mutex, INFINITE);
		}
		
		list1.push_back(msg);

		// Sinaliza que mensagem foi depositada na lista 1
		PulseEvent(hMessageList1);

		// Libera mutex 1
		ReleaseMutex(l1Mutex);

		NSEQ++;

		// Libera semáforo
		ReleaseSemaphore(hPCPSemaphore, 1, NULL);
	}
	return EXIT_SUCCESS;
}

DWORD WINAPI ThreadCapture() {
	HANDLE h[] = { l1Mutex,l2Mutex };
	BOOL bStatus;
	DWORD dwSentBytes;

	while (1) {
		// Aguarda o semáforo para sincronizar com Keyboard
		WaitForSingleObject(hMessageSemaphore, INFINITE);

		// Aguarda evento que mensagem foi depositada na lista 1
		WaitForSingleObject(hMessageList1, INFINITE);

		// Aguarda pelos 2 mutexes
		WaitForMultipleObjects(2, h, TRUE, INFINITE);

		auto it = list1.begin();
		const std::string& ref = *it;
		auto copy = *it;
		
		if (quoted(ref)._Size == 53) {
			list2.push_back(copy);

			// Sinaliza que mensagem foi depositada na lista 2
			PulseEvent(hMessageList2);
			list1.erase(it);
		}
		else if (quoted(ref)._Size == 55) {
			ostringstream ss1;
			ss1 << quoted(copy);
			string s = ss1.str();

			char * msg = new char[s.size() + 1];
			std::copy(s.begin(), s.end(), msg);
			msg[s.size()] = '\0';

			// Escreve no pipe
			bStatus = WriteFile(hPipe, msg, 58, &dwSentBytes, NULL);
			CheckForError(bStatus);
			delete[] msg;
			list1.erase(it);	
		}

		// Libera os dois mutexes
		ReleaseMutex(l1Mutex);
		ReleaseMutex(l2Mutex);

		// Libera semáforo
		ReleaseSemaphore(hMessageSemaphore, 1, NULL);
	}

	return EXIT_SUCCESS;
}

DWORD WINAPI ThreadProd() {
	string nseq,
		   temp1,
		   temp2,
		   temp3,
		   volume,
		   pressao,
		   tempo,
		   hora;

	while (1) {
		// Aguarda o semáforo para sincronizar com Keyboard
		WaitForSingleObject(hDisplaySemaphore, INFINITE);

		// Aguarda evento que mensagem foi depositada na lista 2
		WaitForSingleObject(hMessageList2, INFINITE);

		// Aguarda mutex 2
		WaitForSingleObject(l2Mutex, INFINITE);
		auto begin = list2.begin();
		auto element = *begin;
		ostringstream ss;
		ss << quoted(element);
		string copy = ss.str();

		nseq = copy.substr(1, 6);
		temp1 = copy.substr(8, 6);
		temp2 = copy.substr(15, 6);
		temp3 = copy.substr(22, 6);
		volume = copy.substr(29, 5);
		pressao = copy.substr(35, 5);
		tempo = copy.substr(41, 4);
		hora = copy.substr(46, 8);

		cout << "NSEQ:" << nseq << " " << hora << " TZ1:" << temp1 << " TZ2:" << temp2 << " TZ3:" << temp3 << " V:" << volume << " P:" << pressao << " Tempo:" << tempo << endl;
		list2.erase(begin);

		// Libera mutex 2
		ReleaseMutex(l2Mutex);

		// Libera semáforo
		ReleaseSemaphore(hDisplaySemaphore, 1, NULL);
	}

	return EXIT_SUCCESS;
}