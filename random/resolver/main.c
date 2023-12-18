#include <fcntl.h>
#include <netinet/in.h>
#include <semaphore.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#define DNS_MESSAGE_MAX_BYTES 512
#define QNAME_MAX_SIZE 255
#define QUESTION_SIZE_BYTES (255 + (2 * 16))
#define CONNECTION_PORT 53
// header + len of name + type + class + ttl + rdata length + IPv4 address
// 12     +    255      + 2    + 2     + 4   + 2            + 4           = 281
#define TYPE_A_MAX_RESPONSE_SIZE 281
#define MESSAGE_TYPE_QUERY 0
#define MESSAGE_TYPE_RESPONSE 1
#define QCLASS_IN 1
#define QTYPE_A 1

struct header {
  uint16_t id;
  uint16_t bit_field;
  uint16_t qd_count;
  uint16_t an_count;
  uint16_t ns_count;
  uint16_t ar_count;
};

struct question {
  // TODO: DO I NEED THIS?
  // THIS COULD BE JUST A POINTER TO THE START OF THE QUESTION SINCE IT'S NULL
  // TERMINATED.
  uint8_t qname[255];
  uint16_t qtype;
  uint16_t qclass;
  uint8_t qname_octet_count;
};

uint16_t message_type(uint16_t bit_field) { return (bit_field & 0x8000) >> 15; }

struct header load_header(const uint8_t storage_buffer[DNS_MESSAGE_MAX_BYTES]) {
  struct header header;
  memcpy(&header, storage_buffer, sizeof(struct header));
  header.id = ntohs(header.id);
  header.bit_field = ntohs(header.bit_field);
  header.an_count = ntohs(header.an_count);
  header.ar_count = ntohs(header.ar_count);
  header.ns_count = ntohs(header.ns_count);
  header.qd_count = ntohs(header.qd_count);
  /*
  // 1000 0000 0000 0000
  uint16_t qr = (header.bit_field & 0x8000) >> 15;

  // 0111 1000 0000 0000
  uint16_t opcode = (header.bit_field & 0x7800) >> 11;

  // 0000 0100 0000 0000
  uint16_t aa = (header.bit_field & 0x0400) >> 10;

  // 0000 0010 0000 0000
  uint16_t tc = (header.bit_field & 0x0200) >> 9;

  // 0000 0001 0000 0000
  uint16_t rd = (header.bit_field & 0x0100) >> 8;

  // 0000 0000 1000 0000
  uint16_t ra = (header.bit_field & 0x0080) >> 7;

  // 0000 0000 0000 1111
  uint16_t rcode = (header.bit_field & 0x000F);
  */
  return header;
}

int load_questions(const uint8_t storage_buffer[], struct question questions[],
                   uint16_t QD_COUNT, uint16_t storage_buffer_length) {

  const uint8_t *question_bytes = storage_buffer + sizeof(struct header);

  uint16_t questions_processed = 0;
  const uint8_t *question_start = question_bytes;
  while (questions_processed != QD_COUNT) {
    const uint8_t *c = question_start;
    uint8_t qname_octet_count = 1;
    while (*c != '\0' &&
           c < (question_bytes +
                (storage_buffer_length - sizeof(struct header))) &&
           qname_octet_count <= QNAME_MAX_SIZE) {
      c++;
      // todo remove the qname count and place it after the loop maybe;
      // todo actually replace the length octects with dot.
      qname_octet_count++;
    };
    if (qname_octet_count == 1) {
      perror("QNAME CAN'T HAVE ZERO LENGTH");
      return -1;
    }
    if (*c != '\0') {
      perror("question loading failed as QNAME as improper size or format");
      return -1;
    }
    struct question question;
    // todo what does memcpy return??
    memcpy(&question.qname, question_start, qname_octet_count);
    memcpy(&question.qtype, question_start + qname_octet_count,
           sizeof(uint16_t));
    memcpy(&question.qclass,
           question_start + qname_octet_count + sizeof(uint16_t),
           sizeof(uint16_t));

    question.qtype = ntohs(question.qtype);
    question.qclass = ntohs(question.qclass);
    question_start = c + (2 * (sizeof(uint16_t))) + 1;

    question.qname_octet_count = qname_octet_count;

    questions[questions_processed] = question;

    questions_processed++;
  }
  return 1;
}

