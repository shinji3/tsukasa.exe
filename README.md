# tsukasa.exe
S2MMSHのリメイク

Cで作ったS2MMSHのリメイクです。  
PUSH機能しか無いのでKagamin2が無ければ何も出来ません。  
ffmpegからデータをpipeで受け取りKagamin2にPUSHします。  

ffmpeg -f dshow -i video="動画デバイス":audio="音声デバイス" -c:v mpeg4 -b:v 1000k -c:a libmp3lame -b:a 128k -f asf_stream -map 0:a -map 0:v - | tsukasa.exe http://localhost:8080/
