NGINX_SOURCE_URL = http://nginx.org/download/nginx-1.13.5.tar.gz

all:test

nginx:
	-test ! -f build/nginx-1.13.5.tar.gz &&\
	wget $(NGINX_SOURCE_URL) -P build/ &&\
	cd build &&\
	tar zxvf nginx-1.13.5.tar.gz &&\
	cd nginx-1.13.5 &&\
	./configure --prefix=work --add-module=../../../ngx_memory_leak_killer --with-cc-opt=-DNGX_ENABLE_MEM_LEAK_TEST &&\
	make &&\
	make install &&\
	cp ../../testconf/nginx.conf work/conf/nginx.conf

test:nginx
	cd build/nginx-1.13.5 && ./work/sbin/nginx
	curl localhost:8080/memory
	curl localhost:8080/memory
	curl localhost:8080/memory
	grep "gracefully shutting down" build/nginx-1.13.5/work/logs/error.log

test-clean:
	-rm -f build/nginx-1.13.5/work/logs/error.log
	killall nginx

clean:
	rm -f build/nginx-1.13.5.tar.gz
	rm -rf build/nginx-1.13.5
