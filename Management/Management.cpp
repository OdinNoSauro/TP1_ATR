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
	   hPipeEventKeyboard,
	   hPipeEventDisplay,           // Handle para evento de sincronização mailslot
	   hMailslot,					// Handle para mailslot
	   hEscThread,
	   hClearThread,
	   hPipe;

DWORD WINAPI EscFunc();	           // Declaração da função que encerra a aplicação
DWORD WINAPI ClearFunc(LPVOID);	       // Declaração da função que limpa o console

int main() {
	system("chcp 1252");		   // Comando para apresentar caracteres especiais no console

	BOOL bStatus;

	DWORD dwReturn,
		  dwExitCode,
		  dwThreadId,
		  dwErrorCode;

	printf("Processo management");

	// Pega o handle para o evento de sincronização do mailslot
	hPipeEventKeyboard = OpenEvent(EVENT_MODIFY_STATE, FALSE, "PipeEventKeyboard");
	hPipeEventDisplay = OpenEvent(EVENT_MODIFY_STATE, FALSE, "PipeEventDisplay");

	// Cria instância de um pipe: neste caso cria 3
	hPipe = CreateNamedPipe(
		"\\\\.\\pipe\\ManagementPipe",
		PIPE_ACCESS_DUPLEX,	// Comunicação Full Duplex
		PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
		3,			// Número de instâncias
		0,			// nOutBufferSize
		0,			// nInBufferSize
		1000,		// Timeout default para esperar por cliente
		NULL);		// Atributos de segurança
	CheckForError(hPipe != INVALID_HANDLE_VALUE);

	// Sinaliza que o mailslot foi criado
	SetEvent(hPipeEventKeyboard);

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
		(LPVOID)hPipe,
		0,
		(CAST_LPDWORD)&dwThreadId	// casting necessário
	);

	// Cria instância de um pipe: neste caso cria 3
	hPipe = CreateNamedPipe(
		"\\\\.\\pipe\\ManagementPipe",
		PIPE_ACCESS_DUPLEX,	// Comunicação Full Duplex
		PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
		3,			// Número de instâncias
		0,			// nOutBufferSize
		0,			// nInBufferSize
		1000,		// Timeout default para esperar por cliente
		NULL);		// Atributos de segurança
	CheckForError(hPipe != INVALID_HANDLE_VALUE);

	// Sinaliza que o mailslot foi criado
	SetEvent(hPipeEventDisplay);

	bStatus = ConnectNamedPipe(hPipe, NULL); // Fica preso esperando conexão
	if (bStatus) {
		printf("\nCliente se conectou com sucesso");
	}
	else {
		dwErrorCode = GetLastError();
		if (dwErrorCode == ERROR_PIPE_CONNECTED) {
			printf("\nCliente já havia se conectado");
		}  // if
		else if (dwErrorCode == ERROR_NO_DATA) {
			printf("\nCliente fechou seu handle");
			return 0;
		} // if
		else CheckForError(FALSE);
	}  // else
	
	string s;
	DWORD dwReadBytes;
	HANDLE semaphore = OpenSemaphore(SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, NULL, "Management");
	char *sBuffer = (char*)malloc(58 * sizeof(char));
	int i;

	do {
		for (i = 0; i < 57; i++) {
			sBuffer[i] = '0';
		}
		sBuffer[i] = '\0';
		printf("\nWait no semáforo");
		WaitForSingleObject(semaphore, INFINITE);
		printf("\nPegou o semáforo");
		bStatus = ReadFile(hPipe, sBuffer, 58, &dwReadBytes, NULL);
		printf("\n%s	%i	\n", sBuffer, dwReadBytes);		
		printf("Soltou o semáforo");
		ReleaseSemaphore(semaphore, 1, NULL);
	} while (true);
	
	delete[] sBuffer;
	CloseHandle(semaphore);

	// Fecha handle do evento
	CloseHandle(hEscEvent);
	CloseHandle(hPipeEventKeyboard);
	CloseHandle(hPipeEventDisplay);

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

DWORD WINAPI ClearFunc(LPVOID lpvParam) {
	BOOL bStatus;
	char sBuffer[6];
	DWORD dwReadBytes;

	HANDLE hPipeParam = (HANDLE) lpvParam;

	bStatus = ConnectNamedPipe(hPipe, NULL);

	while (true) {
		bStatus = ReadFile(hPipeParam, &sBuffer, sizeof(sBuffer), &dwReadBytes, NULL);
		CheckForError(bStatus);
		if (strcmp(sBuffer, "CLEAR") == 0) {
			printf("\nMensagem recebida: %s", sBuffer);
			system("cls");
		}
	}
}