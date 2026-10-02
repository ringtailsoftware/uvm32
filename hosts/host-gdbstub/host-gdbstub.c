#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "uvm32.h"
#include "gdbstub.h"

#define PORT 1234

static uint8_t *read_file(const char* filename, int *len) {
  FILE* f = fopen(filename, "rb");
  uint8_t *buf = NULL;

  if (f == NULL) {
    fprintf(stderr, "error: can't open file '%s'.\n", filename);
    return NULL;
  }

  fseek(f, 0, SEEK_END);
  size_t file_size = ftell(f);
  rewind(f);

  if (NULL == (buf = malloc(file_size))) {
    fclose(f);
    return NULL;
  }

  size_t result = fread(buf, sizeof(uint8_t), file_size, f);
  if (result != file_size) {
    fprintf(stderr, "error: while reading file '%s'\n", filename);
    free(buf);
    fclose(f);
    return NULL;
  }
  fclose(f);

  *len = file_size;
  return buf;
}

void usage(const char *name) {
  fprintf(stderr, "%s filename.bin\n", name);
  exit(1);
}

int main(int argc, char *argv[]) {
  uvm32_state_t vmst;
  const char *rom_filename = NULL;
  int romlen = 0;    

  if (2 == argc) {
    rom_filename = argv[1];
  } else {
    usage(argv[0]);
  }

  uint8_t *rom = read_file(rom_filename, &romlen);
  if (NULL == rom) {
    fprintf(stderr,"file read failed!\n");
    return (-1);
  }

  uvm32_init(&vmst);
  uvm32_useBreak(&vmst, true); // enable ebreak

  if (!uvm32_load(&vmst, rom, romlen)) {
    fprintf(stderr, "load failed!\n");
    return (-2);
  }

  // Create server socket
  int server_sock;
  struct sockaddr_in server_addr;

  if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    fprintf(stderr, "Socket creation failed\n");
    return (-3);
  }

  // Set up the server address
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(PORT);

  // Bind the server socket
  if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
      0) {
    fprintf(stderr, "Bind failed\n");
    close(server_sock);
    return (-4);
  }

  // Listen for incoming connections
  if (listen(server_sock, 1) < 0) {
    fprintf(stderr, "Listen failed\n");
    close(server_sock);
    return (-5);
  }

  fprintf(stdout, "GDB Stub listening on port %d...\n", PORT);

  struct sockaddr_in client_addr;
  socklen_t addr_len = sizeof(client_addr);

  // Accept a client connection
  int client_sock =
      accept(server_sock, (struct sockaddr *)&client_addr, &addr_len);
  if (0 > client_sock) {
    fprintf(stderr, "Accept failed: %d\n", client_sock);
    close(server_sock);
    return (-6);
  }

  printf("GDB connected from %s\n", inet_ntoa(client_addr.sin_addr));

  /* Attach gdbstub to vmst */
  struct gdb_state state;
  gdb_sys_init_state(&state, &vmst, client_sock, NULL);

  while(1) {
    gdb_main(&state);
  }

  free(rom);
  return 0;
}
