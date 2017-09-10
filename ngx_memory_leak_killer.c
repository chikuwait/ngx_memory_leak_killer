#include <unistd.h>
#include <sys/resource.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

typedef struct ngx_http_memory_leack_killer_conf_t {
  ngx_flag_t enable;
  ngx_uint_t limit;
} ngx_http_memory_leak_killer_conf_t;

static ngx_int_t ngx_http_memory_leak_killer_init(ngx_conf_t *cf);
static void *ngx_http_memory_leak_killer_create_conf(ngx_conf_t *cf);
static char *ngx_http_memory_leak_killer_init_conf(ngx_conf_t *cf, void *conf);
static ngx_int_t ngx_http_memory_leak_killer_handler(ngx_http_request_t *r);

#if (NGX_ENABLE_MEM_LEAK_TEST)
static char *ngx_http_memory_leak_test(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static ngx_int_t ngx_http_memory_leak_test_handler(ngx_http_request_t *r);
#endif
static ngx_command_t ngx_http_memory_leak_killer_commands[] = {

    {ngx_string("memory_leak_killer"), NGX_HTTP_MAIN_CONF | NGX_CONF_FLAG, ngx_conf_set_flag_slot,
     NGX_HTTP_MAIN_CONF_OFFSET, offsetof(ngx_http_memory_leak_killer_conf_t, enable), NULL},

    {ngx_string("memory_leak_killer_limit"), NGX_HTTP_MAIN_CONF | NGX_CONF_TAKE1, ngx_conf_set_num_slot,
     NGX_HTTP_MAIN_CONF_OFFSET, offsetof(ngx_http_memory_leak_killer_conf_t, limit), NULL},

#if (NGX_ENABLE_MEM_LEAK_TEST)
    {ngx_string("memory_leak_test"),      /* directive */
     NGX_HTTP_LOC_CONF | NGX_CONF_NOARGS, /* location context and takes
                                             no arguments*/
     ngx_http_memory_leak_test,           /* configuration setup function */
     0,                                   /* No offset. Only one context is supported. */
     0,                                   /* No offset when storing the module configuration on struct. */
     NULL},
#endif
    ngx_null_command};

static ngx_http_module_t ngx_http_memory_leak_killer_module_ctx = {
    NULL,                             /* preconfiguration */
    ngx_http_memory_leak_killer_init, /* postconfiguration */

    ngx_http_memory_leak_killer_create_conf, /* create main configuration */
    ngx_http_memory_leak_killer_init_conf,   /* init main configuration */

    NULL, /* create server configuration */
    NULL, /* merge server configuration */

    NULL, /* create location configuration */
    NULL  /* merge location configuration */
};

ngx_module_t ngx_http_memory_leak_killer_module = {NGX_MODULE_V1,
                                                   &ngx_http_memory_leak_killer_module_ctx, /* module context */
                                                   ngx_http_memory_leak_killer_commands,    /* module directives */
                                                   NGX_HTTP_MODULE,                         /* module type */
                                                   NULL,                                    /* init master */
                                                   NULL,                                    /* init module */
                                                   NULL,                                    /* init process */
                                                   NULL,                                    /* init thread */
                                                   NULL,                                    /* exit thread */
                                                   NULL,                                    /* exit process */
                                                   NULL,                                    /* exit master */
                                                   NGX_MODULE_V1_PADDING};

static ngx_int_t ngx_http_memory_leak_killer_init(ngx_conf_t *cf)
{
  ngx_http_core_main_conf_t *cmcf;
  ngx_http_handler_pt *h;

  cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

  h = ngx_array_push(&cmcf->phases[NGX_HTTP_REWRITE_PHASE].handlers);
  if (h == NULL) {
    return NGX_ERROR;
  }
  *h = ngx_http_memory_leak_killer_handler;

  return NGX_OK;
}

static void *ngx_http_memory_leak_killer_create_conf(ngx_conf_t *cf)
{
  ngx_http_memory_leak_killer_conf_t *conf;

  conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_memory_leak_killer_conf_t));
  if (conf == NULL) {
    return NULL;
  }

  conf->enable = NGX_CONF_UNSET;
  conf->limit = NGX_CONF_UNSET_UINT;

  return conf;
}

