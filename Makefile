bin=server
cgi=test_cgi
cc=g++
LD_FLAGS=-std=c++11 -lpthread
curr=$(shell pwd)
src=server.cpp single.cpp

.PHONY:ALL
ALL:$(bin) $(cgi)

$(bin):$(src)
	$(cc) -o $@ $^ $(LD_FLAGS)

$(cgi):cgi/test.cpp
	$(cc) -o $@ $^ $(LD_FLAGS)

.PHONY:clean
clean:
	rm -f $(bin) $(cgi)
	rm -rf output

.PHONY:output
output:
	mkdir -p output
	cp $(bin) output
	cp -rf wwwroot output
	cp $(cgi) output/wwwroot