int answer_first_A_question(int socket_descriptor, struct question questions[],
                            struct header query_header,
                            struct sockaddr_in connection_address) {
  uint8_t type_A_output_buffer[TYPE_A_MAX_RESPONSE_SIZE];
  struct header response_header;
  // TODO do I need to convert id back and forth when it's not used anywhere
  // as a number?
  response_header.id = htons(query_header.id);
  response_header.bit_field = 0;
  // set QR to 1 to indicate response
  response_header.bit_field = htons(0x8000);
  response_header.an_count = htons(1);
  // assign all the remaining fields to 0 to prevent polution of the response.
  response_header.qd_count = 0;
  response_header.ns_count = 0;
  response_header.ar_count = 0;
  uint8_t *destination = type_A_output_buffer;
  // store header into the output buffer
  memcpy(destination, &response_header, sizeof(struct header));
  destination = destination + sizeof(struct header);

  if (query_header.qd_count == 0) {
    printf("there is nothing to respond to: question count is zero\n");
    return -1;
  }
  if (questions[0].qtype != QTYPE_A || questions[0].qclass != QCLASS_IN) {
    printf("answer_first_A_question is only able to answer qtype A of qclass "
           "IN \n");
    return -1;
  }
  // copy the name into the answer section
  memcpy(destination, questions[0].qname, questions[0].qname_octet_count);
  destination = destination + questions[0].qname_octet_count;
  // copy type a62950nd class into the answer section
  uint16_t type = htons(questions[0].qtype);
  uint16_t class = htons(questions[0].qclass);

  memcpy(destination, &type, sizeof(uint16_t));
  destination = destination + sizeof(uint16_t);

  memcpy(destination, &class, sizeof(uint16_t));
  destination = destination + sizeof(uint16_t);

  // set ttl to an hour
  uint32_t ttl = htons(3600);
  memcpy(destination, &ttl, sizeof(uint32_t));
  destination = destination + sizeof(uint32_t);

  // set response data length to 4 for ipv4 address.
  uint16_t rdlength = htons(4);
  memcpy(destination, &rdlength, sizeof(uint16_t));
  destination = destination + sizeof(uint16_t);

  uint8_t ipv4_data[4] = {192, 168, 0, 2};
  memcpy(destination, ipv4_data, 4);
  destination =
      destination + 4; // todo use const intead of hardcode value or use sizeoff

  size_t bytes_to_send = destination - type_A_output_buffer;

  int bytes_sent = sendto(
      socket_descriptor, type_A_output_buffer, bytes_to_send, 0,
      (struct sockaddr *)&connection_address, sizeof(connection_address));
  if (bytes_sent < 0) {
    perror("sendto()");
    return -1;
  }
  return 1;
}

int send_not_implemented(int socket_descriptor, struct header query_header,
                         struct sockaddr_in connection_address) {
  uint8_t output_buffer[sizeof(struct header)];
  struct header response_header;
  response_header.id = htons(query_header.id);

  // set QR to 1 to indicate response
  response_header.bit_field = 0x8000;
  // set RCODE to 4 to indicate Not Implemented.
  response_header.bit_field = response_header.bit_field | 0x0004;
  response_header.bit_field = htons(response_header.bit_field);

  response_header.an_count = 0;
  response_header.qd_count = 0;
  response_header.ns_count = 0;
  response_header.ar_count = 0;

  // store header into the output buffer
  memcpy(output_buffer, &response_header, sizeof(struct header));

  int bytes_sent = sendto(
      socket_descriptor, output_buffer, sizeof(output_buffer), 0,
      (struct sockaddr *)&connection_address, sizeof(connection_address));
  if (bytes_sent < 0) {
    perror("sendto()");
    return -1;
  }
  return 1;
}

static sem_t stop;
static void sig_handler(int signal_number) {
  printf("got signal %d", signal_number);
  sem_post(&stop);
}

int run_server_loop(int socket_descriptor) {
  socklen_t connection_address_size = sizeof(struct sockaddr_in);
  int should_stop = 0;
  while (should_stop <= 0) {

    struct sockaddr_in connection_address;
    uint8_t storage_buffer[DNS_MESSAGE_MAX_BYTES] = {0};

    int bytes_recieved = recvfrom(
        socket_descriptor, storage_buffer, DNS_MESSAGE_MAX_BYTES, 0,
        (struct sockaddr *)&connection_address, &connection_address_size);
    if (bytes_recieved < 0) {
      perror("recvfrom()");
      return EXIT_FAILURE;
    }

    struct header header = load_header(storage_buffer);
    if (message_type(header.bit_field) == MESSAGE_TYPE_QUERY) {
      struct question questions[header.qd_count];
      if (load_questions(storage_buffer, questions, header.qd_count,
                         bytes_recieved) < 0) {
        printf("failed to load questions\n");
        continue;
      }

      if (answer_first_A_question(socket_descriptor, questions, header,
                                  connection_address) < 0) {
        printf("sending Not Implemented response\n");
        if (send_not_implemented(socket_descriptor, header,
                                 connection_address) < 0) {
          printf("failed to send Not Implemented response\n");
        }
        continue;
      }

    } else {
      printf(
          "response handling is not implemented but response was recieved\n");
      return EXIT_FAILURE;
      // TODO handle the responses, if any are pending...
    }

    // check stop signal
    if (sem_getvalue(&stop, &should_stop) < 0) {
      perror("sem_getvalue()");
      return -1;
    }
  }
  return EXIT_SUCCESS;
}

int main() {

  int socket_descriptor;

  // TODO WHAT DOES THIS MEAN??
  int option_value = 1;
  struct sockaddr_in server_address;

  socket_descriptor = socket(AF_INET, SOCK_DGRAM, 0);
  if (socket_descriptor < 0) {
    perror("Socket creation failed");

    exit(EXIT_FAILURE);
  }

  int status = setsockopt(socket_descriptor, SOL_SOCKET, SO_REUSEADDR,
                          &option_value, sizeof(option_value));
  if (status < 0) {
    perror("Couldn't set options");
    exit(EXIT_FAILURE);
  }

  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(CONNECTION_PORT);
  server_address.sin_addr.s_addr = INADDR_ANY;
  status = bind(socket_descriptor, (struct sockaddr *)&server_address,
                sizeof(struct sockaddr));
  if (status < 0) {
    perror("Couldn't bind socket");
    exit(EXIT_FAILURE);
  }

  // handle signals
  sem_init(&stop, 0, 0);
  signal(SIGINT, sig_handler);
  signal(SIGTERM, sig_handler);

  // run the main loop
  status = run_server_loop(socket_descriptor);

  close(socket_descriptor);
  return status;
}