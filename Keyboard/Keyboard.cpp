#define WIN32_LEAN_AND_MEAN 
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>	// _beginthreadex() e _endthreadex() 
#include <conio.h>		// _getch

#define _CHECKERROR	1		// Ativa fun��o CheckForError
#include "../Include/CheckForError.h"

// Casting para terceiro e sexto par�metros da fun��o _beginthreadex
typedef unsigned (WINAPI *CAST_FUNCTION)(LPVOID);
typedef unsigned *CAST_LPDWORD;

#define	ESC			0x1B

DWORD WINAPI threadLista1Cheia();		    //Thread que faz o tratamento quando a lista 1 est� cheia
DWORD WINAPI threadLista2Cheia();			//Thread que faz o tratamento quando a lista 1 est� cheia

HANDLE  hCLPSemaphore,				// Handle para sem�foro da tarefa de leitura do CLP
hPCPSemaphore,				// Handle para sem�foro da tarefa de leitura do PCP
hMessageSemaphore,			// Handle para sem�foro da tarefa de retirada de mensagens
hManagementSemaphore,		// Handle para sem�foro da tarefa de gest�o da produ��o
hDisplaySemaphore,			// Handle para sem�foro da tarefa de exibi��o de eventos
hEscEvent,					// Handle para evento que aborta a execu��o
hMailslotEvent,				// Handle para evento de sincroniza��o mailslot
hTimer,						// Handle para timer
hMailslot,					// Handle para mailslot
hLista1Cheia,
hLista2Cheia;
int main() {
	system("chcp 1252");			// Comando para apresentar caracteres especiais no console
	//HANDLE hThread;

	//DWORD dwThreadId;
	DWORD dwExitCode = 0,
		dwSentBytes,
		dwLista1,
		dwLista2;
	PROCESS_INFORMATION npDisplay, npManagement;		// Informa��es sobre novo processo criado
	STARTUPINFO siDisplay, siManagement;				// StartUpInformation para novo processo
	BOOL bStatus;
	int nKey,
		nCLP = 0,
		nPCP = 0,
		nMessage = 0,
		nManagement = 0,
		nDisplay = 0;

	// Cria��o dos eventos
	hEscEvent = CreateEvent(NULL, TRUE, FALSE, "EscEvent");
	CheckForError(hEscEvent);
	hMailslotEvent = CreateEvent(NULL, TRUE, FALSE, "MailslotEvent");
	CheckForError(hMailslotEvent);
	
	
	hLista1Cheia = OpenSemaphore(SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, NULL, "Lista1Cheia");
	hLista2Cheia = OpenSemaphore(SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, NULL, "Lista2Cheia");

	// Cria��o dos sem�foros
	hCLPSemaphore = CreateSemaphore(NULL, 1, 1, "CLP");
	CheckForError(hCLPSemaphore);
	hPCPSemaphore = CreateSemaphore(NULL, 1, 1, "PCP");
	CheckForError(hPCPSemaphore);
	hMessageSemaphore = CreateSemaphore(NULL, 1, 1, "Message");
	CheckForError(hMessageSemaphore);
	hManagementSemaphore = CreateSemaphore(NULL, 1, 1, "Management");
	CheckForError(hManagementSemaphore);
	hDisplaySemaphore = CreateSemaphore(NULL, 1, 1, "Display");
	CheckForError(hDisplaySemaphore);

	// Cria��o do processo Display
	ZeroMemory(&siDisplay, sizeof(siDisplay));
	siDisplay.cb = sizeof(siDisplay);	// Tamanho da estrutura em bytes
	bStatus = CreateProcess(
		"..\\Debug\\Display.exe",	// Nome do processo
		NULL,						// Linha de comando a ser executada
		NULL,						// Atributos de seguran�a do processo
		NULL,						// Atributos de seguran�a da thread
		FALSE,						// Heran�a de handles
		CREATE_NEW_CONSOLE,			// CreationFlags
		NULL,						// lpEnvironment
		"..\\Debug\\",				// Endere�o do diret�rio atual
		&siDisplay,					// lpStartUpInfo
		&npDisplay);				// lpProcessInformation
	CheckForError(bStatus);

	// Cria��o do processo Management
	ZeroMemory(&siManagement, sizeof(siManagement));
	siManagement.cb = sizeof(siManagement);	// Tamanho da estrutura em bytes
	bStatus = CreateProcess(
		"..\\Debug\\Management.exe",	// Nome do processo
		NULL,							// Linha de comando a ser executada
		NULL,							// Atributos de seguran�a do processo
		NULL,							// Atributos de seguran�a da thread
		FALSE,							// Heran�a de handles
		CREATE_NEW_CONSOLE,				// CreationFlags
		NULL,							// lpEnvironment
		"..\\Debug\\",					// Endere�o do diret�rio atual
		&siManagement,					// lpStartUpInfo
		&npManagement);					// lpProcessInformation
	CheckForError(bStatus);

	// Aguarda que o mailslot seja criado pelo processo Management
	printf("Aguardando cria��o do mailslot\n");
	WaitForSingleObject(hMailslotEvent, INFINITE);

	//hEscEvent = OpenEvent(SYNCHRONIZE | EVENT_MODIFY_STATE, FALSE, "EscEvent");
	//CheckForError(hEscEvent);

	// Cria��o do pseudo-arquivo do mailslot
	hMailslot = CreateFile(
		"\\\\.\\mailslot\\ManagementMailslot",
		GENERIC_WRITE,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	CheckForError(hMailslot != INVALID_HANDLE_VALUE);

	// Iniciando threads segund�rias
	hLista1Cheia = (HANDLE)_beginthreadex(NULL, 0, (CAST_FUNCTION)threadLista1Cheia, NULL, 0, (CAST_LPDWORD)&dwLista1);
	hLista2Cheia = (HANDLE)_beginthreadex(NULL, 0, (CAST_FUNCTION)threadLista2Cheia, NULL, 0, (CAST_LPDWORD)&dwLista2);

	do {
		//system("cls");
		printf("Escolha uma das op��es abaixo para ativar ou bloquear a respectiva tarefa:\n");
		printf("<P> - Leitura de dados do CLP\n");
		printf("<S> - Leitura de dados do PCP\n");
		printf("<R> - Retirada de mensagens\n");
		printf("<G> - Gest�o da produ��o\n");
		printf("<D> - Exibi��o dos dados do processo\n");
		printf("<C> - Limpar a tela de gest�o da produ��o\n");
		printf("<ESC> - Finalizar aplica��o\n");
		nKey = _getch();
		if (nKey == 'p' || nKey == 'P') {
			printf("Tarefa de leitura do CLP\n");
			if (nCLP == 1) {
				ReleaseSemaphore(hCLPSemaphore, 1, NULL);
				nCLP = 0;
			}
			else {
				WaitForSingleObject(hCLPSemaphore, INFINITE);
				nCLP = 1;
			}
		}
		else if (nKey == 's' || nKey == 'S') {
			printf("Tarefa de leitura do PCP\n");
			if (nPCP == 1) {
				ReleaseSemaphore(hPCPSemaphore, 1, NULL);
				nPCP = 0;
			}
			else {
				WaitForSingleObject(hPCPSemaphore, INFINITE);
				nPCP = 1;
			}
		}
		else if (nKey == 'r' || nKey == 'R') {
			printf("Tarefa de retirada de mensagens\n");
			if (nMessage == 1) {
				ReleaseSemaphore(hMessageSemaphore, 1, NULL);
				nMessage = 0;
			}
			else {
				WaitForSingleObject(hMessageSemaphore, INFINITE);
				nMessage = 1;
			}
		}
		else if (nKey == 'g' || nKey == 'G') {
			printf("Tarefa de gest�o da produ��o\n");
			if (nManagement == 1) {
				ReleaseSemaphore(hManagementSemaphore, 1, NULL);
				nManagement = 0;
			}
			else {
				WaitForSingleObject(hManagementSemaphore, INFINITE);
				nManagement = 1;
			}
		}
		else if (nKey == 'd' || nKey == 'D') {
			printf("Tarefa de exibi��o de dados de processo\n");
			if (nDisplay == 1) {
				ReleaseSemaphore(hDisplaySemaphore, 1, NULL);
				nDisplay = 0;
			}
			else {
				WaitForSingleObject(hDisplaySemaphore, INFINITE);
				nDisplay = 1;
			}
		}
		else if (nKey == 'c' || nKey == 'C') {
			printf("Mensagem de limpeza de tela\n");
			bStatus = WriteFile(hMailslot, "CLEAR", 6, &dwSentBytes, NULL);
			CheckForError(bStatus);
		}
		else if (nKey == ESC) {
			SetEvent(hEscEvent);
		}
		else {
			printf("Tecla inv�lida\n");
		}
	} while (nKey != ESC);
	// Fecha handles dos processos
	CloseHandle(npDisplay.hProcess);
	CloseHandle(npDisplay.hThread);
	CloseHandle(npManagement.hProcess);
	CloseHandle(npManagement.hThread);

	// Fecha handles dos eventos
	CloseHandle(hEscEvent);
	CloseHandle(hMailslotEvent);
	CloseHandle(hLista1Cheia);
	CloseHandle(hLista2Cheia);

	// Fecha handles dos sem�foros
	CloseHandle(hCLPSemaphore);
	CloseHandle(hPCPSemaphore);
	CloseHandle(hMessageSemaphore);
	CloseHandle(hManagementSemaphore);
	CloseHandle(hDisplaySemaphore);

	// Fecha handle do mailslot
	CloseHandle(hMailslot);

	printf("\nAcione uma tecla para terminar\n");
	_getch(); // Pare aqui, caso n�o esteja executando no ambiente MDS

	return EXIT_SUCCESS;
}

DWORD WINAPI  threadLista1Cheia() {
	while (1) {
		WaitForSingleObject(hLista1Cheia, INFINITE);
		printf("A Lista 1 est� cheia, As tarefas de leitura do CLP e PCP foram bloquadas.\nPara prosseguir retome a atividade da tarefa de retirada de mensagens");
		ReleaseSemaphore(hLista1Cheia, 1, NULL);
	}
}

DWORD WINAPI threadLista2Cheia() {
	while (1) {
		WaitForSingleObject(hLista2Cheia, INFINITE);
		printf("A Lista 2 est� cheia, a tarefa de retirada de mensagens foi bloquada.\nPara prosseguir retome a atividade da tarefa de exibi��o de dados do processo");
	}
}