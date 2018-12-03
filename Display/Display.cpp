#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <sstream>
#include <string.h>
#include <list>
#include <iomanip>
#include <time.h>
#include <process.h>				//_beginthreadex() e _endthreadex()
#include <conio.h>		// _getch


#define _CHECKERROR	1		// Ativa função CheckForError
#include "../Include/checkforerror.h"

typedef unsigned (WINAPI *CAST_FUNCTION)(LPVOID);	//Casting para terceiro e sexto parâmetros da função
typedef unsigned *CAST_LPDWORD;

using namespace std;

list <string> lista1, 
			  lista2;

HANDLE l1Mutex, 
	   l2Mutex,
	   hCLP,
	   hPCP,
	   hCapture,
	   hProd,
	   hTimer1,
	   hTimer2,
	   msgDepositada1,
	   msgDepositada2,
	   listaCheia,
	   semCLP,
	   semPCP,
	   semDisplay,
	   semMessage,
	   hEscEvent,
	   hEscThread,
	   hMailslotEvent,
	   hMailslot;

int offset = 0, modo = 0;

DWORD WINAPI EscFunc();	            // Declaração da função que encerra a aplicação
DWORD WINAPI ThreadCLP();		    // Thread representando leitura de CLP
DWORD WINAPI ThreadPCP();		    // Thread representando leitura de CLP
DWORD WINAPI ThreadCapture();	    // Thread representando carro
DWORD WINAPI ThreadProd();		    // Thread representando leitura de CLP

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
	
	// Criação dos mutexes
	l1Mutex = CreateMutex(NULL, FALSE, "MutexList1");
	l2Mutex = CreateMutex(NULL, FALSE, "MutexList2");

	// Criação dos semáforos
	listaCheia = CreateSemaphore(NULL, 0, 1, "FullQueueSemaphore");

	// Criação dos eventos
	hTimer1 = CreateEvent(NULL, FALSE, FALSE, "CLPTimer");
	hTimer2 = CreateEvent(NULL, FALSE, FALSE, "PCPTimer");
	msgDepositada1 = CreateEvent(NULL, FALSE, FALSE, "MessageList1");
	msgDepositada2 = CreateEvent(NULL, FALSE, FALSE, "MessageList2");

	// Pega os handles para os semáforos que bloquea/desbloquea as tarefas
	semCLP = OpenSemaphore(SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, NULL, "CLP");
	semPCP = OpenSemaphore(SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, NULL, "PCP");
	semDisplay = OpenSemaphore(SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, NULL, "Display");
	semMessage = OpenSemaphore(SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, NULL, "Message");
	hMailslotEvent = OpenEvent(SYNCHRONIZE, FALSE, "MailslotEvent");
	// Aguarda que o mailslot seja criado pelo processo Management
	printf("Aguardando criação do mailslot\n");
	WaitForSingleObject(hMailslotEvent, INFINITE);

	// Criação do pseudo-arquivo do mailslot
	hMailslot = CreateFile(
		"\\\\.\\mailslot\\ManagementMailslot",
		GENERIC_WRITE,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	//CheckForError(hMailslot != INVALID_HANDLE_VALUE);

	// Criação da tarefa de leitura do CLP
	hCLP = (HANDLE)_beginthreadex(
		NULL, 
		0, 
		(CAST_FUNCTION)ThreadCLP, 
		NULL, 
		0, 
		(CAST_LPDWORD)&dwIdCLP
	);
	//CheckForError(hCLP);

	// Criação da tarefa de leitura do PCP
	hPCP = (HANDLE)_beginthreadex(
		NULL,
		0,
		(CAST_FUNCTION)ThreadPCP,
		NULL,
		0,
		(CAST_LPDWORD)&dwIdPCP
	);
	//CheckForError(hPCP);

	// Criação da tarefa que retira mensagens de uma lista e passa pra outra
	hCapture = (HANDLE)_beginthreadex(
		NULL,
		0,
		(CAST_FUNCTION)ThreadCapture,
		NULL,
		0,
		(CAST_LPDWORD)&dwIdCapture
	);
	//CheckForError(hCapture);

	// Criação da tarefa que retira mensagens da segunda lista e imprime na tela
	hProd = (HANDLE)_beginthreadex(
		NULL,
		0,
		(CAST_FUNCTION)ThreadProd,
		NULL,
		0,
		(CAST_LPDWORD)&dwIdProd
	);
	//CheckForError(hProd);

	// Criação da tarefa que encerra o processo
	hEscThread = (HANDLE)_beginthreadex(
		NULL,
		0,
		(CAST_FUNCTION)EscFunc,
		NULL,
		0,
		(CAST_LPDWORD)&dwThreadId
	);
	//CheckForError(hEscThread);

	dwReturn = WaitForSingleObject(hEscThread, INFINITE);
	//CheckForError(dwReturn);
	
	// Fecha handles das threads
	dwReturn = GetExitCodeThread(hCLP, &dwExitCode);
	//CheckForError(dwReturn);
	CloseHandle(hCLP);

	dwReturn = GetExitCodeThread(hPCP, &dwExitCode);
	//CheckForError(dwReturn);
	CloseHandle(hPCP);

	dwReturn = GetExitCodeThread(hCapture, &dwExitCode);
	//CheckForError(dwReturn);
	CloseHandle(hCapture);

	dwReturn = GetExitCodeThread(hProd, &dwExitCode);
	//CheckForError(dwReturn);
	CloseHandle(hProd);

	dwReturn = GetExitCodeThread(hEscThread, &dwExitCode);
	//CheckForError(dwReturn);
	CloseHandle(hEscThread);

	// Fecha handle dos mutexes
	CloseHandle(l1Mutex);
	CloseHandle(l2Mutex);

	// Fecha handles dos eventos
	CloseHandle(hMailslotEvent);
	CloseHandle(hTimer1);
	CloseHandle(hTimer2);
	CloseHandle(msgDepositada1);
	CloseHandle(msgDepositada2);

	// Fecha handle do mailslot
	CloseHandle(hMailslot);

	// Fecha handles dos semáforos
	CloseHandle(semCLP);
	CloseHandle(semPCP);
	CloseHandle(semDisplay);
	CloseHandle(semMessage);

	return EXIT_SUCCESS;

}

