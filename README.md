[![Build Status](https://travis-ci.org/chikuwait/ngx_memory_leak_killer.svg?branch=master)](https://travis-ci.org/chikuwait/ngx_memory_leak_killer)
# ngx_memory_leak_killer
ngx_memory_leak_killer can graceful restart a working process that exceeds the upper limit of memory usage. Thanks to [nginx-hello-world-module](https://github.com/perusio/nginx-hello-world-module) and [ngx_bumpylife](https://github.com/cubicdaiya/ngx_bumpylife).

## Configuration

- Enable "memory_leak_killer"
- Set the upper limit of memory usage in kilobytes

```nginx
http {
memory_leak_killer on;
memory_leak_killer_limit 5000; #5MB
...
}
```

## Install

- use nginx configure option

```
./configure --add-module=/path/to/ngx_memory_leak_killer

```
## Example
```
$ ./work/sbin/nginx

$ ps auwxf |grep nginx
ubuntu   23505  0.0  0.0  16464   384 ?        Ss   08:46   0:00 nginx: master process ./work/sbin/nginx
ubuntu   23506  0.0  0.0  16908  1936 ?        S    08:46   0:00  \_ nginx: worker process

$ curl localhost:8080/memory
leak memory(+2MB)

$ ps auwxf |grep nginx
ubuntu   23505  0.0  0.0  16464   384 ?        Ss   08:46   0:00 nginx: master process ./work/sbin/nginx
ubuntu   23506  0.0  0.0  18864  4744 ?        S    08:46   0:00  \_ nginx: worker process

$ curl localhost:8080/memory
leak memory(+2MB)

$ ps auwxf |grep nginx
ubuntu   23505  0.0  0.0  16464   384 ?        Ss   08:46   0:00 nginx: master process ./work/sbin/nginx
ubuntu   23506  0.0  0.0  20820  6640 ?        S    08:46   0:00  \_ nginx: worker process

$ curl localhost:8080/memory
leak memory(+2MB)

$ ps auwxf |grep nginx
ubuntu   23505  0.0  0.0  16464   384 ?        Ss   08:46   0:00 nginx: master process ./work/sbin/nginx
ubuntu   23519  0.0  0.0  16908  1936 ?        S    08:46   0:00  \_ nginx: worker process





```

## LICENSE

See [LICENSE](https://git.pepabo.com/matsumotory/ngx_memory_leak_killer/blob/master/LICENSE).
