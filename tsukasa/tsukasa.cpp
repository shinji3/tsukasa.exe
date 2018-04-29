// tsukasa.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"


int main(int argc, char *argv[])
{

#ifdef _WIN32
    _setmode(_fileno(stdin), _O_BINARY);
#endif

    int data_pos;
    int audio_stream_number = 0;

    /* Object ID */
    unsigned char asf_stream_bitrate_properties_object[16] = { 0xce, 0x75, 0xf8, 0x7b, 0x8d, 0x46, 0xd1, 0x11, 0x8d, 0x82, 0x00, 0x60, 0x97, 0xc9, 0xa2, 0xb2 };
    unsigned char asf_file_properties_object[16] = { 0xa1, 0xdc, 0xab, 0x8c, 0x47, 0xa9, 0xcf, 0x11, 0x8e, 0xe4, 0x00, 0xc0, 0xc, 0x20, 0x53, 0x65 };
    unsigned char asf_stream_properties_object[16] = { 0x91, 0x7, 0xdc, 0xb7, 0xb7, 0xa9, 0xcf, 0x11, 0x8e, 0xe6, 0x00, 0xc0, 0xc, 0x20, 0x53, 0x65 };

    unsigned char asf_audio_media[16] = { 0x40, 0x9e, 0x69, 0xf8, 0x4d, 0x5b, 0xcf, 0x11, 0xa8, 0xfd, 0x00, 0x80, 0x5f, 0x5c, 0x44, 0x2b };
    unsigned char asf_no_error_correction[16] = { 0x00, 0x57, 0xfb, 0x20, 0x55, 0x5b, 0xcf, 0x11, 0xa8, 0xfd, 0x00, 0x80, 0x5f, 0x5c, 0x44, 0x2b };

    /* 8bit */
    unsigned char framing_header[12];
    unsigned char *data;
    char bitrate_records[127] = "";

    /* 16bit */
    unsigned short data_size;
    unsigned short old_data_size;

    /* 32bit */
    unsigned long number_of_header_objects = 0;
    unsigned long maximum_bitrate = 0;
    unsigned long average_number_of_bytes_per_second = 0;

    /* 64bit */
    unsigned long long header_object_size = 0;
    unsigned long long object_size;

    if (argc == 1)
    {
        exit(EXIT_FAILURE);
    }

    /* framing_headerをSTDINから読み込む */
    std::cin.read((char *)framing_header, 12);
    if (std::cin.gcount() < 12)
    {
        exit(EXIT_FAILURE);
    }

    /* PacketIDのエラーチェック */
    if (framing_header[0] != '$' || framing_header[1] != 'H')
    {
        exit(EXIT_FAILURE);
    }

    /* MMS Data Packetのエラーチェック */

    /* LocationIdとIncarnationが0以外はエラー */
    for (int i = 0; i < 5; i++)
    {
        if (framing_header[4 + i] != 0)
        {
            exit(EXIT_FAILURE);
        }
    }

    /* AFFlagsが12以外はエラー */
    if (framing_header[9] != 12)
    {
        exit(EXIT_FAILURE);
    }

    /* PacketSizeがPacketLengthとサイズが違うとエラー */
    for (int i = 0; i < 2; i++) {
        if (framing_header[10 + i] != framing_header[2 + i])
        {
            exit(EXIT_FAILURE);
        }
    }

    /* data_sizeを取得、8バイトのMMS Data Packetをdata_sizeから引く */
    data_size = 0;
    for (int i = 0; i < 2; i++)
    {
        data_size += (unsigned short)framing_header[2 + i] << 8 * i;
    }
    data_size -= 8;

    /* dataにdata_size+792のメモリを確保 */
    /* 792はasf_stream_bitrate_properties_objectの最大サイズ(788)とFraming Headerのサイズ(4)の合計 */
    /* 788 = 26 + 6 * 127 */
    data = new unsigned char[data_size + 792];
    std::memcpy(data, framing_header, 2);

    /* STDINからデータを読み込む */
    std::cin.read((char *)data + 4, data_size);
    if (std::cin.gcount() < data_size)
    {
        exit(EXIT_FAILURE);
    }

    /* header_object_sizeを取得 */
    for (int i = 0; i < 8; i++)
    {
        header_object_size += (unsigned long long)data[20 + i] << 8 * i;
    }

    /* number_of_header_objectsを取得 */
    for (int i = 0; i < 4; i++)
    {
        number_of_header_objects += (unsigned long)data[28 + i] << 8 * i;
    }

    /* data_posをasf_header_objectの終端に設定 */
    data_pos = 34;

    for (unsigned int j = 0; j < number_of_header_objects; j++)
    {
        /* object_sizeを取得 */
        object_size = 0;
        for (int i = 0; i < 8; i++)
        {
            object_size += (unsigned long long)data[data_pos + 16 + i] << 8 * i;
        }

        /* asf_file_properties_objectかどうか */
        if (std::memcmp(data + data_pos, asf_file_properties_object, 16) == 0)
        {
            /* file_idの末端を0xFFにする */
            data[data_pos + 39] = 0xFF;

            /* maximum_bitrateを取得 */
            for (int i = 0; i < 4; i++)
            {
                maximum_bitrate += (unsigned long)data[data_pos + 100 + i] << 8 * i;
            }

            /* maximum_bitrateが0xFFFFFFFFの場合はmaximum_bitrateに128000を書き込む */
            if (maximum_bitrate == 0xFFFFFFFF)
            {
                maximum_bitrate = 128000;
                for (int i = 0; i < 4; i++)
                {
                    data[data_pos + 100 + i] = (maximum_bitrate >> 8 * i) & 0xFF;
                }
            }
        }

        /* asf_stream_properties_objectかどうか */
        if (std::memcmp(data + data_pos, asf_stream_properties_object, 16) == 0)
        {
            /* stream_typeがasf_audio_mediaかどうか */
            if (std::memcmp(data + data_pos + 24, asf_audio_media, 16) == 0)
            {
                /* error_correction_typeをasf_no_error_correctionに設定 */
                std::memcpy(data + data_pos + 40, asf_no_error_correction, 16);

                /* average_number_of_bytes_per_secondを取得 */
                average_number_of_bytes_per_second = 0;
                for (int i = 0; i < 4; i++)
                {
                    average_number_of_bytes_per_second += (unsigned long)data[data_pos + 86 + i] << 8 * i;
                }

                /* average_number_of_bytes_per_secondが0の場合はaverage_number_of_bytes_per_secondに16000を書き込む※MP3のVBR対応のために必要 */
                if (average_number_of_bytes_per_second == 0)
                {
                    average_number_of_bytes_per_second = 16000;
                    for (int i = 0; i < 4; i++)
                    {
                        data[data_pos + 86 + i] = (average_number_of_bytes_per_second >> 8 * i) & 0xFF;
                    }
                }

                audio_stream_number = data[data_pos + 72];
            }

            /* bitrate_recordsにstream_numberを追加 */
            bitrate_records[std::strlen(bitrate_records)] = data[data_pos + 72];
        }

        /* data_posにobject_sizeを加算 */
        data_pos += (int)object_size;
    }

    /* asf_data_object */
    /* file_idの末端を0xFFにする※これをしないとKagaminにPush出来ない */
    data[data_pos + 39] = 0xFF;

    /* asf_stream_bitrate_properties_objectを作成 */
    object_size = 26 + 6 * std::strlen(bitrate_records);

    /* asf_stream_bitrate_properties_objectの領域を確保 */
    std::memmove(data + data_pos + object_size, data + data_pos, data_size + 4 - data_pos);

    /* object_idを書き込む */
    for (int i = 0; i < 16; i++)
    {
        data[data_pos + i] = asf_stream_bitrate_properties_object[i];
    }

    /* object_sizeを書き込む */
    for (int i = 0; i < 8; i++)
    {
        data[data_pos + 16 + i] = (object_size >> 8 * i) & 0xFF;
    }

    /* bitrate_records_countを書き込む */
    for (int i = 0; i < 2; i++)
    {
        data[data_pos + 24 + i] = (std::strlen(bitrate_records) >> 8 * i) & 0xFF;
    }

    /* bitrate_recordsを書き込む */
    for (unsigned int j = 0; j < strlen(bitrate_records); j++)
    {
        /* stream_numberを書き込む */
        data[data_pos + 26 + j * 6] = bitrate_records[j];
        /* reservedを書き込む */
        data[data_pos + 27 + j * 6] = 0;
        /* average_bitrateを書き込む */
        if (bitrate_records[j] == audio_stream_number)
        {
            for (int i = 0; i < 4; i++)
            {
                data[data_pos + 28 + i + j * 6] = ((average_number_of_bytes_per_second * 8) >> 8 * i) & 0xFF;
            }
        }
        else
        {
            for (int i = 0; i < 4; i++)
            {
                data[data_pos + 28 + i + j * 6] = ((maximum_bitrate - average_number_of_bytes_per_second * 8) >> 8 * i) & 0xFF;
            }
        }
    }

    /* 各種サイズを更新  */
    number_of_header_objects += 1;
    header_object_size += object_size;
    data_size += (unsigned short)object_size;

    /* 新しいnumber_of_header_objectsを書き込む */
    for (int i = 0; i < 4; i++)
    {
        data[28 + i] = (number_of_header_objects >> 8 * i) & 0xFF;
    }

    /* 新しいheader_object_sizeを書き込む */
    for (int i = 0; i < 8; i++)
    {
        data[20 + i] = (header_object_size >> 8 * i) & 0xFF;
    }

    /* 新しいdata_sizeを書き込む */
    for (int i = 0; i < 2; i++)
    {
        data[2 + i] = (data_size >> 8 * i) & 0xFF;
    }

    Poco::URI uri(argv[1]);

    Poco::Net::HTTPClientSession client(uri.getHost(), uri.getPort());

    std::string path(uri.getPathAndQuery());
    if (path.empty()) path = "/";

    Poco::Net::HTTPRequest request(Poco::Net::HTTPRequest::HTTP_POST, path, Poco::Net::HTTPMessage::HTTP_1_1);
    Poco::Net::HTTPResponse response;

    request.setContentType("application/x-wms-pushsetup");
    request.set("Cookie", "push-id=0");
    request.set("User-Agent", "WMEncoder/12.0.10011.16384");

    client.sendRequest(request);
    client.receiveResponse(response);

    std::string cookie = response.get("Set-Cookie");

    request.setContentType("application/x-wms-pushstart");
    request.set("Cookie", cookie);

    /* pushstartを送信 */
    std::ostream &os = client.sendRequest(request);
    os.write((char *)data, data_size + 4);
    os.flush();

    /* old_data_sizeにdata_sizeの値を入れる */
    old_data_size = data_size;

    /* $D */

    while (true)
    {
        /* framing_headerをSTDINから読み込む */
        std::cin.read((char *)framing_header, 12);
        if (std::cin.gcount() < 12)
        {
            exit(EXIT_FAILURE);
        }

        /* PacketIDのエラーチェック */
        if (framing_header[0] != '$' || (framing_header[1] != 'D' && framing_header[1] != 'E'))
        {
            exit(EXIT_FAILURE);
        }

        /* MMS Data Packetのエラーチェック */

        /* IncarnationとAFFlagsが0以外はエラー */
        for (int i = 0; i < 2; i++)
        {
            if (framing_header[8 + i] != 0)
            {
                exit(EXIT_FAILURE);
            }
        }

        /* PacketSizeがPacketLengthとサイズが違うとエラー */
        for (int i = 0; i < 2; i++)
        {
            if (framing_header[10 + i] != framing_header[2 + i])
            {
                exit(EXIT_FAILURE);
            }
        }

        /* data_sizeを取得、8バイトのMMS Data Packetをdata_sizeから引く */
        data_size = 0;
        for (int i = 0; i < 2; i++)
        {
            data_size += (unsigned short)framing_header[2 + i] << 8 * i;
        }
        data_size -= 8;

        /* data_sizeとold_data_sizeが違う場合は新しくメモリを確保 */
        if (data_size != old_data_size)
        {
            /* dataのメモリを開放 */
            delete[] data;

            /* dataにdata_size+4(Framing Headerのサイズ)のメモリを確保 */
            data = new unsigned char[data_size + 4];
        }

        /* framing_headerをdataにコピー */
        std::memcpy(data, framing_header, 2);

        /* 新しいdata_sizeを書き込む */
        for (int i = 0; i < 2; i++)
        {
            data[2 + i] = (data_size >> 8 * i) & 0xFF;
        }

        /* STDINからデータを読み込む */
        std::cin.read((char *)data + 4, data_size);
        if (std::cin.gcount() < data_size)
        {
            exit(EXIT_FAILURE);
        }

        /* dataを送信 */
        os.write((char *)data, data_size + 4);
        os.flush();

        /* old_data_sizeにdata_sizeの値を入れる */
        old_data_size = data_size;

        /* PacketIDが$Eだったら終了 */
        if (framing_header[1] == 'E')
        {
            break;
        }
    }

    return EXIT_SUCCESS;
}
