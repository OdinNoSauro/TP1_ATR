#define WIN32_LEAN_AND_MEAN 
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>	// _beginthreadex() e _endthreadex() 
#include <conio.h>		// _getch

#define _CHECKERROR	1		// Ativa função CheckForError
#include "../Include/CheckForError.h"

// Casting para terceiro e sexto parâmetros da função _beginthreadex
typedef unsigned (WINAPI *CAST_FUNCTION)(LPVOID);
typedef unsigned *CAST_LPDWORD;

#define	ESC			0x1B

HANDLE  hCLPSemaphore,				// Handle para semáforo da tarefa de leitura do CLP
		hPCPSemaphore,				// Handle para semáforo da tarefa de leitura do PCP
		hMessageSemaphore,			// Handle para semáforo da tarefa de retirada de mensagens
		hManagementSemaphore,		// Handle para semáforo da tarefa de gestão da produção
		hDisplaySemaphore,			// Handle para semáforo da tarefa de exibição de eventos
		hEscEvent,					// Handle para evento que aborta a execução
		hPipeEventKeyboard,
	    hPipeEventDisplay,			// Handle para evento de sincronização pipe
		hTimer,						// Handle para timer
		hMailslot,					// Handle para mailslot
		hLista1Cheia,
		hLista2Cheia,
		hPipe;

int main() {
	system("chcp 1252");			// Comando para apresentar caracteres especiais no console

	DWORD dwExitCode,
		  dwSentBytes;

	LPTSTR lpszPipename = "\\\\.\\pipe\\ManagementPipe";
	PROCESS_INFORMATION npDisplay, npManagement;		// Informações sobre novo processo criado
	STARTUPINFO siDisplay, siManagement;				// StartUpInformation para novo processo
	BOOL bStatus;
	int nKey,
		nCLP = 0,
		nPCP = 0,
		nMessage = 0,
		nManagement = 0,
		nDisplay = 0;

	// Criação dos eventos
	hEscEvent = CreateEvent(NULL, TRUE, FALSE, "EscEvent");
	CheckForError(hEscEvent);
	hPipeEventKeyboard = CreateEvent(NULL, TRUE, FALSE, "PipeEventKeyboard");
	CheckForError(hPipeEventKeyboard);
	hPipeEventDisplay = CreateEvent(NULL, TRUE, FALSE, "PipeEventDisplay");
	CheckForError(hPipeEventDisplay);
	hLista1Cheia = CreateEvent(NULL, FALSE, FALSE, "List1Full");
	CheckForError(hLista1Cheia);
	hLista2Cheia = CreateEvent(NULL, FALSE, FALSE, "List2Full");
	CheckForError(hLista2Cheia);

	// Criação dos semáforos
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

	// Criação do processo Display
	ZeroMemory(&siDisplay, sizeof(siDisplay));
	siDisplay.cb = sizeof(siDisplay);	// Tamanho da estrutura em bytes
	bStatus = CreateProcess(
		"..\\Debug\\Display.exe",	// Nome do processo
		NULL,						// Linha de comando a ser executada
		NULL,						// Atributos de segurança do processo
		NULL,						// Atributos de segurança da thread
		FALSE,						// Herança de handles
		CREATE_NEW_CONSOLE,			// CreationFlags
		NULL,						// lpEnvironment
		"..\\Debug\\",				// Endereço do diretório atual
		&siDisplay,					// lpStartUpInfo
		&npDisplay);				// lpProcessInformation
	CheckForError(bStatus);

	// Criação do processo Management
	ZeroMemory(&siManagement, sizeof(siManagement));
	siManagement.cb = sizeof(siManagement);	// Tamanho da estrutura em bytes
	bStatus = CreateProcess(
		"..\\Debug\\Management.exe",	// Nome do processo
		NULL,							// Linha de comando a ser executada
		NULL,							// Atributos de segurança do processo
		NULL,							// Atributos de segurança da thread
		FALSE,							// Herança de handles
		CREATE_NEW_CONSOLE,				// CreationFlags
		NULL,							// lpEnvironment
		"..\\Debug\\",					// Endereço do diretório atual
		&siManagement,					// lpStartUpInfo
		&npManagement);					// lpProcessInformation
	CheckForError(bStatus);

	// Aguarda que o mailslot seja criado pelo processo Management
	//printf("Aguardando criação do mailslot\n");
	//WaitForSingleObject(hMailslotEvent, INFINITE);

	// Aguarda que o pipe seja criado pelo processo Management
	printf("Aguardando criação do pipe\n");
	WaitForSingleObject(hPipeEventKeyboard, INFINITE);

	// Criação do pseudo-arquivo do mailslot
	//hMailslot = CreateFile(
	//	"\\\\.\\mailslot\\ManagementMailslot",
	//	GENERIC_WRITE,
	//	FILE_SHARE_READ,
	//	NULL,
	//	OPEN_EXISTING,
	//	FILE_ATTRIBUTE_NORMAL,
	//	NULL);
	//CheckForError(hMailslot != INVALID_HANDLE_VALUE);

	hPipe = CreateFile(
		lpszPipename,   // nome do pipe 
		GENERIC_WRITE,  // acesso para escrita 
		FILE_SHARE_WRITE,              // sem compartilhamento 
		NULL,           // lpSecurityAttributes
		OPEN_EXISTING,  // dwCreationDistribution 
		0,              // dwFlagsAndAttributes 
		NULL);          // hTemplate
	//CheckForError(hPipe != INVALID_HANDLE_VALUE);

	WaitNamedPipe(lpszPipename, NMPWAIT_USE_DEFAULT_WAIT);

	do {
		//system("cls");
		printf("Escolha uma das opções abaixo para ativar ou bloquear a respectiva tarefa:\n");
		printf("<P> - Leitura de dados do CLP\n");
		printf("<S> - Leitura de dados do PCP\n");
		printf("<R> - Retirada de mensagens\n");
		printf("<G> - Gestão da produção\n");
		printf("<D> - Exibição dos dados do processo\n");
		printf("<C> - Limpar a tela de gestão da produção\n");
		printf("<ESC> - Finalizar aplicação\n");
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
			printf("Tarefa de gestão da produção\n");
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
			printf("Tarefa de exibição de dados de processo\n");
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
			bStatus = WriteFile(hPipe, "CLEAR", 6, &dwSentBytes, NULL);
			CheckForError(bStatus);
		}
		else if (nKey == ESC) {
			SetEvent(hEscEvent);
		}
		else {
			printf("Tecla inválida\n");
		}
	} while (nKey != ESC);


	// Fecha handles dos processos
	CloseHandle(npDisplay.hProcess);
	CloseHandle(npDisplay.hThread);
	CloseHandle(npManagement.hProcess);
	CloseHandle(npManagement.hThread);

	// Fecha handles dos eventos
	CloseHandle(hEscEvent);
	CloseHandle(hPipeEventKeyboard);
	CloseHandle(hPipeEventDisplay);
	CloseHandle(hLista1Cheia);
	CloseHandle(hLista2Cheia);

	// Fecha handles dos semáforos
	CloseHandle(hCLPSemaphore);
	CloseHandle(hPCPSemaphore);
	CloseHandle(hMessageSemaphore);
	CloseHandle(hManagementSemaphore);
	CloseHandle(hDisplaySemaphore);

	// Fecha handle do mailslot
	//CloseHandle(hMailslot);

	// Fecha handle do Pipe
	CloseHandle(hPipe);

	printf("\nAcione uma tecla para terminar\n");
	_getch(); // Pare aqui, caso não esteja executando no ambiente MDS

	return EXIT_SUCCESS;
}