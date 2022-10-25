#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include <iostream>

#include "helpers.hpp"
#include "requests.hpp"
#include "json.hpp"

using namespace std;

int main(int argc, char *argv[]) {
    char *message = NULL; // message for server
    char *response = NULL; // respons from server
    int sockfd;
    char host[] = "34.241.4.235"; // our server
    int port = 8080; // server port
    char buff[100]; // read from stdin buffer
    char cookie[200], token[500]; // cookie and token fields
    bool logged_in = false; // if you are currently logged in or not
    bool in_library = false; // if you have access to the library

    while(true) {
        fgets(buff, 100, stdin);  

        // exit cmd, stops program
        if (strncmp(buff, "exit", 4) == 0)
            break;
        // register a new account
        else if (strncmp(buff, "register", 8) == 0) {
            char *user = (char *)calloc(100, sizeof(char));
            char *pass = (char *)calloc(100, sizeof(char));

            // read user and pass from stdin
            fputs("username=", stdout);
            scanf("%s", user);

            fputs("password=", stdout);
            scanf("%s", pass);
            
            // easy parse if nlohmann json
            nlohmann::json j = {{"username", user}, {"password", pass}};

            // 1 string array; convert json object to string
            char *reg[1];
            reg[0] = new char[j.dump().length() + 1];
            strcpy(reg[0], j.dump().c_str());

            // send to specified url and receive from server
            sockfd = open_connection(host, port, AF_INET, SOCK_STREAM, 0);
            message = compute_post_request(host, (char *)"/api/v1/tema/auth/register", (char *)"application/json", reg, 1, NULL, 0, NULL);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            puts(response);

            // confirmation
            if(strstr(response, "400 Bad Request") != NULL) {
                puts("Account already exists.\n");
            } else if(strstr(response, "201 Created") != NULL) {
                puts("Succesfully created account!\n");
            }

            free(user);
            free(pass);
        // login to a known account
        } else if (strncmp(buff, "login", 5) == 0) {
            // you can't log in to another account while logged in already
            if(!logged_in) {
                char *user = (char *)calloc(100, sizeof(char));
                char *pass = (char *)calloc(100, sizeof(char));

                // user and pass from stdin
                fputs("username=", stdout);
                scanf("%s", user);

                fputs("password=", stdout);
                scanf("%s", pass);
                
                // another easy parse
                nlohmann::json j = {{"username", user}, {"password", pass}};

                // 1 string array; convert json to string
                char *login[1];
                login[0] = new char[j.dump().length() + 1];
                strcpy(login[0], j.dump().c_str());

                // send to specified url and receive from server
                sockfd = open_connection(host, port, AF_INET, SOCK_STREAM, 0);
                message = compute_post_request(host, (char *)"/api/v1/tema/auth/login", (char *)"application/json", login, 1, NULL, 0, NULL);
                send_to_server(sockfd, message);
                response = receive_from_server(sockfd);
                puts(response);

                // if the response is not an error save received cookie
                char *start = strstr(response, "Set-Cookie: ");
                // extract cookie
                if(start != NULL) {
                    logged_in = true;
                    char *token = strtok(start, ";");
                    strcpy(cookie, token + 12);
                    cookie[strlen(cookie)] = '\0';
                    puts("Login successful!\n");
                } else 
                    puts("Credentials do not match\n");

                free(user);
                free(pass);
            } else
                puts("Already logged in!\n");
        // Access library
        } else if (strncmp(buff, "enter_library", 13) == 0) {
            // can't access library while not logged
            if(logged_in) {
                // can't access library a second time
                if(!in_library) {
                    // 1 string array for cookie
                    char *cookies[1];
                    strcpy(cookies[0], cookie);
                    // send to specified url and receive from server
                    sockfd = open_connection(host, port, AF_INET, SOCK_STREAM, 0);
                    message = compute_get_request(host, (char *)"/api/v1/tema/library/access", NULL, cookies, 1, NULL, (char *)"GET");
                    send_to_server(sockfd, message);
                    response = receive_from_server(sockfd);
                    puts(response);
                    printf("\n");

                    // extract and save token
                    char *start = strstr(response, "token");
                    if(start != NULL) {
                        in_library = true;
                        char *tok = strtok(start + 7, "\"");
                        strcpy(token, tok);
                        token[strlen(token)] = '\0';
                        puts("In library!\n");
                    }
                } else {
                    puts("Already in library.\n");
                }
            } else {
                puts("Not logged in!\n");
                continue;
            }
        // Show books from library
        } else if (strncmp(buff, "get_books", 9) == 0) {
            // check if library access
            if(in_library) {
                // send to specified url and receive from server
                sockfd = open_connection(host, port, AF_INET, SOCK_STREAM, 0);
                message = compute_get_request(host, (char *)"/api/v1/tema/library/books", NULL, NULL, 0, token, (char *)"GET");
                send_to_server(sockfd, message);
                response = receive_from_server(sockfd);
                puts(response);
                printf("\n");
            } else 
                puts("No access to library.\n");
        // More information on a specific book
        } else if (strncmp(buff, "get_book", 8) == 0) {
            // check if library access
            if(in_library){
                int aux;

                // get id from stdin
                fputs("id=", stdout);
                scanf("%d", &aux);

                // check if number is valid
                if(aux < 0) {
                    puts("Wrong id format.\n");
                } else {
                    // convert to string
                    char* id = (char *)calloc(100, sizeof(char));
                    sprintf(id, "%d", aux);

                    // send to specified url and receive from server
                    sockfd = open_connection(host, port, AF_INET, SOCK_STREAM, 0);
                    char *url = (char *)calloc(100, sizeof(char));
                    sprintf(url, "/api/v1/tema/library/books/%s", id);
                    message = compute_get_request(host, url, NULL, NULL, 0, token, (char *)"GET");
                    send_to_server(sockfd, message);
                    response = receive_from_server(sockfd);
                    puts(response);
                    printf("\n");
                    free(url);
                    free(id);
                } 
            } else
                puts("No access to library.\n");
        // Add book to library
        } else if (strncmp(buff, "add_book", 8) == 0) {
            // Has access to library
            if(in_library){
                char* title = (char *)calloc(100, sizeof(char));
                char* author = (char *)calloc(100, sizeof(char));
                char* genre = (char *)calloc(100, sizeof(char));
                char* page_count = (char *)calloc(100, sizeof(char));
                char* publisher = (char *)calloc(100, sizeof(char));

                // get all book info
                fputs("title=", stdout);
                fgets(title, 100, stdin);
                title[strlen(title) - 1] = '\0';

                fputs("author=", stdout);
                fgets(author, 100, stdin);
                author[strlen(author) - 1] = '\0';

                fputs("genre=", stdout);
                fgets(genre, 100, stdin);
                genre[strlen(genre) - 1] = '\0';

                fputs("page_count=", stdout);
                fgets(page_count, 100, stdin);
                page_count[strlen(page_count) - 1] = '\0';

                fputs("publisher=", stdout);
                fgets(publisher, 100, stdin);
                publisher[strlen(publisher) - 1] = '\0';

                printf("\n");

                char *endptr;

                // check if number is valid
                int check = strtol(page_count, &endptr, 10);
                if(*endptr != '\0' || check < 0){
                    puts("Incorrect page_count format!\n");
                    continue;
                }

                // easiest json parse
                nlohmann::json j = {
                    {"title", title}, 
                    {"author", author},
                    {"genre", genre},
                    {"page_count", page_count},
                    {"publisher", publisher}
                    };

                // json to string
                char *book[1];
                book[0] = new char[j.dump().length() + 1];
                strcpy(book[0], j.dump().c_str());

                // send to specified url and receive from server
                sockfd = open_connection(host, port, AF_INET, SOCK_STREAM, 0);
                message = compute_post_request(host, (char *)"/api/v1/tema/library/books", (char *)"application/json", book, 1, NULL, 0, token);
                send_to_server(sockfd, message);
                response = receive_from_server(sockfd);
                puts(response);

                // check if success
                if(strstr(response, "200 OK") != NULL)
                    puts("Book added!\n");
                else {
                    puts("Something went wrong.\n");
                }
                free(title);
                free(author); 
                free(genre); 
                free(publisher);
            } else
                puts("No access to library.\n");
        // Delete book from library
        } else if (strncmp(buff, "delete_book", 11) == 0) {
            // Check for access
            if(in_library){
                int aux;

                // get input
                fputs("id=", stdout);
                scanf("%d", &aux);

                // valid number
                if(aux < 0) {
                    puts("Wrong id format.\n");
                } else {
                    char* id = (char *)calloc(100, sizeof(char));
                    sprintf(id, "%d", aux);

                    // send to specified url and receive from server
                    sockfd = open_connection(host, port, AF_INET, SOCK_STREAM, 0);
                    char *url = (char *)calloc(100, sizeof(char));
                    sprintf(url, "/api/v1/tema/library/books/%s", id);
                    message = compute_get_request(host, url, NULL, NULL, 0, token, (char *)"DEL");
                    send_to_server(sockfd, message);
                    response = receive_from_server(sockfd);
                    puts(response);
                    
                    // check for success
                    if(strstr(response, "200 OK") != NULL)
                        puts("Book deleted!\n");
                    else
                        puts("Something went wrong.\n");
                    free(url);
                    free(id);
                } 
            } else
                puts("No access to library.\n");
        // Logout
        } else if (strncmp(buff, "logout", 6) == 0) {
            // check if logged in
            if(logged_in) {
                char *cookies[1];
                strcpy(cookies[0], cookie);
                // send to specified url and receive from server
                sockfd = open_connection(host, port, AF_INET, SOCK_STREAM, 0);
                message = compute_get_request(host, (char *)"/api/v1/tema/auth/logout", NULL, cookies, 1, token, (char *)"GET");
                send_to_server(sockfd, message);
                response = receive_from_server(sockfd);
                puts(response);
                // no access to library and not logged anymore
                logged_in = false;
                in_library = false;
                puts("Logout successful!\n");
            } else 
                puts("Not logged in.\n");

        } 
        close_connection(sockfd);
    }
    if(message != NULL)
        free(message);
    if(response != NULL)
        free(response);

    return 0;
}