#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

//          This program uses LSB method 

//      Group 16
//  Evrim Gizem İŞCİ        2020510091
//  Yiğit CAN               2021510021
//  Eylül PINAR YETİŞKİN    2021510069

#define DATA_SIZE 100
#define PORT_NUMBER 60000

char INPUT_STRING[DATA_SIZE];
int FIRST_ARRAY[DATA_SIZE];
int SECOND_ARRAY[DATA_SIZE];
int CARRY_ARRAY[DATA_SIZE];
int RESULT_ARRAY[DATA_SIZE];
pthread_t THREAD_ARRAY[DATA_SIZE];
int server_socket, client_socket;

// Check if input is numeric or not
int isNumericString(const char *s) {
    // Check if input is empty
    if (s == NULL || *s == '\0') {
        printf("Error: Empty string\n");
        send(client_socket, "Error: Empty string\n", strlen("Error: Empty string\n"), 0);
        return 0;
    }
    // Check if there is unexpected character in input
    while (*s) {
        if (!isdigit((unsigned char)*s) && *s != ' ' && *s != '\r' && *s != '\n' && *s != '-' && *s!='\t') {
            printf("Invalid character: '%c' (ASCII: %d)\n", (unsigned char)*s, (int)*s);
            send(client_socket, "Invalid character\n", strlen("Invalid character\n"), 0);
            return 0;
        }
        s++;
    }
    return 1;
}
// Clean spaces in input
void cleanSpaces(char *s) {
    int i, j;
    int spaceFound = 0;

    // Remove leading spaces
    while (s[0] == ' ') {
        memmove(s, s + 1, strlen(s));
    }

    for (i = 0, j = 0; s[i] != '\0'; i++) {
        if (s[i] == ' ') {
            if (!spaceFound) {
                s[j++] = ' ';
                spaceFound = 1;
            }
        } else {
            s[j++] = s[i];
            spaceFound = 0;
        }
    }

    // Remove trailing spaces
    while (j > 0 && (s[j - 1] == ' '|| s[j - 1] == '\n' || s[j - 1] == '\t'|| s[j - 1] == '\r')) {
        j--;
    } 
    s[j] = '\0';
    

    // Remove consecutive spaces
    for (i = 0, j = 0; s[i] != '\0'; i++) {
        if (s[i] == ' ' || s[i] == '\t' || s[i] == '\n' || s[i] == '\r') {
            while (s[i + 1] == ' ') {
                i++;
            }
            s[j++] = ' ';
        } else {
            s[j++] = s[i];
        }
    }
    s[j] = '\0';
}
// Convert input string to integer array
int stringToIntArray(const char *s, int *array, char *errorMessage) {
   
    char cleanString[strlen(s) * 2 + 1];
    strcpy(cleanString, s);
    cleanSpaces(cleanString);

    printf("Cleaned String: %s\n", cleanString);
    printf("Cleaned string length is: %d\n",strlen(cleanString));
    if(strlen(s) > 100){
        sprintf(errorMessage, "Error: Input array size is bigger than expected!!\n");
        return -4; // There is more than DATA_SIZE character
    }
    // Split input by space character
    char *token = strtok(cleanString, " ");
    int i = 0;

    while (token != NULL) {
        //Convert char to integer
        int value = atoi(token);

        // If there is an integer more than 999, set the errorMessage and return -1
        if (value > 999) {
            sprintf(errorMessage, "Error: There are integer larger than 999\n");
            return -1; // Bigger than 999 error
        }

        // If there is a negative integer, set the errorMessage and return -3
        if (value < 0) {
            sprintf(errorMessage, "Error: There is a negative integer number\n");
            return -3; // Negative integer error
        }

        array[i++] = value;
        token = strtok(NULL, " ");

        // Array's integer is bigger than DATA_SIZE, set the errorMessage and return -2
        if (i >= DATA_SIZE) {
            sprintf(errorMessage, "Error: Integer array size is larger than expected!!\n");
            return -2; // There is more than DATA_SIZE integer input
        }
    }

    printf("Array size: %d\n", i);
    return i;
}
// Array addition
void *arrayAddition(void *index) {
    int i = *((int *)index);
    int k = i + 1;
    RESULT_ARRAY[k] = FIRST_ARRAY[i] + SECOND_ARRAY[i];
    if (RESULT_ARRAY[k] > 999) {
        CARRY_ARRAY[i] = RESULT_ARRAY[k] / 1000; // Add carry
        RESULT_ARRAY[k] %= 1000; // Add result array
    } else {
        CARRY_ARRAY[i] = 0; // If there is no carry
    }

    free(index);  // Free the memory
    pthread_exit(NULL);
}
// Telnet connection
void handleTelnetConnection(int client_socket) {
    printf("Hello, this is Array Addition Server!\n");
    send(client_socket, "Hello, this is Array Addition Server!\n", strlen("Hello, this is Array Addition Server!\n"), 0);
    int numElementsFirstArray;
    int numElementsSecondArray;
    char errorMessage[200];  // Assuming error messages won't exceed 200 characters
    memset(INPUT_STRING, 0, sizeof(INPUT_STRING));
    memset(errorMessage, 0, sizeof(errorMessage));
    // Receive and process the first array
    // ----------------------------------------------- Input integer placement is LEAST SIGNIFICANT BIT <-------------------------------------------
    printf("Please enter the first array for addition(using LSB):\n");
    send(client_socket, "Please enter the first array for addition(using LSB):\n", strlen("Please enter the first array for addition(using LSB):\n"), 0);
    ssize_t bytes_received = recv(client_socket, INPUT_STRING, DATA_SIZE, 0);

    // Check for receive error
    if (bytes_received < 0) {
        perror("Error receiving data\n");
        send(client_socket, "Error receiving data\n", strlen("Error receiving data\n"), 0);
        close(client_socket);
        // Handle the error as needed
        return;
    }

    // Null-terminate the received string
    INPUT_STRING[bytes_received] = '\0';

    // Check if the input string exceeds the limit
    if (bytes_received == DATA_SIZE) {
        printf("Error: Input string exceeds the size limit of %d characters.\n", DATA_SIZE);
        send(client_socket, "Error: Input string exceeds the size limit.\n", strlen("Error: Input string exceeds the size limit.\n"), 0);
        close(client_socket);
        // Handle the error as needed
        return;
    }
    // Check input string
    if (!isNumericString(INPUT_STRING)) {
        sprintf(errorMessage, "Error: The inputted integer array contains non-integer characters.\n");
        send(client_socket, errorMessage, strlen(errorMessage), 0);
        close(client_socket);
        return;
    }
    numElementsFirstArray = stringToIntArray(INPUT_STRING, FIRST_ARRAY, errorMessage);
    //Error detection
    if (numElementsFirstArray == -1 || numElementsFirstArray == -2 || numElementsFirstArray == -3|| numElementsFirstArray == -4) {
        send(client_socket, errorMessage, strlen(errorMessage), 0);
        close(client_socket);
        return;
    }

    memset(INPUT_STRING, 0, sizeof(INPUT_STRING));
    // Receive and process the second array
    printf("Please enter the second array for addition:\n");
    send(client_socket, "Please enter the second array for addition(using LSB):\n", strlen("Please enter the second array for addition(using LSB):\n"), 0);
    bytes_received = recv(client_socket, INPUT_STRING, DATA_SIZE, 0);

    // Check for receive error
    if (bytes_received < 0) {
        perror("Error receiving data\n");
        send(client_socket, "Error receiving data\n", strlen("Error receiving data\n"), 0);
        close(client_socket);
        // Handle the error as needed
        return;
    }

    // Null-terminate the received string
    INPUT_STRING[bytes_received] = '\0';

    // Check if the input string exceeds the limit
    if (bytes_received == DATA_SIZE) {
        printf("Error: Input string exceeds the size limit of %d characters.\n", DATA_SIZE);
        send(client_socket, "Error: Input string exceeds the size limit.\n", strlen("Error: Input string exceeds the size limit.\n"), 0);
        close(client_socket);
        // Handle the error as needed
        return;
    }
    // Check input string
    if (!isNumericString(INPUT_STRING)) {
        sprintf(errorMessage, "Error: The inputted integer array contains non-integer characters.\n");
        send(client_socket, errorMessage, strlen(errorMessage), 0);
        close(client_socket);
        return;
    }
    numElementsSecondArray = stringToIntArray(INPUT_STRING, SECOND_ARRAY, errorMessage);
    //Error detection
    if (numElementsSecondArray == -1 || numElementsSecondArray == -2 || numElementsSecondArray == -3|| numElementsSecondArray == -4) {
        send(client_socket, errorMessage, strlen(errorMessage), 0);
        close(client_socket);
        return;
    }
    
    memset(INPUT_STRING, 0, sizeof(INPUT_STRING));

    // Check if the number of integers is the same for both arrays
    if (numElementsFirstArray != numElementsSecondArray) {
        sprintf(errorMessage, "Error: The number of integers is different for both arrays. You must send an equal number of integers for both arrays!\n");
        send(client_socket, errorMessage, strlen(errorMessage), 0);
        close(client_socket);
        return;
    }

    // Create threads for array addition
    int i;
    for (i = 0; i < numElementsFirstArray; i++) {
        int *threadIndex = malloc(sizeof(int));
        *threadIndex = i;
        pthread_create(&THREAD_ARRAY[i], NULL, arrayAddition, (void *)threadIndex);
    }

    // Wait for threads to complete
    for (i = 0; i < numElementsFirstArray; i++) {
        pthread_join(THREAD_ARRAY[i], NULL);
    }

    // Add the calculated carries
    for (i = numElementsFirstArray - 1; i > 0; i--) {
        RESULT_ARRAY[i] += CARRY_ARRAY[i];
        if (RESULT_ARRAY[i] > 999) {
            CARRY_ARRAY[i - 1] = RESULT_ARRAY[i] / 1000;
            RESULT_ARRAY[i] %= 1000;
        }
    }

    // Carry is added to index of 0
    RESULT_ARRAY[0] += CARRY_ARRAY[0];

    if (CARRY_ARRAY[0] == 1) {
        // Convert the relevant part of RESULT_ARRAY to INPUT_STRING
        snprintf(INPUT_STRING, sizeof(INPUT_STRING), "%d", RESULT_ARRAY[0]);

        for (i = 1; i < numElementsFirstArray + 1; i++) {
            snprintf(INPUT_STRING + strlen(INPUT_STRING), sizeof(INPUT_STRING) - strlen(INPUT_STRING) - 1, " %d", RESULT_ARRAY[i]);
        }
    } else {
        // No carry, so convert the 1 to n index
        snprintf(INPUT_STRING, sizeof(INPUT_STRING), "%d", RESULT_ARRAY[1]);

        for (i = 2; i < numElementsFirstArray + 1; i++) {
            snprintf(INPUT_STRING + strlen(INPUT_STRING), sizeof(INPUT_STRING) - strlen(INPUT_STRING) - 1, " %d", RESULT_ARRAY[i]);
        }
    }   

    // Send the result back to the client
    printf("\nThe result of array addition is sent to the client.\n");
    send(client_socket, "The result of array addition is sent to the client.\n", strlen("The result of array addition is sent to the client.\n"), 0);
    send(client_socket, INPUT_STRING, strlen(INPUT_STRING), 0);

    
    printf("Thank you for Array Addition Server! Good Bye!\n");
    send(client_socket, "\nThank you for Array Addition Server! Good Bye!\n", strlen("Thank you for Array Addition Server! Good Bye!\n"), 0);

    // Close the connection
    close(client_socket);
}

int main() {
    struct sockaddr_in server_address, client_address;
    

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // Initialize server_address structure
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(PORT_NUMBER);

    // Bind the socket
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        perror("Error binding socket");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_socket, 5) == -1) {
        perror("Error listening for connections");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT_NUMBER);

    while (1) {
        //Clear all array for reuse
        memset(INPUT_STRING, 0, sizeof(INPUT_STRING));
        memset(RESULT_ARRAY, 0, sizeof(RESULT_ARRAY));
        memset(CARRY_ARRAY, 0, sizeof(CARRY_ARRAY));
        memset(FIRST_ARRAY, 0, sizeof(FIRST_ARRAY));
        memset(SECOND_ARRAY, 0, sizeof(SECOND_ARRAY));
        // Accept a connection
        printf("Waiting for a connection...\n");
        socklen_t client_address_len = sizeof(client_address);
        client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_address_len);
        if (client_socket == -1) {
            perror("Error accepting connection");
            continue;
        }
        printf("Client connected!\n");
        // Handle the telnet connection
        handleTelnetConnection(client_socket);
    }

    // Close the server socket
    close(server_socket);

    return 0;
}