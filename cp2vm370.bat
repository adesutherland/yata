REM Copy, build and install source files to VM370
docker cp yata.c vm370:/opt/hercules/vm370/io
docker cp make.exec vm370:/opt/hercules/vm370/io
docker cp cmsbuild.sh vm370:/opt/hercules/vm370/io
docker cp cmsinstall.sh vm370:/opt/hercules/vm370/io

docker exec vm370 bash -c "mkdir /opt/hercules/vm370/io/test"
docker cp test/yata.txt vm370:/opt/hercules/vm370/io/test

docker exec vm370 bash -c "cd /opt/hercules/vm370/io && ./cmsbuild.sh"
docker exec vm370 bash -c "cd /opt/hercules/vm370/io && ./cmsinstall.sh"