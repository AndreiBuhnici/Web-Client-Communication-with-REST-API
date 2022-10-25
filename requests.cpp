#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <stdio.h>
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.hpp"
#include "requests.hpp"

char *compute_get_request(char *host, char *url, char *query_params,
                            char **cookies, int cookies_count, char *token, char* command)
{
    char *message = (char *)calloc(BUFLEN, sizeof(char));
    char *line = (char *)calloc(LINELEN, sizeof(char));

    // Step 1: write the method name, URL, request params (if any) and protocol type
    if (strncmp(command, "GET", 3) == 0){
        if (query_params != NULL) {
            sprintf(line, "GET %s?%s HTTP/1.1", url, query_params);
        } else {
            sprintf(line, "GET %s HTTP/1.1", url);
        }
    } else if (strncmp(command, "DEL", 3) == 0) {
        if (query_params != NULL) {
            sprintf(line, "DELETE %s?%s HTTP/1.1", url, query_params);
        } else {
            sprintf(line, "DELETE %s HTTP/1.1", url);
        }
    }

    compute_message(message, line);

    // Step 2: add the host
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    // Step 3 (optional): add headers and/or cookies, according to the protocol format
    if (cookies != NULL) {
        sprintf(line, "Cookie:");
        for (int i = 0; i < cookies_count; i++) {
            strcat(line, " ");
            strcat(line, cookies[i]);
            strcat(line, ";");
        }
        compute_message(message, line);
    }

    if (token != NULL) {
        sprintf(line, "Authorization: Bearer %s", token);
        compute_message(message, line);
    }

    // Step 4: add final new line
    compute_message(message, "\n");
    return message;
}

char *compute_post_request(char *host, char *url, char* content_type, char **body_data,
                            int body_data_fields_count, char **cookies, int cookies_count, char* token)
{
    char *message = (char *)calloc(BUFLEN, sizeof(char));
    char *line = (char *)calloc(LINELEN, sizeof(char));
    char *body_data_buffer = (char *)calloc(LINELEN, sizeof(char));

    // Step 1: write the method name, URL and protocol type
    sprintf(line, "POST %s HTTP/1.1", url);
    compute_message(message, line);
    
    // Step 2: add the host
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    /* Step 3: add necessary headers (Content-Type and Content-Length are mandatory)
            in order to write Content-Length you must first compute the message size
    */

    for (int i = 0; i < body_data_fields_count; i++) {
        if (i < body_data_fields_count - 1) {
            strcat(body_data_buffer, body_data[i]);
            strcat(body_data_buffer, "&");
        }
        else {
            strcat(body_data_buffer, body_data[i]);
        }
    }

    sprintf(line, "Content-Type: %s", content_type);
    compute_message(message, line);
    sprintf(line, "Content-Length: %ld", strlen(body_data_buffer));
    compute_message(message, line);
    // Step 4 (optional): add cookies
    if (cookies != NULL) {
        sprintf(line, "Cookie:");
        for (int i = 0; i < cookies_count; i++) {
            sprintf(line, " %s;", cookies[i]);
            compute_message(message, line);
        }
    }

    if (token != NULL) {
        sprintf(line, "Authorization: Bearer %s", token);
        compute_message(message, line);
    }

    // Step 5: add new line at end of header
    compute_message(message, "");

    // Step 6: add the actual payload data
    memset(line, 0, LINELEN);
    compute_message(message, body_data_buffer);

    free(line);
    return message;
}
