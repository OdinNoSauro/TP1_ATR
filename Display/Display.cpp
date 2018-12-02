#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>	// _beginthreadex() e _endthreadex() 
#include <conio.h>		// _getch

#define _CHECKERROR	1		// Ativa fun��o CheckForError
#include "../Include/checkforerror.h"

// Casting para terceiro e sexto par�metros da fun��o _beginthreadex
typedef unsigned (WINAPI *CAST_FUNCTION)(LPVOID);
typedef unsigned *CAST_LPDWORD;

HANDLE hEscEvent,
	   hEscThread;

DWORD WINAPI EscFunc();	// declara��o da fun��o

int main() {
	system("chcp 1252"); // Comando para apresentar caracteres especiais no console

	DWORD dwReturn,
		  dwExitCode,
		  dwThreadId;

	printf("Processo display");

	hEscThread = (HANDLE)_beginthreadex(
		NULL,
		0,
		(CAST_FUNCTION)EscFunc,	// casting necess�rio
		NULL,
		0,
		(CAST_LPDWORD)&dwThreadId	// casting necess�rio
	);

	dwReturn = WaitForSingleObject(hEscThread, INFINITE);
	CheckForError(dwReturn);

	dwReturn = GetExitCodeThread(hEscThread, &dwExitCode);
	CheckForError(dwReturn);
	CloseHandle(hEscThread);	// apaga refer�ncia ao objeto

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