DWORD WINAPI EscFunc() {
	// Pega o handle para o evento que aborta
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
		WaitForSingleObject(semCLP, INFINITE);
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
		WaitForSingleObject(l1Mutex, INFINITE);

		auto it = lista1.begin();

		if (lista1.size() == 200) {
			printf("\nLista 1 cheia");
			ReleaseMutex(l1Mutex);
		//	WaitForSingleObject(listaCheia, INFINITE);
			while (lista1.size() > 180)
				PulseEvent(msgDepositada1);
			WaitForSingleObject(l1Mutex, INFINITE);
		}		
		
		lista1.push_back(msg);
		PulseEvent(msgDepositada1);
		ReleaseMutex(l1Mutex);
		
		NSEQ++;
		ReleaseSemaphore(semCLP, 1, NULL);
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
		WaitForSingleObject(semPCP, INFINITE);
		float wait_time = (rand() % 400 + 100) / 100;
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
		WaitForSingleObject(l1Mutex, INFINITE);
		if (lista1.size() == 200) {
			printf("\nLista 1 cheia");
			ReleaseMutex(l1Mutex);
			while (lista1.size() > 180)
				PulseEvent(msgDepositada1);
			WaitForSingleObject(l1Mutex, INFINITE);
		}
		
		lista1.push_back(msg);
		PulseEvent(msgDepositada1);
		ReleaseMutex(l1Mutex);

		NSEQ++;
		ReleaseSemaphore(semPCP, 1, NULL);
	}
	return EXIT_SUCCESS;
}

DWORD WINAPI ThreadCapture() {
	HANDLE h[] = { l1Mutex,l2Mutex };
	BOOL bStatus;
	DWORD dwSentBytes;

	string nseq,
		   temp1,
		   temp2,
		   temp3,
		   volume,
		   pressao,
		   tempo,
		   hora;

	while (1) {
		WaitForSingleObject(semMessage, INFINITE);
		WaitForSingleObject(msgDepositada1, INFINITE);
		WaitForMultipleObjects(2, h, TRUE, INFINITE);

		while (lista2.size() == 100) {
			printf("\nLista 2 cheia");
		}
		auto it = lista1.begin();
		const std::string& ref = *it;
		auto copy = *it;
		
		if (quoted(ref)._Size == 53) {
			lista2.push_back(copy);
			PulseEvent(msgDepositada2);
			lista1.erase(it);
		}
		else if (quoted(ref)._Size == 55) {
			ostringstream ss1;
			ss1 << quoted(copy);
			string s = ss1.str();

			nseq = s.substr(1, 6);
			temp1 = s.substr(8, 6);
			temp2 = s.substr(15, 6);
			temp3 = s.substr(22, 6);
			volume = s.substr(29, 5);
			pressao = s.substr(35, 5);
			tempo = s.substr(41, 4);
			hora = s.substr(46, 8);

			//cout << "NSEQ:" << nseq << " " << hora << " TZ1:" << temp1 << " TZ2:" << temp2 << " TZ3:" << temp3 << " V:" << volume << " P:" << pressao << " Tempo:" << tempo << endl;
			char * msg = new char[s.size() + 1];
			std::copy(s.begin(), s.end(), msg);
			msg[s.size()] = '\0'; // don't forget the terminating
			printf("%s\n",msg);
		//	bStatus = WriteFile(hMailslot, msg, 58, &dwSentBytes, NULL);
			//CheckForError(bStatus);
			delete[] msg;
			lista1.erase(it);	
		}
		ReleaseMutex(l1Mutex);
		ReleaseMutex(l2Mutex);
		ReleaseSemaphore(semMessage, 1, NULL);
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
		WaitForSingleObject(semDisplay, INFINITE);

		WaitForSingleObject(msgDepositada2, INFINITE);

		WaitForSingleObject(l2Mutex, INFINITE);
		auto begin = lista2.begin();
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
		lista2.erase(begin);
		ReleaseMutex(l2Mutex);
		ReleaseSemaphore(semDisplay, 1, NULL);
	}

	return EXIT_SUCCESS;
}