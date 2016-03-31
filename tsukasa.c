#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <io.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <ctype.h>
#include <wininet.h>

int main(int argc, char *argv[]) {
	int i, j;
	int data_pos;
	int result;

	/* Boolean */
	int asf_stream_bitrate_properties_object_add = 1;

	/* Object ID */
	unsigned char asf_stream_bitrate_properties_object[16] = {206, 117, 248, 123, 141, 70, 209, 17, 141, 130, 0, 96, 151, 201, 162, 178};
	unsigned char asf_file_properties_object[16] = {161, 220, 171, 140, 71, 169, 207, 17, 142, 228, 0, 192, 12, 32, 83, 101};
	unsigned char asf_stream_properties_object[16] = {145, 7, 220, 183, 183, 169, 207, 17, 142, 230, 0, 192, 12, 32, 83, 101};

	unsigned char asf_audio_media[16] = {64, 158, 105, 248, 77, 91, 207, 17, 168, 253, 0, 128, 95, 92, 68, 43};
	unsigned char asf_no_error_correction[16] = {0, 87, 251, 32, 85, 91, 207, 17, 168, 253, 0, 128, 95, 92, 68, 43};

	/* 8bit */
	unsigned char framing_header[12];
	unsigned char *data;
	char bitrate_records[127] = "";

	/* 16bit */
	unsigned short data_size;
	unsigned short old_data_size;
	
	/* 32bit */
	unsigned long number_of_header_objects = 0;
	unsigned long average_number_of_bytes_per_second;
	
	/* 64bit */
	unsigned long long header_object_size = 0;
	unsigned long long object_size;

	/* WinSock2の初期化 */
	WSADATA wsaData;
	SOCKET sock;
	struct sockaddr_in server;
	
	char pushsetup_tpl[] =
		"POST %s%s HTTP/1.1\r\n"
		"Content-Type: application/x-wms-pushsetup\r\n"
		"X-Accept-Authentication: Negotiate, NTLM, Digest\r\n"
		"User-Agent: WMEncoder/12.0.10011.16384\r\n"
		"Host: %s:%d\r\n"
		"Content-Length: 0\r\n"
		"Connection: Keep-Alive\r\n"
		"Cache-Control: no-cache\r\n"
		"Cookie: push-id=0\r\n"
		"\r\n";

	char pushstart_tpl[] =
		"POST %s%s HTTP/1.1\r\n"
		"Content-Type: application/x-wms-pushstart\r\n"
		"X-Accept-Authentication: Negotiate, NTLM, Digest\r\n"
		"User-Agent: WMEncoder/12.0.10011.16384\r\n"
		"Host: %s:%d\r\n"
		"Content-Length: 2147483647\r\n"
		"Connection: Keep-Alive\r\n"
		"Cache-Control: no-cache\r\n"
		"Cookie: push-id=%s\r\n"
		"\r\n";
	
	char *pushsetup;
	char *pushstart;
	
	char recv_buf[1];
	char recv_end[] = {'\r', '\n', '\r', '\n'};
	char push_id_begin[] = {'p', 'u', 's', 'h', '-', 'i', 'd', '='};
	char push_id_buf[11] = "";
	
	LPHOSTENT hostent;
	
	URL_COMPONENTS uc;
	
	char *scheme;
	char *host_name;
	char *user_name;
	char *password;
	char *url_path;
	char *extra_info;
	
	if (argc == 1) {
		exit(0);
	}
	
	/* urlのサイズを取得 +1は\0 */
	result = strlen(argv[1]) + 1;

	scheme     = (char*)malloc(sizeof(char) * result);
	host_name  = (char*)malloc(sizeof(char) * result);
	user_name  = (char*)malloc(sizeof(char) * result);
	password   = (char*)malloc(sizeof(char) * result);
	url_path   = (char*)malloc(sizeof(char) * result);
	extra_info = (char*)malloc(sizeof(char) * result);

	memset(scheme,     '\0', sizeof(char) * result);
	memset(host_name,  '\0', sizeof(char) * result);
	memset(user_name,  '\0', sizeof(char) * result);
	memset(password,   '\0', sizeof(char) * result);
	memset(url_path,   '\0', sizeof(char) * result);
	memset(extra_info, '\0', sizeof(char) * result);

	uc.dwStructSize  = sizeof(uc);
	
	uc.lpszScheme    = scheme;
	uc.lpszHostName  = host_name;
	uc.lpszUserName  = user_name;
	uc.lpszPassword  = password;
	uc.lpszUrlPath   = url_path;
	uc.lpszExtraInfo = extra_info;

	uc.dwSchemeLength    = sizeof(char) * result;
	uc.dwHostNameLength  = sizeof(char) * result;
	uc.dwUserNameLength  = sizeof(char) * result;
	uc.dwPasswordLength  = sizeof(char) * result;
	uc.dwUrlPathLength   = sizeof(char) * result;
	uc.dwExtraInfoLength = sizeof(char) * result;
	
	/* URLを解析 */
	InternetCrackUrl(argv[1], strlen(argv[1]), 0, &uc);
	
	/* URLがhttp以外の場合は終了 */
	if (uc.nScheme != INTERNET_SCHEME_HTTP) {
		exit(0);
	}
	
	/* url_pathが空の場合は/を挿入 */
	if(strlen(uc.lpszUrlPath) == 0) {
		uc.lpszUrlPath[0] = '/';
	}
	
	/* ソケットの初期化 */
	result = WSAStartup(MAKEWORD(2,0), &wsaData);
	if (result != NO_ERROR) {
		exit(0);
	}
	
	/* ソケットの作成 */
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) {
		WSACleanup();
		exit(0);
	}

	/* hostnameを取得 */
	hostent = gethostbyname(uc.lpszHostName);

	/* 接続先指定用構造体の準備 */
	server.sin_family = AF_INET;
	server.sin_port = htons(uc.nPort);
	server.sin_addr.S_un.S_addr = **(unsigned int **)(hostent->h_addr_list);

	/* サーバに接続 */
	result = connect(sock, (SOCKADDR*)&server, sizeof(server));
	if (result == SOCKET_ERROR) {
		closesocket(sock);
		WSACleanup();
		exit(0);
	}

	/* pushsetupのメモリを確保 */
	pushsetup = (char*)malloc(sizeof(char) * (strlen(pushsetup_tpl) + strlen(uc.lpszUrlPath) + strlen(uc.lpszExtraInfo) + strlen(uc.lpszHostName) + 6));

	/* pushsetupを生成 */
	sprintf(pushsetup, pushsetup_tpl, uc.lpszUrlPath, uc.lpszExtraInfo, uc.lpszHostName, uc.nPort);

	/* pushsetupを送信 */
	result = send(sock, pushsetup, strlen(pushsetup), 0);
	if (result == SOCKET_ERROR) {
		closesocket(sock);
		WSACleanup();
		exit(0);
	}
	
	/* pushsetupのメモリを開放 */
	free(pushsetup);
	pushsetup = NULL;
	
	i = 0;
	j = 0;
	
	do {
		/* HTTP Headerを受信 */
		result = recv(sock, recv_buf, 1, 0);
		
		/* \r\n\r\nの文字列まで読み込む */
		if (recv_buf[0] == recv_end[i]) {
			i++;
		/* \n\nの文字列だった場合の対策 */
		} else if (recv_buf[0] == '\n') {
			i += 2;
		/* 違う文字だった場合はiを初期値に戻す */
		} else {
			i = 0;
		}
		
		/* push-id=の文字列が見つかった時の処理 */
		if (j == sizeof(push_id_begin)) {
			/* 数字だったらpush-idの文字列に追加 */
			if (isdigit(recv_buf[0])) {
				push_id_buf[strlen(push_id_buf)] = recv_buf[0];
			/* 数字以外だったらpush-idの文字列の生成を終了 */
			} else {
				j = 0;
			}
		/* push-idが見つかった場合は処理しない */
		} else if (strlen(push_id_buf) == 0) {
			/* push-id=の文字を検索 */
			if (recv_buf[0] == push_id_begin[j]) {
				j++;
			/* 違う文字だった場合はjを初期値に戻す */
			} else {
				j = 0;
			}
		}
	/* \r\n\r\nまで読み込んだらループを終了 */
	} while (result > 0 && i != sizeof(recv_end));
	
	/* pushstartのメモリを確保 */
	pushstart = (char*)malloc(sizeof(char) * (strlen(pushstart_tpl) + strlen(uc.lpszUrlPath) + strlen(uc.lpszExtraInfo) + strlen(uc.lpszHostName) + strlen(push_id_buf) + 6));

	/* pushstartを生成 */
	sprintf(pushstart, pushstart_tpl, uc.lpszUrlPath, uc.lpszExtraInfo, uc.lpszHostName, uc.nPort, push_id_buf);
	
	free(scheme);
	free(host_name);
	free(user_name);
	free(password);
	free(url_path);
	free(extra_info);

	scheme = NULL;
	host_name = NULL;
	user_name = NULL;
	password = NULL;
	url_path = NULL;
	extra_info = NULL;
	
	/* pushstartを送信 */
	result = send(sock, pushstart, strlen(pushstart), 0);
	if (result == SOCKET_ERROR) {
		closesocket(sock);
		WSACleanup();
		exit(0);
	}

	/* pushstartのメモリを開放 */
	free(pushstart);
	pushstart = NULL;

	/* STDINをバイナリで読み込む */
	setmode(fileno(stdin), O_BINARY);

	/* $H */
	
	/* framing_headerをSTDINから読み込む */
	result = fread(framing_header, sizeof(unsigned char), 12, stdin);
	if (result < 12) {
		closesocket(sock);
		WSACleanup();
		exit(0);
	}

	/* PacketIDのエラーチェック */
	if (framing_header[0] != '$' || framing_header[1] != 'H') {
		closesocket(sock);
		WSACleanup();
		exit(0);
	}

	/* MMS Data Packetのエラーチェック */

	/* LocationIdとIncarnationが0以外はエラー */
	for (i=0;i<5;i++) {
		if (framing_header[4+i] != 0) {
			closesocket(sock);
			WSACleanup();
			exit(0);
		}
	}

	/* AFFlagsが12以外はエラー */
	if (framing_header[9] != 12) {
		closesocket(sock);
		WSACleanup();
		exit(0);
	}

	/* PacketSizeがPacketLengthとサイズが違うとエラー */
	for (i=0;i<2;i++) {
		if (framing_header[10+i] != framing_header[2+i]) {
			closesocket(sock);
			WSACleanup();
			exit(0);
		}
	}

	/* data_sizeを取得、8バイトのMMS Data Packetをdata_sizeから引く */
	data_size = 0;
	for (i=0;i<2;i++) {
		data_size += (unsigned short)framing_header[2+i] << 8 * i;
	}
	data_size -= 8;

	/* dataにdata_size+792のメモリを確保 */
	/* 792はasf_stream_bitrate_properties_objectの最大サイズ(788)とFraming Headerのサイズ(4)の合計 */
	/* 788 = 26 + 6 * 127 */
	data = (unsigned char*)malloc(sizeof(unsigned char) * (data_size + 792));
	
	/* framing_headerをdataにコピー */
	memcpy(data, framing_header, sizeof(unsigned char) * 2);

	/* STDINからデータを読み込む */
	result = fread(data+4, sizeof(unsigned char), data_size, stdin);
	if (result < data_size) {
		free(data);
		data = NULL;
		
		closesocket(sock);
		WSACleanup();
		exit(0);
	}

	/* header_object_sizeを取得 */
	for (i=0;i<8;i++) {
		header_object_size += (unsigned long long)data[20+i] << 8 * i;
	}

	/* number_of_header_objectsを取得 */
	for (i=0;i<4;i++) {
		number_of_header_objects += (unsigned long)data[28+i] << 8 * i;
	}

	/* data_posをasf_header_objectの終端に設定 */
	data_pos = 34;

	for (j=0;j<number_of_header_objects;j++) {
		/* object_sizeを取得 */
		object_size = 0;
		for (i=0;i<8;i++) {
			object_size += (unsigned long long)data[data_pos+16+i] << 8 * i;
		}

		/* asf_stream_bitrate_properties_objectかどうか */
		if (memcmp(data+data_pos, asf_stream_bitrate_properties_object, sizeof(unsigned char) * 16) == 0) {
			/* asf_stream_bitrate_properties_objectを作成しない */
			asf_stream_bitrate_properties_object_add = 0;
		}

		/* asf_file_properties_objectかどうか */
		if (memcmp(data+data_pos, asf_file_properties_object, sizeof(unsigned char) * 16) == 0) {
			/* file_idの末端を0xFFにする */
			data[data_pos+39] = 255;
		}

		/* asf_stream_properties_objectかどうか */
		if (memcmp(data+data_pos, asf_stream_properties_object, sizeof(unsigned char) * 16) == 0) {
			/* stream_typeがasf_audio_mediaかどうか */
			if (memcmp(data+data_pos+24, asf_audio_media, sizeof(unsigned char) * 16) == 0) {
				/* error_correction_typeをasf_no_error_correctionに設定 */
				memcpy(data+data_pos+40, asf_no_error_correction, sizeof(unsigned char) * 16);

				/* average_number_of_bytes_per_secondを取得 */
				average_number_of_bytes_per_second = 0;
				for (i=0;i<4;i++) {
					average_number_of_bytes_per_second += (unsigned long)data[data_pos+86+i] << 8 * i;
				}

				/* average_number_of_bytes_per_secondが0の場合はaverage_number_of_bytes_per_secondに16000を書き込む※MP3のVBR対応のために必要 */
				if (average_number_of_bytes_per_second == 0) {
					for (i=0;i<4;i++) {
						data[data_pos+86+i] = (16000 >> 8 * i) & 0xFF;
					}
				}
			}
			
			/* bitrate_recordsにstream_numberを追加 */
			bitrate_records[strlen(bitrate_records)] = data[data_pos+72];
		}

		/* data_posにobject_sizeを加算 */
		data_pos += object_size;
	}

	/* asf_data_object */
	/* file_idの末端を0xFFにする※これをしないとKagaminにPush出来ない */
	data[data_pos+39] = 255;

	/* asf_stream_bitrate_properties_objectが無い場合は作成 */
	if (asf_stream_bitrate_properties_object_add) {
		object_size = 26 + 6 * strlen(bitrate_records);
		
		/* asf_stream_bitrate_properties_objectの領域を確保 */
		memmove(data+data_pos+object_size, data+data_pos, data_size+4-data_pos);
		
		/* object_idを書き込む */
		for (i=0;i<16;i++) {
			data[data_pos+i] = asf_stream_bitrate_properties_object[i];
		}
		
		/* object_sizeを書き込む */
		for (i=0;i<8;i++) {
			data[data_pos+16+i] = (object_size >> 8 * i) & 0xFF;
		}
		
		/* bitrate_records_countを書き込む */
		for (i=0;i<2;i++) {
			data[data_pos+24+i] = (strlen(bitrate_records) >> 8 * i) & 0xFF;
		}
		
		/* bitrate_recordsを書き込む */
		for (j=0;j<strlen(bitrate_records);j++) {
			/* stream_numberを書き込む */
			data[data_pos+26+j*6] = bitrate_records[j];
			/* reservedを書き込む */
			data[data_pos+27+j*6] = 0;
			/* average_bitrateを書き込む */
			for (i=0;i<4;i++) {
				data[data_pos+28+i+j*6] = (1000 >> 8 * i) & 0xFF;
			}
		}
		
		/* 各種サイズを更新  */
		number_of_header_objects +=1;
		header_object_size += object_size;
		data_size += object_size;
		
		/* 新しいnumber_of_header_objectsを書き込む */
		for (i=0;i<4;i++) {
			data[28+i] = (number_of_header_objects >> 8 * i) & 0xFF;
		}
		
		/* 新しいheader_object_sizeを書き込む */
		for (i=0;i<8;i++) {
			data[20+i] = (header_object_size >> 8 * i) & 0xFF;
		}
	}

	/* 新しいdata_sizeを書き込む */
	for (i=0;i<2;i++) {
		data[2+i] = (data_size >> 8 * i) & 0xFF;
	}

	/* dataを送信 */
	result = send(sock, (char *)data, sizeof(char) * (data_size+4), 0);
	if (result == SOCKET_ERROR) {
		free(data);
		data = NULL;
		
		closesocket(sock);
		WSACleanup();
		exit(0);
	}

	/* PacketIDが$Eだったら終了 */
	old_data_size = data_size;

	/* $D */

	while (1) {
		/* framing_headerをSTDINから読み込む */
		result = fread(framing_header, sizeof(unsigned char), 12, stdin);
		if (result < 12) {
			free(data);
			data = NULL;
			
			closesocket(sock);
			WSACleanup();
			exit(0);
		}

		/* PacketIDのエラーチェック */
		if (framing_header[0] != '$' || (framing_header[1] != 'D' && framing_header[1] != 'E')) {
			free(data);
			data = NULL;
			
			closesocket(sock);
			WSACleanup();
			exit(0);
		}

		/* MMS Data Packetのエラーチェック */

		/* IncarnationとAFFlagsが0以外はエラー */
		for (i=0;i<2;i++) {
			if (framing_header[8+i] != 0) {
				free(data);
				data = NULL;
				
				closesocket(sock);
				WSACleanup();
				exit(0);
			}
		}

		/* PacketSizeがPacketLengthとサイズが違うとエラー */
		for (i=0;i<2;i++) {
			if (framing_header[10+i] != framing_header[2+i]) {
				free(data);
				data = NULL;
				
				closesocket(sock);
				WSACleanup();
				exit(0);
			}
		}

		/* data_sizeを取得、8バイトのMMS Data Packetをdata_sizeから引く */
		data_size = 0;
		for (i=0;i<2;i++) {
			data_size += (unsigned short)framing_header[2+i] << 8 * i;
		}
		data_size -= 8;

		/* data_sizeとold_data_sizeが違う場合は新しくメモリを確保 */
		if (data_size != old_data_size) {
			/* dataのメモリを開放 */
			free(data);
			data = NULL;
			
			/* dataにdata_size+4(Framing Headerのサイズ)のメモリを確保 */
			data = (unsigned char*)malloc(sizeof(unsigned char) * (data_size + 4));
		}

		/* framing_headerをdataにコピー */
		memcpy(data, framing_header, sizeof(unsigned char) * 2);

		/* 新しいdata_sizeを書き込む */
		for (i=0;i<2;i++) {
			data[2+i] = (data_size >> 8 * i) & 0xFF;
		}

		/* STDINからデータを読み込む */
		result = fread(data+4, sizeof(unsigned char), data_size, stdin);
		if (result < data_size) {
			free(data);
			data = NULL;
			
			closesocket(sock);
			WSACleanup();
			exit(0);
		}
		
		/* dataを送信 */
		result = send(sock, (char *)data, sizeof(char) * (data_size+4), 0);
		if (result == SOCKET_ERROR) {
			free(data);
			data = NULL;
			
			closesocket(sock);
			WSACleanup();
			exit(0);
		}
		
		/* old_data_sizeにdata_sizeの値を入れる */
		old_data_size = data_size;
		
		/* PacketIDが$Eだったら終了 */
		if (framing_header[1] == 'E') {
			break;
		}
	}

	/* dataのメモリを開放 */
	free(data);
	data = NULL;
	
	/* winsockの終了 */
	closesocket(sock);
	WSACleanup();

	return EXIT_SUCCESS;
}
