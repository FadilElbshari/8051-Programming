#define INSTRUCT_SIZE 8
#define MAX_CODE_SIZE 256
#define MAX_WAIT_TIME_MS 2000 
#define BYTE_CHAR_SIZE 3
#define ISNTRUCT_BYTES 4
#define MAX_CODE_LINES 10

#define MOSI  8 // Instruction Pin, P1.5, PIN 6
#define MISO  9 // Output Pin, P1.6, PIN 7
#define SCK  10 // Serial Clock Pin, P1.7, PIN 8
#define RST  11 // Reset Pin, PIN 9

#define WRITE 0

typedef unsigned char U8;
typedef unsigned short U16;
typedef unsigned int U32;

constexpr unsigned int CLK_SPEED = 500; 
constexpr U8 ONE = 0x80; 
typedef U8 INSTRUCTION[4];

constexpr INSTRUCTION PROG_EN = {0xAC, 0x53, 0x00, 0x00}; 
constexpr INSTRUCTION ERASE_CHIP = {0xAC, 0x80, 0x00, 0x00};

// Prototypes
void send_instruction(const U8 *instructList, size_t size, bool read=false);

char hex_file[MAX_CODE_LINES][MAX_CODE_SIZE];
size_t hex_index = 0;
int lines=0;

inline U32 construct_32bit_number(INSTRUCTION instruct) {
  return ((U32)instruct[0] << 24) | ((U32)instruct[1] << 16) | ((U32)instruct[2] << 8) | instruct[3];
}

inline U8 get_bits2B(char byte[BYTE_CHAR_SIZE]) {
      auto hex_to_int = [](char c) {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        return 0;
    };
    return hex_to_int(byte[0]) * 16 + hex_to_int(byte[1]);
}

inline U8 get_bits4B(char byte[BYTE_CHAR_SIZE+2]) {
      auto hex_to_int = [](char c) {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        return 0;
    };
    return (hex_to_int(byte[0]) << 12) + 
           (hex_to_int(byte[1]) << 8) + 
           (hex_to_int(byte[2]) << 4) + 
           hex_to_int(byte[3]);
}

void construct_write_instruction(U16 address, U8 data, U8* result) {
  result[0] = 0x40;
  result[1] = (address >> 8) & 0x1F;
  result[2] = address & 0xFF;
  result[3] = data;
}

void interpret_data(char hexFile[MAX_CODE_LINES][MAX_CODE_SIZE], int length) {

  for (int i=0; i<length; i++) {
    size_t index = 1; // Bypass first colon

    // 1. Data Count
    char data_count_string[BYTE_CHAR_SIZE];
    data_count_string[0] = hexFile[i][index++];
    data_count_string[1] = hexFile[i][index++];

    U8 data_count = get_bits2B(data_count_string);

    Serial.print("Number of bytes: ");
    Serial.println(data_count);

    // 2. Address
    char address_string[BYTE_CHAR_SIZE+2];
    address_string[0] = hexFile[i][index++];
    address_string[1] = hexFile[i][index++];
    address_string[2] = hexFile[i][index++];
    address_string[3] = hexFile[i][index++];

    U16 start_address = get_bits4B(address_string);

    // 2. Record
    char record_string[BYTE_CHAR_SIZE];
    record_string[0] = hexFile[i][index++];
    record_string[1] = hexFile[i][index++];

    U8 record = get_bits2B(record_string);

    U16 addr = start_address;
    for (U8 j = 0; j < data_count; j++) {
      char data_byte_string[BYTE_CHAR_SIZE];
      data_byte_string[0] = hexFile[i][index++];
      data_byte_string[1] = hexFile[i][index++];
      U8 data_byte = get_bits2B(data_byte_string);
      INSTRUCTION instruction;
      construct_write_instruction(addr++, data_byte, instruction);
      send_instruction(instruction, ISNTRUCT_BYTES);
      // Serial.print(addr, HEX);
      // Serial.print(" - ");
      // Serial.print(data_byte, HEX);
      // Serial.println();
      // delay(10);
    }

    Serial.println();
  }
  
  
}

void clock_gen() {
  digitalWrite(SCK, HIGH);
  delayMicroseconds(CLK_SPEED);
  digitalWrite(SCK, LOW);
  delayMicroseconds(CLK_SPEED);
}

void send_byte(const U8 byte) {
  for (int i=0; i<INSTRUCT_SIZE; i++) {
      digitalWrite(MOSI, (((byte << i) & ONE) ? HIGH : LOW));
      clock_gen();
  }
}

U8 send_and_read_byte(const U8 byte) {
  U8 response = 0;
  for (int i = 0; i < 8; i++) {
    digitalWrite(MOSI, ((byte << i) & ONE) ? HIGH : LOW);

    digitalWrite(SCK, HIGH);
    delayMicroseconds(CLK_SPEED);

    response <<= 1;
    if (digitalRead(MISO)) {
      response |= 1;
    }

    digitalWrite(SCK, LOW);
    delayMicroseconds(CLK_SPEED);
  }
  return response;
}

void send_instruction(const U8 *instructList, size_t size, bool read) {
  for (size_t i=0; i<size; i++) {
    if (i == size - 1 && read) {
      U8 response = send_and_read_byte(instructList[i]);
      Serial.print("Response: 0x");
      Serial.println(response, HEX);
    } else {
      Serial.print("Sending: ");
      Serial.println(instructList[i], HEX);
      send_byte(instructList[i]);
    }
  }
}

int read_file(char buffer[MAX_CODE_LINES][MAX_CODE_SIZE], size_t maxSize) {

  int index = 0;
  while (Serial.available() && hex_index < maxSize - 1) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      buffer[index][hex_index] = '\0';
      hex_index = 0;
      index ++;
    } else {
      buffer[index][hex_index++] = c;
    }

  }
  return index;
}



void setup() {
  
  Serial.begin(9600);

  pinMode(MOSI, OUTPUT);
  pinMode(MISO, INPUT);
  pinMode(SCK, OUTPUT);
  pinMode(RST, OUTPUT);

  // Step 1, RST -> HIGH and send programming enable instruction
  digitalWrite(RST, LOW);
  delay(10);
  digitalWrite(RST, HIGH);
  digitalWrite(SCK, LOW);

  // Step 2, send programming enable instruction and chip erase
  send_instruction(PROG_EN, sizeof(PROG_EN), true);
  delay(1000);
  

  while (!Serial.available());
  delay(MAX_WAIT_TIME_MS);
  lines = read_file(hex_file, MAX_CODE_SIZE);
  send_instruction(ERASE_CHIP, sizeof(ERASE_CHIP));
  delay(1000);
  interpret_data(hex_file, lines);

  // INSTRUCTION read = {0x20, 0x00, 0x10, 0x00}; 
  // send_instruction(read, sizeof(read), true);
  // delay(20);

  // Final step: RST -> LOW to allow normal operation
  digitalWrite(RST, LOW);


}

void loop() {
}
