#ifdef __APPLE__
#else
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <signal.h>
#include <unistd.h>
#include <assert.h>

#include <Python.h>
#include <curl/curl.h>

#include "../include/queue.h"

// TODO: Limit time spent on each website

#define QUEUE_SIZE 10000000
#define NUM_THREADS 6
#define URL_LIST_MAX 100000
#define RESP_LEN 10000 /* Length of the HTML response */
#define URL_TOKEN "http"
#define HREF_TOKEN "href="
#define RELATIVE_URL_TOKEN "\"/"
#define HTTP_REQUEST "GET / HTTP/1.1\nHost: %s\n\r\n\r\n"

struct MemoryStruct {
        char *memory;
        size_t size;
};

/* Global variables */
volatile sig_atomic_t running = 1;
queue_t q;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

void stop_threads()
{
        running = 0;
}


/*
 * SIGINT signal handler cleans up threads gracefully
 */
void handle_sigint(int signal)
{
        if (signal == SIGINT)
                stop_threads();
}

        static size_t
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
        size_t realsize = size * nmemb;
        struct MemoryStruct *mem = (struct MemoryStruct *)userp;

        mem->memory = realloc(mem->memory, mem->size + realsize + 1);
        if (mem->memory == NULL) {
                fprintf(stderr, "Not enough memory (realloc returned NULL)\n");
                return 0;
        }

        memcpy(&(mem->memory[mem->size]), contents, realsize);
        mem->size += realsize;
        mem->memory[mem->size] = 0;

        return realsize;
}

/*
 * Utility function that removes non printable characters from a string
 */
char* trim(char *str)
{
        char *end;

        // Trim leading space
        while (*str && (!isprint(*str) || isspace(*str))) ++str;

        if (*str == 0)
                return str;

        // Trim trailing space
        end = str + strlen(str) - 1;

        while (end > str && (!isprint(*str) || isspace(*str))) --end;

        *(end + 1) = 0;
        return str;
}

/*
 * Returns argv to be used with a python script
 */
char **argv_init(char *script, char *url, char *data)
{
        if (!data) {
                char **argv = malloc(2 * sizeof(char*));
                argv[0] = script;
                argv[1] = url;
                return argv;
        }
        char **argv = malloc(3 * sizeof(char*));
        argv[0] = script;
        argv[1] = url;
        argv[2] = data;
        return argv;
}

/*
 * Destroy argv variable
 */
void argv_destroy(char **argv)
{
        free(argv);
}

/*
 * Check if URL exists in MySQL database
 */
int check_url(char *url)
{
        char **argv = argv_init("check.py", url, NULL);
        PySys_SetArgvEx(2, argv, 0);
        FILE *fptr = fopen("src/check.py", "r");
        assert(fptr);
        PyRun_SimpleFile(fptr, "check.py");
        argv_destroy(argv);
        fclose(fptr);
        fptr = fopen(".check.txt", "r");
        assert(fptr);
        char byte = '!';
        fread(&byte, 1, 1, fptr);
        fclose(fptr);

        if (byte == '1')
                return 1;
        else if (byte == '0')
                return 0;

        fprintf(stderr, "Read error: .check.txt\n");
        exit(1);
}

/*
 * Insert URL into the table using insert.py
 */
void insert_url(char *url, char *resp)
{
        char **argv = argv_init("insert.py", url, resp);
        PySys_SetArgvEx(3, argv, 0);
        FILE *fptr = fopen("src/insert.py", "r");
        assert(fptr);
        PyRun_SimpleFile(fptr, "insert.py");
        argv_destroy(argv);
        fclose(fptr);
}

/*
 * Given a url, return the url if it's root page
 * Helper function for when we create full url's from relative links
 */
char *get_root_url(char *url)
{
        pthread_mutex_lock(&m);
        char *p;

        /* Move past http:// or https:// token */
        if (strstr(url, URL_TOKEN)) {
                /* Shift p past initial http token */
                p = url + strlen(URL_TOKEN);

                /* Now p should point to either :// or s:// */
                if (*p == 's' || *p == 'S')
                        p += 4;
                else if (*p == ':')
                        p += 3;
        } else {
                p = url;
        }

        if ((p = strstr(p, "/")) != NULL)
                *p = '\0';
        pthread_mutex_unlock(&m);
        return url;
}

/*
 * Given an html response, and the origin page, parse any links it contains.
 * Returns a list of url's which can then be used to perform more requests.
 *
 * We pass the origin so that we can parse relative url's
 */
void parse_resp(char *origin, char *resp)
{
        if (!resp)
                return;

        /* Get the start of the html code */
        char *tok = strstr(resp, "<");


        if (!tok)
                return;

        /*
         * Iterate over every href attribute
         */
        while (running && (tok = strcasestr(tok, HREF_TOKEN))) {
                char *p; // Generic pointer
                tok += 6; /* Account for the first quote after href */
                /*
                 * Check for closing quotes (single or double)
                 * in the original string we are working on
                 */
                char *end = strstr(tok, "\"");
                if (!end)
                        end = strstr(tok, "\'");
                if (!end)
                        continue;

                assert(end);
                *end = 0; /* Null terminate at end before we look for tokens */

                int relative = 0; /* Are we working with a relative url? */
                if (*tok == '/')
                        relative = 1;

                /* Not a url */
                if (*tok == '#' || *tok == '.')
                        continue;
                /*
                 * If we found the start of a URL, copy the first 20
                 * characters after to a new string
                 */
                char *url = malloc(100);
                strncpy(url, tok, 99);
                url[99] = '\0';


                /* Create a full url from a relative url, if applicable */
                if (relative) {
                        /* Remove any slashes from end of the origin url */
                        if (origin[strlen(origin) - 1] == '/')
                                origin[strlen(origin) - 1] = '\0';
                        char *full_url = malloc(100);
                        if (!full_url) {
                                perror("malloc");
                                free(full_url);
                                free(url);
                                continue;
                        }

                        strncpy(full_url, origin, 99);
                        full_url[99] = '\0';

                        int bytes_left = 98 - strlen(full_url);
                        full_url = strncat(full_url, url, bytes_left);
                        free(url);
                        url = full_url;
                }

                /* Edge case for certain links on dmoz.org */
                if ((p = strstr(url, "'")))
                        *p = '\0';

                /* Add url to queue */
                queue_push(&q, url);

                tok = end + 1;
        }
};

