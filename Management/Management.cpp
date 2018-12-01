#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <process.h>	// _beginthreadex() e _endthreadex() 
#include <conio.h>		// _getch

#define _CHECKERROR	1		// Ativa função CheckForError
#include "../Include/checkforerror.h"

HANDLE hEscEvent,					// Handle para evento que aborta a execução
	   hMailslotEvent,				// Handle para evento de sincronização mailslot
	   hMailslot;					// Handle para mailslot

using namespace std;

int main() {
	system("chcp 1252"); // Comando para apresentar caracteres especiais no console

	BOOL bStatus;
	char sBuffer[6];
	DWORD dwReadBytes,
		  dwReturn;

	printf("Processo management");

	// Pega o handle para o evento de sincronização do mailslot
	hMailslotEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, "MailslotEvent");

	// Pega o handle para o evento que aborta
	hEscEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, "EscEvent");

	// Criação do mailslot
	hMailslot = CreateMailslot(
		"\\\\.\\mailslot\\ManagementMailslot",
		0,
		MAILSLOT_WAIT_FOREVER,
		NULL);
	CheckForError(hMailslot != INVALID_HANDLE_VALUE);

	printf("\nAcione uma tecla para continuar\n");
	_getch(); // Pare aqui, caso não esteja executando no ambiente MDS

	// Sinaliza que o mailslot foi criado
	SetEvent(hMailslotEvent);

	if (hMailslot != INVALID_HANDLE_VALUE) {
		bStatus = ReadFile(hMailslot, &sBuffer, 6, &dwReadBytes, NULL);
		CheckForError(bStatus);
		if (strcmp(sBuffer, "CLEAR") == 0) {
			printf("\nMensagem recebida: %s", sBuffer);
			system("cls");
		}
	}
	HANDLE semaphore = OpenSemaphore(SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, NULL, "Display");
	do
	{
		printf("\nWait no semáforo");
		WaitForSingleObject(semaphore, INFINITE);
		printf("\nPegou o semáforo");
		printf("\nSoltou o semáforo");
		ReleaseSemaphore(semaphore, 1, NULL);
	} while (WaitForSingleObject(hEscEvent, INFINITE) != WAIT_OBJECT_0);
	
	CloseHandle(semaphore);
	// Fecha handle do evento
	CloseHandle(hEscEvent);
	CloseHandle(hMailslotEvent);

	// Fecha handle do mailslot
	CloseHandle(hMailslot);

	printf("\nAcione uma tecla para terminar\n");
	_getch(); // Pare aqui, caso não esteja executando no ambiente MDS

	ExitProcess(EXIT_SUCCESS);
	return EXIT_SUCCESS;
}