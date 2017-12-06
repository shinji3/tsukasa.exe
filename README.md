# tsukasa.exe
S2MMSHのリメイク

C++で作ったS2MMSHのリメイクです。  
PUSH機能しか無いのでKagamin2が無ければ何も出来ません。  
ffmpegからデータをpipeで受け取りKagamin2にPUSHします。  

VisualStudioでのビルド方法  

cmakeをダウンロードしてインストールした後に  
VisualStudioのコマンドプロンプトを立ち上げて下のコマンドを実行するとビルド出来ます  
x86のコマンドプロンプトでビルドするとx86のバイナリが出来ます  
x64のコマンドプロンプトでビルドするとx64のバイナリが出来ます  

pocoのビルド  
cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release -DPOCO_STATIC=ON -DPOCO_MT=ON .  
nmake  

tsukasaのビルドの前にtsukasaのフォルダにpocoのincludeファイルとlibファイルをコピーして下さい  

tsukasaのビルドはtsukasa.slnかcmakeで出来ます  
cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release .  
nmake  
