#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <sstream>
#include <string.h>
#include <list>
#include <iomanip>
#include <time.h>
#include <stdlib.h>
#include <process.h>				//_beginthreadex() e _endthreadex()
using namespace std;

typedef unsigned (WINAPI *CAST_FUNCTION)(LPVOID);	//Casting para terceiro e sexto parâmetros da função
typedef unsigned *CAST_LPDWORD;

list <string> lista1, lista2;
HANDLE l1Mutex, l2Mutex, hCLP, hPCP, hCapture, hProd, hTimer1, hTimer2, msgDepositada1, msgDepositada2, listaCheia, semCLP, semPCP, semDisplay, semMessage;
#define _CHECKERROR	1		// Ativa função CheckForError
#include "../Include/checkforerror.h"


HANDLE hEscEvent,
hEscThread;

DWORD WINAPI EscFunc();	// declaração da função
DWORD WINAPI ThreadCLP();		    //Thread representando leitura de CLP
DWORD WINAPI ThreadPCP();		    //Thread representando leitura de CLP
DWORD WINAPI ThreadCapture();			//Thread representando carro
DWORD WINAPI ThreadProd();		    //Thread representando leitura de CLP
int main() {
	system("chcp 1252"); // Comando para apresentar caracteres especiais no console
	
	DWORD dwIdCLP, dwIdPCP, dwIdCapture, dwIdProd;
	DWORD dwReturn,
		dwExitCode,
		dwThreadId;

	l1Mutex = CreateMutex(NULL, FALSE, (LPCSTR)"Mutex da lista 1");
	l2Mutex = CreateMutex(NULL, FALSE, (LPCSTR) "Mutex da lista 2");

	listaCheia = CreateSemaphore(NULL, 0, 1, (LPCSTR) "Semaforo");

	hTimer1 = CreateEvent(NULL, FALSE, FALSE, (LPCSTR)"Timer da Tarefa CLP");
	hTimer2 = CreateEvent(NULL, FALSE, FALSE, (LPCSTR)"Timer da Tarefa PCP");
	msgDepositada1 = CreateEvent(NULL, FALSE, FALSE, (LPCSTR)"Avisa que uma msg foi depositada na lista 1");
	msgDepositada2 = CreateEvent(NULL, FALSE, FALSE, (LPCSTR)"Avisa que uma msg foi depositada na lista 2");

	hCLP = (HANDLE)_beginthreadex(NULL, 0, (CAST_FUNCTION)ThreadCLP, NULL, 0, (CAST_LPDWORD)&dwIdCLP);
	hPCP = (HANDLE)_beginthreadex(NULL, 0, (CAST_FUNCTION)ThreadPCP, NULL, 0, (CAST_LPDWORD)&dwIdPCP);
	hCapture = (HANDLE)_beginthreadex(NULL, 0, (CAST_FUNCTION)ThreadCapture, NULL, 0, (CAST_LPDWORD)&dwIdCapture);
	hProd = (HANDLE)_beginthreadex(NULL, 0, (CAST_FUNCTION)ThreadProd, NULL, 0, (CAST_LPDWORD)&dwIdProd);
	hEscThread = (HANDLE)_beginthreadex(NULL, 0, (CAST_FUNCTION)EscFunc, NULL, 0, (CAST_LPDWORD)&dwThreadId);
	CheckForError(hEscThread);

	semCLP = OpenSemaphore(SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, NULL, "CLP");
	semPCP = OpenSemaphore(SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, NULL, "PCP");
	semDisplay = OpenSemaphore(SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, NULL, "Display");
	semMessage = OpenSemaphore(SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, NULL, "Message");
	dwReturn = WaitForSingleObject(hEscThread, INFINITE);
	CheckForError(dwReturn);

	dwReturn = GetExitCodeThread(hEscThread, &dwExitCode);
	CheckForError(dwReturn);

	CloseHandle(hCLP);
	CloseHandle(hCapture);
	CloseHandle(hPCP);
	CloseHandle(hProd);
	CloseHandle(l1Mutex);
	CloseHandle(l2Mutex);
	CloseHandle(hEscThread);


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
	int NSEQ = 1, tempo, hora, minuto, segundos;
	float TZona1, TZona2, TZona3, volume, pressao;
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
		while (lista1.size() == 200) {
			WaitForSingleObject(listaCheia, INFINITE);
		}
		lista1.push_back(msg);
		PulseEvent(msgDepositada1);
		ReleaseMutex(l1Mutex);
		
		NSEQ++;
		ReleaseSemaphore(semCLP, 1, NULL);
	}

	return NULL;
}



DWORD WINAPI ThreadPCP() {
	int NSEQ = 1, slot1, slot2, slot3, hora, minuto, segundos, i = 0;
	char op1_aux[8], op2_aux[8], op3_aux[8];
	char msg[56];
	time_t theTime;
	struct tm atime;
	char alphanum[] = "123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
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
		while (lista1.size() == 200) {
			WaitForSingleObject(listaCheia, INFINITE);
		}
		lista1.push_back(msg);
		PulseEvent(msgDepositada1);
		ReleaseMutex(l1Mutex);


		NSEQ++;
		ReleaseSemaphore(semPCP, 1, NULL);
	}
	return 0;
}

DWORD WINAPI ThreadCapture() {
	while (1) {
		WaitForSingleObject(semMessage, INFINITE);
		WaitForSingleObject(msgDepositada1, INFINITE);
		HANDLE h[] = { l1Mutex,l2Mutex };
		WaitForMultipleObjects(2, h, TRUE, INFINITE);
		auto it = lista1.end();
		it--;
		/*while (it != lista1.end()) {
			const std::string& ref = *it;
			std::string copy = *it;
			if (quoted(ref)._Size == 53) {
				lista2.push_back(copy);
			}
			else if (quoted(ref)._Size == 55) {
				//comunicação mailslot
			}
			it++;
		}*/
		const std::string& ref = *it;
		std::string copy = *it;
		if (quoted(ref)._Size == 53) {
			lista2.push_back(copy);
			PulseEvent(msgDepositada2);
		}
		else if (quoted(ref)._Size == 55) {
			//comunicação mailslot
		}
		ReleaseMutex(l1Mutex);
		ReleaseMutex(l2Mutex);
		ReleaseSemaphore(semMessage, 1, NULL);
	}

	return 0;
}

DWORD WINAPI ThreadProd() {
	string nseq, temp1, temp2, temp3, volume, pressao, tempo, hora;
	while (1) {
		WaitForSingleObject(semDisplay, INFINITE);

		WaitForSingleObject(msgDepositada2, INFINITE);

		WaitForSingleObject(l2Mutex, INFINITE);
		auto it = lista2.back();
		ostringstream ss;
		ss << quoted(it);
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
		ReleaseMutex(l2Mutex);
		ReleaseSemaphore(semDisplay, 1, NULL);
	}
	return 0;
}