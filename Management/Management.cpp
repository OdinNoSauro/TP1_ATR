#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <process.h>	// _beginthreadex() e _endthreadex() 
#include <conio.h>		// _getch

#include <sstream>
#include <string.h>
using namespace std;

#define _CHECKERROR	1		// Ativa função CheckForError
#include "../Include/checkforerror.h"

// Casting para terceiro e sexto parâmetros da função _beginthreadex
typedef unsigned (WINAPI *CAST_FUNCTION)(LPVOID);
typedef unsigned *CAST_LPDWORD;

HANDLE hEscEvent,					// Handle para evento que aborta a execução
	   hMailslotEvent,				// Handle para evento de sincronização mailslot
	   hMailslot,					// Handle para mailslot
	   hEscThread,
	   hClearThread;

DWORD WINAPI EscFunc();	           // Declaração da função que encerra a aplicação
DWORD WINAPI ClearFunc();	       // Declaração da função que limpa o console

int main() {
	system("chcp 1252");		   // Comando para apresentar caracteres especiais no console

	BOOL bStatus;
	DWORD dwReturn,
		  dwExitCode,
		  dwThreadId;

	printf("Processo management");

	// Pega o handle para o evento de sincronização do mailslot
	hMailslotEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, "MailslotEvent");

	// Criação da tarefa que encerra o processo
	hEscThread = (HANDLE)_beginthreadex(
		NULL,
		0,
		(CAST_FUNCTION)EscFunc,	// casting necessário
		NULL,
		0,
		(CAST_LPDWORD)&dwThreadId	// casting necessário
	);

	// Criação da tarefa que limpa a tela
	hClearThread = (HANDLE)_beginthreadex(
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

	// Sinaliza que o mailslot foi criado
	SetEvent(hMailslotEvent);

	char sBuffer[55];
	DWORD dwReadBytes;
	//string sBuffer;
	HANDLE semaphore = OpenSemaphore(SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, NULL, "Management");
	do {
		printf("\nWait no semáforo");
		WaitForSingleObject(semaphore, INFINITE);
		printf("\nPegou o semáforo");
		if (hMailslot != INVALID_HANDLE_VALUE) {
			//cout << "\nVai ler do mailslot" << endl;
			bStatus = ReadFile(hMailslot, &sBuffer, sizeof(sBuffer), &dwReadBytes, NULL);
			//cout << "\nMensagem recebida: " << sBuffer << endl;
			if (strcmp(sBuffer, "CLEAR") != 0) {
				printf("\nMensagem recebida: %s", sBuffer);
			}
		}
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

	dwReturn = GetExitCodeThread(hClearThread, &dwExitCode);
	CheckForError(dwReturn);
	CloseHandle(hClearThread);	// apaga referência ao objeto

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

	while (true) {
		if (hMailslot != INVALID_HANDLE_VALUE) {
			bStatus = ReadFile(hMailslot, &sBuffer, 54, &dwReadBytes, NULL);
			//CheckForError(bStatus);
			if (strcmp(sBuffer, "CLEAR") == 0) {
				printf("\nMensagem recebida: %s", sBuffer);
				system("cls");
			}
		}
	}
}