REM Copy source files to VM370
docker cp yata.c vm370:/opt/hercules/vm370/io
docker cp make.exec vm370:/opt/hercules/vm370/io
docker cp test/yata.txt vm370:/opt/hercules/vm370/io
docker cp cmsbuild.sh vm370:/opt/hercules/vm370/io
docker cp install.sh vm370:/opt/hercules/vm370/io
