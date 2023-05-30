#include <iOSAuthRuntime/authorize.h>



// 程序目录
std::string auth_service_application_file_dir() noexcept
{
	static std::string 	static_object_example;
	if(static_object_example.empty())
	{
		char		vDirectory[MAX_PATH] = { 0 };
		GetModuleFileNameA(GetModuleHandleW(nullptr), vDirectory, MAX_PATH);

		auto		vStrFind = strrchr(vDirectory, '\\');
		if(vStrFind)
		{
			*vStrFind = 0;
		}
		static_object_example = vDirectory;
	}
	return static_object_example;
}

// 同步接收
bool auth_service_sync_recv(SOCKET _Socket, HANDLE _Handle) noexcept
{
	char* package_recv_byte = nullptr;
	uint32_t	package_recv_size = 0;
	if(!auth_package_recv(_Socket, &package_recv_byte, &package_recv_size))
	{
		return false;
	}
	if(!auth_package_write(_Handle, package_recv_byte, package_recv_size))
	{
		auth_package_free(package_recv_byte);
		return false;
	}
	auth_package_free(package_recv_byte);
	return true;
}

// 同步发送
bool auth_service_sync_send(SOCKET _Socket, HANDLE _Handle) noexcept
{
	char* package_recv_byte = nullptr;
	uint32_t	package_recv_size = 0;
	if(!auth_package_read(_Handle, &package_recv_byte, &package_recv_size))
	{
		return false;
	}
	if(!auth_package_send(_Socket, package_recv_byte, package_recv_size))
	{
		auth_package_free(package_recv_byte);
		return false;
	}
	auth_package_free(package_recv_byte);
	return true;
}

// 线程
void auth_service_exec_thread(SOCKET _Socket) noexcept
{
	// 打开一个子进程
	auto		vRunPipe = auth_pipe_random_name();
	auto		vRunDirectory = auth_service_application_file_dir();
	auto		vRunExec = vRunDirectory + "\\iOSAuthHandle.exe";
	auto		vRunParam = std::string("\"") + vRunExec + std::string("\"") + std::string(" ") + std::string("--pipe-name=") + vRunPipe;
	auto 		vExecStatus = auth_process_exec(vRunExec, vRunDirectory, vRunParam);
	if(!vExecStatus)
	{
		auth_socket_close(_Socket);
		return;
	}

	// 连接命名管道
	auto		vSyncHandle = auth_pipe_create(vRunPipe.data());
	if (vSyncHandle == INVALID_HANDLE_VALUE)
	{
		auth_socket_close(_Socket);
		return;
	}

	// 开始同步数据
	do
	{
		// 第一次同步
		if(!auth_service_sync_recv(_Socket, vSyncHandle))
		{
			break;
		}
		if(!auth_service_sync_send(_Socket, vSyncHandle))
		{
			break;
		}

		// 第二次同步
		if(!auth_service_sync_recv(_Socket, vSyncHandle))
		{
			break;
		}
		if(!auth_service_sync_send(_Socket, vSyncHandle))
		{
			break;
		}
	} while(false);

	// 释放资源
	auth_pipe_release(vSyncHandle);
	auth_socket_close(_Socket);
}

// 执行
int auth_service_exec() noexcept
{
	auto 		vSocket = auth_socket_bind(27987);
	if(INVALID_SOCKET == vSocket)
	{
		return -1;
	}

	// 循环监听
	while(true)
	{
		struct sockaddr_in	vClientAddress {};
		socklen_t		vClientSize = sizeof(struct sockaddr_in);
		auto			vClient = accept(vSocket, (struct sockaddr*)&vClientAddress, &vClientSize);
		if(INVALID_SOCKET == vClient)
		{
			break;
		}
		auto			vClientIP = inet_ntoa(vClientAddress.sin_addr);
		std::printf(u8"[%s : %d] Client ip: %s\n", __FUNCTION__, __LINE__, vClientIP);
		std::thread(auth_service_exec_thread, vClient).detach();
	}
	auth_socket_close(vSocket);
	return 0;
}



// 入口
int main(int _Argc, char** _Argv)
{
	UNREFERENCED_PARAMETER(_Argc);
	UNREFERENCED_PARAMETER(_Argv);

	WSADATA		vWsaData;
	WSAStartup(MAKEWORD(2, 2), &vWsaData);
	OleInitialize(nullptr);

	auth_service_exec();

	OleUninitialize();
	WSACleanup();
	return 0;
}
