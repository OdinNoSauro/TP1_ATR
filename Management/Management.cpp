#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <string.h>
#include <process.h>	// _beginthreadex() e _endthreadex() 
#include <conio.h>		// _getch

using namespace std;

#define _CHECKERROR	1		// Ativa função CheckForError
#include "../Include/checkforerror.h"

// Casting para terceiro e sexto parâmetros da função _beginthreadex
typedef unsigned (WINAPI *CAST_FUNCTION)(LPVOID);
typedef unsigned *CAST_LPDWORD;

HANDLE hEscEvent,					// Handle para evento que aborta a execução
	   hPipeEventKeyboard,			// Handle para evento de sincronização do pipe com o processo Keyboard
	   hPipeEventDisplay,           // Handle para evento de sincronização do pipe com o processo Display
	   hEscThread,					// Handle para thread que aborta a execução
	   hClearThread,				// Handle para thread que limpa a tela
	   hPipe,						// Handle para o pipe
	   hSemaphore;					// Handle para semáforo que sincroniza com a thread Keyboard

DWORD WINAPI EscFunc();				   // Declaração da função que encerra a aplicação
DWORD WINAPI ClearFunc(LPVOID);	       // Declaração da função que limpa o console

int main() {
	system("chcp 1252");		   // Comando para apresentar caracteres especiais no console

	string s,
		   nseq,
		   op1,
		   op2,
		   op3,
		   slot1,
		   slot2,
		   slot3,
		   hora;

	BOOL bStatus;

	DWORD dwReturn,
		  dwExitCode,
		  dwThreadId,
		  dwErrorCode,
		  dwReadBytes;

	char sBuffer[58];
	int i;

	// Pega os handles para os eventos de sincronização do pipe
	hPipeEventKeyboard = OpenEvent(EVENT_MODIFY_STATE, FALSE, "PipeEventKeyboard");
	hPipeEventDisplay = OpenEvent(EVENT_MODIFY_STATE, FALSE, "PipeEventDisplay");

	// Pega handle para o semáforo
	hSemaphore = OpenSemaphore(SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, NULL, "Management");

	// Cria instância do pipe
	hPipe = CreateNamedPipe(
		"\\\\.\\pipe\\ManagementPipe",
		PIPE_ACCESS_DUPLEX,	// Comunicação Full Duplex
		PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
		2,			// Número de instâncias
		0,			// nOutBufferSize
		0,			// nInBufferSize
		1000,		// Timeout default para esperar por cliente
		NULL);		// Atributos de segurança
	CheckForError(hPipe != INVALID_HANDLE_VALUE);

	// Sinaliza para o processo Keyboard que o pipe foi criado
	SetEvent(hPipeEventKeyboard);

	bStatus = ConnectNamedPipe(hPipe, NULL); // Aguarda conexão

	// Faz o tratamento da resposta
	if (bStatus) {
		printf("\nCliente se conectou com sucesso");
	}
	else {
		dwErrorCode = GetLastError();
		if (dwErrorCode == ERROR_PIPE_CONNECTED) {
			printf("\nCliente já havia se conectado");
		}
		else if (dwErrorCode == ERROR_NO_DATA) {
			printf("\nCliente fechou seu handle");
			return 0;
		}
		else CheckForError(FALSE);
	}

	// Criação da tarefa que aborta a execução
	hEscThread = (HANDLE)_beginthreadex(
		NULL,
		0,
		(CAST_FUNCTION)EscFunc,
		NULL,
		0,
		(CAST_LPDWORD)&dwThreadId
	);

	// Criação da tarefa que limpa a tela
	hClearThread = (HANDLE)_beginthreadex(
		NULL,
		0,
		(CAST_FUNCTION)ClearFunc,
		(LPVOID)hPipe,
		0,
		(CAST_LPDWORD)&dwThreadId
	);

	// Cria instância do pipe
	hPipe = CreateNamedPipe(
		"\\\\.\\pipe\\ManagementPipe",
		PIPE_ACCESS_DUPLEX,	// Comunicação Full Duplex
		PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
		2,			// Número de instâncias
		0,			// nOutBufferSize
		0,			// nInBufferSize
		1000,		// Timeout default para esperar por cliente
		NULL);		// Atributos de segurança
	CheckForError(hPipe != INVALID_HANDLE_VALUE);

	// Sinaliza para o processo Display que o pipe foi criado
	SetEvent(hPipeEventDisplay);

	bStatus = ConnectNamedPipe(hPipe, NULL); // Aguarda conexão

	// Faz o tratamento da resposta
	if (bStatus) {
		printf("\nCliente se conectou com sucesso");
	}
	else {
		dwErrorCode = GetLastError();
		if (dwErrorCode == ERROR_PIPE_CONNECTED) {
			printf("\nCliente já havia se conectado");
		}
		else if (dwErrorCode == ERROR_NO_DATA) {
			printf("\nCliente fechou seu handle");
			return 0;
		}
		else CheckForError(FALSE);
	}

	do {
		// Preenche o buffer com valor zero em todas as posições
		for (i = 0; i < 57; i++) {
			sBuffer[i] = '0';
		}
		sBuffer[i] = '\0';

		// Aguarda o semáforo para sincronizar com Keyboard
		WaitForSingleObject(hSemaphore, INFINITE);

		// Lê mensagem do pipe
		bStatus = ReadFile(hPipe, &sBuffer, sizeof(sBuffer), &dwReadBytes, NULL);
		s = sBuffer;
		nseq = s.substr(1, 4);
		hora = s.substr(6, 8);
		op1 = s.substr(15, 8);
		slot1 = s.substr(24, 4);
		op2 = s.substr(29, 8);
		slot2 = s.substr(38, 4);
		op3 = s.substr(43, 8);
		slot3 = s.substr(52, 4);
		cout << nseq << " " << hora << " OP1:" << op1 << " [" << slot1 << "] OP2: " << op2 << " [" << slot2 << "] OP3:" << op3 << " [" << slot3 << "]" << endl;

		// Libera semáforo
		ReleaseSemaphore(hSemaphore, 1, NULL);
	} while (true);

	// Fecha handle do semáforo
	CloseHandle(hSemaphore);

	// Fecha handles dos eventos
	CloseHandle(hPipeEventKeyboard);
	CloseHandle(hPipeEventDisplay);

	dwReturn = GetExitCodeThread(hEscThread, &dwExitCode);
	CheckForError(dwReturn);
	CloseHandle(hEscThread);	// apaga referência ao objeto

	dwReturn = GetExitCodeThread(hClearThread, &dwExitCode);
	CheckForError(dwReturn);
	CloseHandle(hClearThread);	// apaga referência ao objeto

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

DWORD WINAPI ClearFunc(LPVOID lpvParam) {

	BOOL bStatus;
	char sBuffer[6];
	DWORD dwReadBytes;

	HANDLE hPipeParam = (HANDLE) lpvParam;

	bStatus = ConnectNamedPipe(hPipe, NULL);

	while (true) {
		// Lê mensagem do pipe
		bStatus = ReadFile(hPipeParam, &sBuffer, sizeof(sBuffer), &dwReadBytes, NULL);
		CheckForError(bStatus);

		// Aguarda o semáforo para sincronizar com Keyboard
		WaitForSingleObject(hSemaphore, INFINITE);
		if (strcmp(sBuffer, "CLEAR") == 0) {
			// Executa comando para limpar o console
			system("cls");
		}

		// Libera semáforo
		ReleaseSemaphore(hSemaphore, 1, NULL);
	}
}