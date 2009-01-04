/**
 * SMSD Windows Service
 */
/* Copyright (c) 2009 Michal Cihar <michal@cihar.com> */
/* Licensend under GNU GPL 2 */

#include <stdio.h>
#include <windows.h>
#include <winsvc.h>
#include <time.h>
#include <gammu-smsd.h>

#include "service.h"

char smsd_service_name_intern[] = "GammuSMSD";
char *smsd_service_name = smsd_service_name_intern;

SERVICE_STATUS m_ServiceStatus;

SERVICE_STATUS_HANDLE m_ServiceStatusHandle;

void WINAPI SMSDServiceCtrlHandler(DWORD Opcode)
{
	switch (Opcode) {
		case SERVICE_CONTROL_PAUSE:
			m_ServiceStatus.dwCurrentState = SERVICE_PAUSED;
			break;
		case SERVICE_CONTROL_CONTINUE:
			m_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
			break;
		case SERVICE_CONTROL_STOP:
			m_ServiceStatus.dwWin32ExitCode = 0;
			m_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
			m_ServiceStatus.dwCheckPoint = 0;
			m_ServiceStatus.dwWaitHint = 0;

			SetServiceStatus(m_ServiceStatusHandle,
					 &m_ServiceStatus);
			SMSD_Shutdown(config);
			break;
		case SERVICE_CONTROL_INTERROGATE:
			break;
	}
	return;
}


void WINAPI ServiceMain(DWORD argc, LPTSTR * argv)
{
	GSM_Error error;
	m_ServiceStatus.dwServiceType = SERVICE_WIN32;
	m_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
	m_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
	m_ServiceStatus.dwWin32ExitCode = 0;
	m_ServiceStatus.dwServiceSpecificExitCode = 0;
	m_ServiceStatus.dwCheckPoint = 0;
	m_ServiceStatus.dwWaitHint = 0;

	m_ServiceStatusHandle = RegisterServiceCtrlHandler(smsd_service_name,
							   SMSDServiceCtrlHandler);
	if (m_ServiceStatusHandle == (SERVICE_STATUS_HANDLE) 0) {
		return;
	}
	m_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
	m_ServiceStatus.dwCheckPoint = 0;
	m_ServiceStatus.dwWaitHint = 0;
	if (!SetServiceStatus(m_ServiceStatusHandle, &m_ServiceStatus)) {
		return;
	}

	error = SMSD_MainLoop(config);
	if (error != ERR_NONE) {
		SMSD_Terminate(config, "Failed to run SMSD", error, true, 2);
	}
	return;
}

bool install_smsd_service(SMSD_Parameters *params)
{
	char strDir[1024];

	HANDLE schSCManager, schService;

	LPCTSTR lpszBinaryPathName;

	GetCurrentDirectory(1024, strDir);
	strcat(strDir, "\\gammu-smsd.exe");
	strcat(strDir, " -c ");
	strcat(strDir, params->config_file);
	schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

	if (schSCManager == NULL)
		return false;

	lpszBinaryPathName = strDir;

	schService = CreateService(schSCManager, smsd_service_name, "Gammu SMSD Service",	// service name to display
				   SERVICE_ALL_ACCESS,	// desired access
				   SERVICE_WIN32_OWN_PROCESS,	// service type
				   SERVICE_AUTO_START,	// start type
				   SERVICE_ERROR_NORMAL,	// error control type
				   lpszBinaryPathName,	// service's binarygammu-smsd
				   NULL,	// no load ordering group
				   NULL,	// no tag identifier
				   NULL,	// no dependencies
				   NULL,	// LocalSystem account
				   NULL);	// no password

	if (schService == NULL)
		return false;

	CloseServiceHandle(schService);
	return true;
}

bool uninstall_smsd_service(void)
{
	HANDLE schSCManager;

	SC_HANDLE hService;

	schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

	if (schSCManager == NULL)
		return false;
	hService = OpenService(schSCManager, smsd_service_name, SERVICE_ALL_ACCESS);
	if (hService == NULL)
		return false;
	if (DeleteService(hService) == 0)
		return false;
	if (CloseServiceHandle(hService) == 0)
		return false;

	return true;
}

void start_smsd_service_dispatcher(void)
{
	SERVICE_TABLE_ENTRY DispatchTable[] = {
		{smsd_service_name, ServiceMain},
		{NULL, NULL}
	};

	StartServiceCtrlDispatcher(DispatchTable);
}

/* How should editor hadle tabs in this file? Add editor commands here.
 * vim: noexpandtab sw=8 ts=8 sts=8:
 */