/*
 * Perform an http GET request for the url specified.
 * Returns the response as a string that must be free'd
 */
char *http_request(char *url)
{
        /* Check the URL against the MySQL database */
        int exists = 0;
        pthread_mutex_lock(&m);
        if (check_url(url))
                exists = 1;
        pthread_mutex_unlock(&m);

        if (exists)
                return NULL;

        //char *resp = NULL;
        CURL *curl_handle;
        CURLcode res;

        struct MemoryStruct chunk;

        chunk.memory = calloc(1, 1);  /* will be grown as needed by the realloc above */
        chunk.size = 0;    /* no data at this point */

        /* init the curl session */
        curl_handle = curl_easy_init();

        assert(curl_handle);

        /* specify URL to get */
        curl_easy_setopt(curl_handle, CURLOPT_URL, url);

        /* send all data to this function  */
        res = curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

        if (res != CURLE_OK) {
                fprintf(stderr, "curl_easy_setopt() failed: %s\n",
                                curl_easy_strerror(res));
        }
        /* we pass our 'chunk' struct to the callback function */
        res = curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);

        if (res != CURLE_OK) {
                fprintf(stderr, "curl_easy_setopt() failed: %s\n",
                                curl_easy_strerror(res));
        }

        /*
         * some servers don't like requests that are made without a user-agent
         * field, so we provide one
         */
        res = curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
        if (res != CURLE_OK) {
                fprintf(stderr, "curl_easy_setopt() failed: %s\n",
                                curl_easy_strerror(res));
        }

        /* get it! */
        res = curl_easy_perform(curl_handle);

        /* check for errors */

        if (res != CURLE_OK) {
                fprintf(stderr, "%s: curl_easy_perform() failed: %s\n",
                                url, curl_easy_strerror(res));
                free(chunk.memory);
                curl_easy_cleanup(curl_handle);
                return NULL;
        } else if (chunk.size <= 0) {
                free(chunk.memory);
                curl_easy_cleanup(curl_handle);
                return NULL;
        }
        /* Print success message */
        write(STDOUT_FILENO, ".", 2);

        /* cleanup curl stuff */

        curl_easy_cleanup(curl_handle);

        // free(chunk.memory);

        return chunk.memory;
}


/*
 * This function will be called by our threads.
 * It finds website data from a hostname on the queue
 * Then it parses the data and adds any links it finds back to the queue
 */
void *work(void *p)
{
        char last_origin[100] = "";
        while (running) {
                char *hostname = trim(queue_pull(&q));
                /* Make sure we aren't pulling NULL or an empty string */
                if (!hostname || strcmp(hostname, "") == 0)
                        continue;

                char origin[100];
                if (strcmp(last_origin, "") == 0) {
                        strncpy(last_origin, get_root_url(hostname), 99);
                        last_origin[99] = '\0';
                } else if (strstr(hostname, last_origin)) {
                        strncpy(origin, last_origin, 99);
                        origin[99] = '\0';
                } else {
                        strncpy(origin, get_root_url(hostname), 99);
                        origin[99] = '\0';
                        strncpy(last_origin, origin, 99);
                        last_origin[99] = '\0';

                }



                /* Get the web data */
                char *resp = http_request(hostname);

                if (resp) {
                        pthread_mutex_lock(&m);
                        insert_url(hostname, resp);
                        pthread_mutex_unlock(&m);
                        parse_resp(origin, resp);
                }
                /* Free memory */
                free(resp);
        }

        return p;
}

int main(int argc, char **argv)
{
        if (argc != 2) {
                fprintf(stderr, "usage: %s <start_url>\n", *argv);
                return 1;
        }

        /* Initialize everything */
        signal(SIGINT, handle_sigint);
        queue_init(&q, QUEUE_SIZE);
        queue_push(&q, argv[1]);

        /* Initialize Python */
        Py_SetProgramName(*argv);
        Py_Initialize();

        /* Initialize curl library */
        curl_global_init(CURL_GLOBAL_ALL);

        pthread_t workers[NUM_THREADS];
        int i;

        /* Create worker threads */
        for (i = 0; i < NUM_THREADS; ++i) {
                pthread_create(&workers[i], NULL, work, NULL);
                pthread_detach(workers[i]);
        }

        while (running);

        /* Clean up data structures */
        queue_destroy(&q);
        pthread_mutex_destroy(&m);

        /* Clean up curl library */
        curl_global_cleanup();

        /* Close python interpreter */
        Py_Finalize();

        return 0;
}
