// ------------------------------- ISP based 8051 programmer -------------------------------
// - Method explained below is made by reffering to the ATMEL "AT89S52" datasheet.
// 1. This programmer uses a bintbang method.
// 2. Waits for a HEX file to be sent on the serial port.
// 3. Initialses the 8051 to programming enable mode.
// 4. Perform a chip erase process, to allow for clean upload.
// 5. Read in the hex file and store all bytes in their relative address locations.
// - Pinout is given below in the pin definition section.
// ------------------------------- By Fadil ELBSHARI ---------------------------------------


// Constants definition
#define INSTRUCT_SIZE_BITS 8
#define MAX_CODE_SIZE_PER_LINE_BYTES 256
#define MAX_CODE_LINES 10
#define HEX_BYTE_STRING_SIZE 3
#define ISNTRUCT_BYTES 4
#define MAX_WAIT_TIME_FOR_FILE_MS 2000 
#define DELAY 1000

// Pin definition
#define MOSI  8 // Instruction Pin, P1.5, PIN 6
#define MISO  9 // Output Pin, P1.6, PIN 7
#define SCK  10 // Serial Clock Pin, P1.7, PIN 8
#define RST  11 // Reset Pin, PIN 9

// 8-bit, 16-bit, 32-bit and 8_bit*4 type definitions
typedef unsigned char U8;
typedef unsigned short U16;
typedef unsigned int U32;
typedef U8 INSTRUCTION[4];

// Constants
constexpr unsigned int CLK_SPEED = 500; // 500 us
constexpr U8 ONE = 0x80; 

// Static instructions
constexpr INSTRUCTION PROG_EN = {0xAC, 0x53, 0x00, 0x00}; 
constexpr INSTRUCTION ERASE_CHIP = {0xAC, 0x80, 0x00, 0x00};

// Global variables
char hex_file[MAX_CODE_LINES][MAX_CODE_SIZE_PER_LINE_BYTES];
size_t hex_index = 0;
int lines=0;

// Function prototypes
void send_byte(const U8 byte); 
U8 send_and_read_byte(const U8 byte);
void send_instruction(const U8 *instructList, size_t size, bool read=false);
void clock_gen();
void construct_read_write_instruction(U16 address, U8 data, U8* result, bool write=true);
int read_file(char buffer[MAX_CODE_LINES][MAX_CODE_SIZE_PER_LINE_BYTES], size_t maxSize);
void load_hexfile_to_8051(char hexFile[MAX_CODE_LINES][MAX_CODE_SIZE_PER_LINE_BYTES], int length);

// Inline functions (Quick Helper Functions)
inline U32 construct_32bit_number(INSTRUCTION instruct) { // Takes in a list of 4*8-bit, comines into 1*32bit
  return ((U32)instruct[0] << 24) | ((U32)instruct[1] << 16) | ((U32)instruct[2] << 8) | instruct[3];
}
inline U8 get_bits2B(char byte[HEX_BYTE_STRING_SIZE]) { // Converts a string to 1Byte hex; "2F" -> 0x2F;
      auto hex_to_int = [](char c) {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        return 0;
    };
    return hex_to_int(byte[0]) * 16 + hex_to_int(byte[1]);
}
inline U8 get_bits4B(char byte[HEX_BYTE_STRING_SIZE+2]) { // Converts a string to 2Byte hex; "2F2D" -> 0x2F2D;
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

// Sending handling
void send_byte(const U8 byte) {
  for (int i=0; i<INSTRUCT_SIZE_BITS; i++) {
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

// Clock emulation
void clock_gen() {
  digitalWrite(SCK, HIGH);
  delayMicroseconds(CLK_SPEED);
  digitalWrite(SCK, LOW);
  delayMicroseconds(CLK_SPEED);
}

// Instruction construction
void construct_read_write_instruction(U16 address, U8 data, U8* result, bool write) { // Combines required data for an 8051 prog memory write instruction;
  result[0] = write ? 0x40 : 0x40;
  result[1] = (address >> 8) & 0x1F;
  result[2] = address & 0xFF;
  result[3] = data;
}

// Hex file management
int read_file(char buffer[MAX_CODE_LINES][MAX_CODE_SIZE_PER_LINE_BYTES], size_t maxSize) { // Parses a hex file into a list of string (lines);

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
void load_hexfile_to_8051(char hexFile[MAX_CODE_LINES][MAX_CODE_SIZE_PER_LINE_BYTES], int length) { // Interprets a hex file and uploads to 8051

  for (int i=0; i<length; i++) {
    size_t index = 1; // Bypass first colon

    // 1. Data Count
    char data_count_string[HEX_BYTE_STRING_SIZE];
    data_count_string[0] = hexFile[i][index++];
    data_count_string[1] = hexFile[i][index++];

    U8 data_count = get_bits2B(data_count_string);

    Serial.print("Number of bytes: ");
    Serial.println(data_count);

    // 2. Address
    char address_string[HEX_BYTE_STRING_SIZE+2];
    address_string[0] = hexFile[i][index++];
    address_string[1] = hexFile[i][index++];
    address_string[2] = hexFile[i][index++];
    address_string[3] = hexFile[i][index++];

    U16 start_address = get_bits4B(address_string);

    // 2. Record
    char record_string[HEX_BYTE_STRING_SIZE];
    record_string[0] = hexFile[i][index++];
    record_string[1] = hexFile[i][index++];

    U8 record = get_bits2B(record_string);

    U16 addr = start_address;
    for (U8 j = 0; j < data_count; j++) {
      char data_byte_string[HEX_BYTE_STRING_SIZE];
      data_byte_string[0] = hexFile[i][index++];
      data_byte_string[1] = hexFile[i][index++];
      U8 data_byte = get_bits2B(data_byte_string);
      INSTRUCTION instruction;
      construct_read_write_instruction(addr++, data_byte, instruction);
      send_instruction(instruction, ISNTRUCT_BYTES);
    }

    Serial.println();
  }
  
  
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

  // Step 2, send programming enable
  send_instruction(PROG_EN, sizeof(PROG_EN), true);
  delay(DELAY);
  

  while (!Serial.available()); // Wait until hex file is being sent on serial port

  delay(MAX_WAIT_TIME_FOR_FILE_MS); // Makes sure the whole file is read
  lines = read_file(hex_file, MAX_CODE_SIZE_PER_LINE_BYTES);

  send_instruction(ERASE_CHIP, sizeof(ERASE_CHIP)); // Erase chip before uploading any code
  delay(DELAY);

  load_hexfile_to_8051(hex_file, lines);

  // Final step: RST -> LOW to allow normal operation
  digitalWrite(RST, LOW);
}

void loop(){}