static char *ngx_http_memory_leak_killer_init_conf(ngx_conf_t *cf, void *conf)
{
  ngx_http_memory_leak_killer_conf_t *blcf = conf;

  ngx_conf_init_value(blcf->enable, 0);
  ngx_conf_init_uint_value(blcf->limit, 0);

  return NGX_CONF_OK;
}

static ngx_int_t ngx_http_memory_leak_killer_handler(ngx_http_request_t *r)
{
  static ngx_uint_t ngx_http_memory_leak_killer_limit = 0;
  ngx_pid_t ngx_http_memory_leak_killer_target_pid = getpid();

  struct rusage usage;
  ngx_http_memory_leak_killer_conf_t *blcf;
  blcf = ngx_http_get_module_main_conf(r, ngx_http_memory_leak_killer_module);

  if (!blcf->enable) {
    return NGX_DECLINED;
  }

  if (ngx_http_memory_leak_killer_limit == 0) {
    ngx_http_memory_leak_killer_limit = blcf->limit;
  }

  if (getrusage(RUSAGE_SELF, &usage) != 0) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "getrusage failed");
  }

  ngx_log_error(NGX_LOG_INFO, r->connection->log, 0, "pid %d, limit %d,usage maxrss %d",
                ngx_http_memory_leak_killer_target_pid, ngx_http_memory_leak_killer_limit, usage.ru_maxrss);
  if (ngx_http_memory_leak_killer_limit < (uint)usage.ru_maxrss) {
    if (ngx_http_memory_leak_killer_target_pid != 0) {
      if (kill(ngx_http_memory_leak_killer_target_pid, SIGQUIT) == -1) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "kill SIGQUIT failed");
      }
    }
  }

  return NGX_DECLINED;
}

#if (NGX_ENABLE_MEM_LEAK_TEST)
static ngx_int_t ngx_http_memory_leak_test_handler(ngx_http_request_t *r)
{
  char *test = malloc(2000000);
  memset(test, 'A', 2000000);

  static u_char ngx_mem_leak_message[] = "leak memory\n";
  ngx_log_error(NGX_LOG_INFO, r->connection->log, 0, "called test_handler\n");
  ngx_buf_t *b;
  ngx_chain_t out;

  /* Set the Content-Type header. */
  r->headers_out.content_type.len = sizeof("text/plain") - 1;
  r->headers_out.content_type.data = (u_char *)"text/plain";

  /* Allocate a new buffer for sending out the reply. */
  b = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));

  /* Insertion in the buffer chain. */
  out.buf = b;
  out.next = NULL; /* just one buffer */

  b->pos = ngx_mem_leak_message;                                 /* first position in memory of the data */
  b->last = ngx_mem_leak_message + sizeof(ngx_mem_leak_message); /* last position in memory of the data */
  b->memory = 1;                                                 /* content is in read-only memory */
  b->last_buf = 1;                                               /* there will be no more buffers in the request */

  /* Sending the headers for the reply. */
  r->headers_out.status = NGX_HTTP_OK; /* 200 status code */
  /* Get the content length of the body. */
  r->headers_out.content_length_n = sizeof(ngx_mem_leak_message);
  ;
  ngx_http_send_header(r); /* Send the headers */

  /* Send the body, and return the status code of the output filter chain. */
  return ngx_http_output_filter(r, &out);
}

static char *ngx_http_memory_leak_test(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
  ngx_http_core_loc_conf_t *clcf; /* pointer to core location configuration */

  /* Install the hello world handler. */
  clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
  clcf->handler = ngx_http_memory_leak_test_handler;

  return NGX_CONF_OK;
}
#endif
