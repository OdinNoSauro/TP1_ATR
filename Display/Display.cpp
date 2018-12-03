#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>	// _beginthreadex() e _endthreadex() 
#include <conio.h>		// _getch

// bibliotecas do Gabe
#include <iostream>
#include <sstream>
#include <string.h>
#include <iomanip>
#include <time.h>

using namespace std;

#define _CHECKERROR	1		// Ativa função CheckForError
#include "../Include/checkforerror.h"

// Casting para terceiro e sexto parâmetros da função _beginthreadex
typedef unsigned (WINAPI *CAST_FUNCTION)(LPVOID);
typedef unsigned *CAST_LPDWORD;

HANDLE hEscEvent,
	   hEscThread,
	   hMailslotEvent,
	   hMailslot;

DWORD WINAPI EscFunc();	// declaração da função

int main() {
	system("chcp 1252"); // Comando para apresentar caracteres especiais no console

	DWORD dwReturn,
		  dwExitCode,
		  dwThreadId,
		  dwSentBytes;

	BOOL bStatus;

	printf("Processo display");

	hEscThread = (HANDLE)_beginthreadex(
		NULL,
		0,
		(CAST_FUNCTION)EscFunc,	// casting necessário
		NULL,
		0,
		(CAST_LPDWORD)&dwThreadId	// casting necessário
	);

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

	int NSEQ = 1, tempo, hora, minuto, segundos;
	float TZona1, TZona2, TZona3, volume, pressao;
	char msg[54];
	std::ostringstream ss;
	time_t theTime;
	struct tm atime;
	srand(time(NULL));

	HANDLE semaphore = OpenSemaphore(SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, NULL, "Display");
	do {
		printf("\nWait no semáforo");
		WaitForSingleObject(semaphore, INFINITE);
		printf("\nPegou o semáforo");
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
		bStatus = WriteFile(hMailslot, &msg, sizeof(msg), &dwSentBytes, NULL);
		CheckForError(bStatus);
		printf("\nSoltou o semáforo");
		ReleaseSemaphore(semaphore, 1, NULL);
		Sleep(1000);
	} while (true);

	dwReturn = WaitForSingleObject(hEscThread, INFINITE);
	//CheckForError(dwReturn);

	dwReturn = GetExitCodeThread(hEscThread, &dwExitCode);
	//CheckForError(dwReturn);
	CloseHandle(hEscThread);	// apaga referência ao objeto

	CloseHandle(semaphore);

	// Fecha handles dos eventos
	CloseHandle(hMailslotEvent);

	// Fecha handle do mailslot
	CloseHandle(hMailslot);


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