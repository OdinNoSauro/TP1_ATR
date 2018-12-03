#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>	// _beginthreadex() e _endthreadex() 
#include <conio.h>		// _getch

#define _CHECKERROR	1		// Ativa função CheckForError
#include "../Include/checkforerror.h"

// Casting para terceiro e sexto parâmetros da função _beginthreadex
typedef unsigned (WINAPI *CAST_FUNCTION)(LPVOID);
typedef unsigned *CAST_LPDWORD;

HANDLE hEscEvent,					// Handle para evento que aborta a execução
	   hMailslotEvent,				// Handle para evento de sincronização mailslot
	   hMailslot,					// Handle para mailslot
	   hEscThread,
	   hMailslotThread;

DWORD WINAPI EscFunc();	// declaração da função
DWORD WINAPI ClearFunc();	// declaração da função

int main() {
	system("chcp 1252"); // Comando para apresentar caracteres especiais no console

	BOOL bStatus;
	DWORD dwReturn,
		  dwExitCode,
		  dwThreadId;

	printf("Processo management");

	// Pega o handle para o evento de sincronização do mailslot
	hMailslotEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, "MailslotEvent");

	hEscThread = (HANDLE)_beginthreadex(
		NULL,
		0,
		(CAST_FUNCTION)EscFunc,	// casting necessário
		NULL,
		0,
		(CAST_LPDWORD)&dwThreadId	// casting necessário
	);

	hMailslotThread = (HANDLE)_beginthreadex(
		NULL,
		0,
		(CAST_FUNCTION)ClearFunc,	// casting necessário
		NULL,
		0,
		(CAST_LPDWORD)&dwThreadId	// casting necessário
	);

	// Criação do mailslot
	hMailslot = CreateMailslot(
		"\\\\.\\mailslot\\ManagementMailslot",
		0,
		MAILSLOT_WAIT_FOREVER,
		NULL);
	CheckForError(hMailslot != INVALID_HANDLE_VALUE);

	// A função de leitura retorna imediatamente se não houver mensagens
	//bStatus = SetMailslotInfo(hMailslot, 0);
	//CheckForError(bStatus);

	// Sinaliza que o mailslot foi criado
	SetEvent(hMailslotEvent);

	HANDLE semaphore = OpenSemaphore(SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, NULL, "Management");
	do {
		printf("\nWait no semáforo");
		WaitForSingleObject(semaphore, INFINITE);
		printf("\nPegou o semáforo");
		printf("\nSoltou o semáforo");
		ReleaseSemaphore(semaphore, 1, NULL);
	} while (true);
	
	CloseHandle(semaphore);
	// Fecha handle do evento
	CloseHandle(hEscEvent);
	CloseHandle(hMailslotEvent);

	// Fecha handle do mailslot
	CloseHandle(hMailslot);

	dwReturn = GetExitCodeThread(hEscThread, &dwExitCode);
	CheckForError(dwReturn);
	CloseHandle(hEscThread);	// apaga referência ao objeto

	printf("\nFechou handles");
	printf("\nAcione uma tecla para terminar\n");
	_getch(); // Pare aqui, caso não esteja executando no ambiente MDS

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

DWORD WINAPI ClearFunc() {
	BOOL bStatus;
	char sBuffer[6];
	DWORD dwReadBytes;

	do {
		if (hMailslot != INVALID_HANDLE_VALUE) {
			bStatus = ReadFile(hMailslot, &sBuffer, 6, &dwReadBytes, NULL);
			//CheckForError(bStatus);
			if (strcmp(sBuffer, "CLEAR") == 0) {
				printf("\nMensagem recebida: %s", sBuffer);
				system("cls");
			}
		}
	} while (true);
}