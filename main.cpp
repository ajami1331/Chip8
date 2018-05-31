#include <iostream>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <SFML/Graphics.hpp>

namespace emulator {

        const int       MEM_SIZE = 4096;
        const int       GFX_ROWS = 32;
        const int       GFX_COLS = 64;
        const int       STACK_SIZE = 16;
        const int       KEY_SIZE = 16;
        const int       MAX_GAME_SIZE = 0xE00;

        uint16_t        opcode;                 /// opcodes, 16bit.
        uint16_t        I;                      /// 16bit register (For memory address)
        uint16_t        PC;                     /// program counter
        uint16_t        stack[STACK_SIZE];      /// stack
        uint16_t        SP;                     /// stack pointer
        uint16_t        nnn;                    /// address


        uint8_t         x;                      /// 4-bit register identifier
        uint8_t         y;                      /// 4-bit register identifier
        uint8_t         n;                      /// 4-bit constant
        uint8_t         nn;                     /// 8 bit constant
        uint8_t         memory[MEM_SIZE];
        uint8_t         V[16];
        uint8_t         gfx[GFX_ROWS][GFX_COLS];
        uint8_t         delayTimer;
        uint8_t         soundTimer;
        uint8_t         key[KEY_SIZE];

        uint8_t chip8Fontset[80] =  {
                0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
                0x20, 0x60, 0x20, 0x20, 0x70, // 1
                0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
                0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
                0x90, 0x90, 0xF0, 0x10, 0x10, // 4
                0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
                0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
                0xF0, 0x10, 0x20, 0x40, 0x40, // 7
                0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
                0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
                0xF0, 0x90, 0xF0, 0x90, 0x90, // A
                0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
                0xF0, 0x80, 0x80, 0x80, 0xF0, // C
                0xE0, 0x90, 0x90, 0x90, 0xE0, // D
                0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
                0xF0, 0x80, 0xF0, 0x80, 0x80  // F
        };

        bool drawFlag;

        void UnknownOpcode( uint16_t op );

        void CpuNull();

        void OpcodeX0();
        void OpcodeX1();
        void OpcodeX2();
        void OpcodeX3();
        void OpcodeX4();
        void OpcodeX5();
        void OpcodeX6();
        void OpcodeX7();
        void OpcodeX8();
        void OpcodeX9();
        void OpcodeXA();
        void OpcodeXB();
        void OpcodeXC();
        void OpcodeXD();
        void OpcodeXE();
        void OpcodeXF();

        uint8_t GetRandom();

        void DrawSprite( uint8_t x, uint8_t y, uint8_t n );

        void Tick();

        void (*JumpTableA[16])() = {
                OpcodeX0, OpcodeX1, OpcodeX2, OpcodeX3,
                OpcodeX4, OpcodeX5, OpcodeX6, OpcodeX7,
                OpcodeX8, OpcodeX9, OpcodeXA, OpcodeXB,
                OpcodeXC, OpcodeXD, OpcodeXE, OpcodeXF
        };

        void Initialize() {

                PC      = 0x200;
                opcode  = 0;
                I       = 0;
                SP      = 0;

                memset( memory, 0, sizeof memory );
                memset( gfx,    0, sizeof gfx );
                memset( stack,  0, sizeof stack );
                memset( key,    0, sizeof key );

                for( int i=0; i<80; i++ ) {
                        memory[i] = chip8Fontset[i];
                }

                delayTimer = 0;
                soundTimer = 0;
                drawFlag = true;

                srand( time( nullptr ) );
        }

        void LoadRom( const std::string& filePath ) {
                FILE *fp;

                fp = fopen( filePath.c_str(), "rb" );

                if( fp == nullptr ) {
                        fprintf( stderr, "Can not open file.\n" );
                        exit( 1 );
                }

                fread( &memory[0x200], 1, MAX_GAME_SIZE, fp );

                fclose( fp );
        }

        void Cycle() {
                opcode = memory[PC] << 8 | memory[PC + 1];

                fprintf( stderr, "Ox%x\n", opcode );

                n       = opcode & 0x000F;
                nn      = opcode & 0x00FF;
                nnn     = opcode & 0x0FFF;
                x       = opcode & 0x0F00;
                y       = opcode & 0x00F0;

                JumpTableA[(opcode & 0xF000) >> 12]();
        }

        void CpuNull() {}

        void OpcodeX0() {
                switch (nn) {
                        case 0xE0:
                                fprintf( stderr, "clearing the screen\n" );
                                memset( gfx, 0, sizeof gfx );
                                drawFlag = true;
                                PC += 2;
                                break;
                        case 0xEE:
                                fprintf( stderr, "subroutine return\n" );
                                PC = stack[--SP];
                                break;
                        default:
                                UnknownOpcode( opcode );
                                break;
                }
        }

        void OpcodeX1() {
                fprintf( stderr, "jumping to nnn: 0x%x", nnn );
                PC = nnn;
        }

        void OpcodeX2() {
                fprintf( stderr, "call subroutine in nnn: 0x%x", nnn );
                stack[SP++] = PC + 2;
                PC = nnn;
        }

        void OpcodeX3() {
                fprintf( stderr, "skip next instruction if 0x%x == 0x%x\n", V[x], nn );
                PC += ( V[x] == nn ) ? 4 : 2;
        }

        void OpcodeX4() {
                fprintf( stderr, "skip next instruction if 0x%x != 0x%x\n", V[x], nn );
                PC += ( V[x] != nn ) ? 4 : 2;
        }

        void OpcodeX5() {
                fprintf( stderr, "skip next instruction if 0x%x != 0x%x\n", V[x], V[y] );
                PC += ( V[x] == V[y] ) ? 4 : 2;
        }

        void OpcodeX6() {
                fprintf( stderr, "set 0x%x = 0x%x\n", x, nn );
                V[x] = nn;
                PC += 2;
        }

        void OpcodeX7() {
                fprintf( stderr, "set 0x%x += 0x%x\n", x, nn );
                V[x] += nn;
                PC += 2;
        }

        void OpcodeX8() {
                switch (n) {
                        case 0x0:
                                V[x] = V[y];
                                break;
                        case 0x1:
                                V[x] |= V[y];
                                break;
                        case 0x2:
                                V[x] &= V[y];
                                break;
                        case 0x3:
                                V[x] ^= V[y];
                                break;
                        case 0x4:
                                V[0xF] = ( (int)V[x] + (int)V[y] ) > 255 ? 1 : 0;
                                V[x] += V[y];
                                break;
                        case 0x5:
                                V[0xF] = V[x] > V[y] ? 1 : 0;
                                V[x] -= V[y];
                                break;
                        case 0x6:
                                V[0xF] = V[y] & 1;
                                V[x] = V[y] >> 1;
                                break;
                        case 0x7:
                                V[0xF] = V[y] > V[x] ? 1 : 0;
                                V[x] = V[y] - V[x];
                                break;
                        case 0xE:
                                V[0xF] = ( V[y] >> 7 ) & 1;
                                V[y] <<= 1;
                                V[x] = V[y];
                                break;
                        default:
                                UnknownOpcode( opcode );
                                break;
                }
                PC += 2;
        }

        void OpcodeX9() {
                PC += ( V[x] != V[y] ) ? 4 : 2;
        }

        void OpcodeXA() {
                I = nnn;
                PC += 2;
        }

        void OpcodeXB() {
                PC = V[0] + nnn;
        }

        void OpcodeXC() {
                V[0] = GetRandom() & nn;
                PC += 2;
        }

        void OpcodeXD() {
                DrawSprite( V[x], V[y], n );
                drawFlag = true;
                PC += 2;
        }

        void OpcodeXE() {
                switch (nn) {
                        case 0x9E:
                                PC += ( key[ V[x] ] ) ? 4 : 2;
                                break;
                        case 0xA1:
                                PC += !( key[ V[x] ] ) ? 4 : 2;
                                break;
                        default:
                                UnknownOpcode( opcode );
                                break;
                }
        }

        void OpcodeXF() {
                switch (nn) {
                        case 0x07:
                                V[x] = delayTimer;
                                PC += 2;
                                break;
                        case 0x0A:
                                {bool gotKeyPress = false;
                                while( !gotKeyPress ) {
                                        for( int i=0; i<KEY_SIZE; i++ ) {
                                                if( key[i] ) {
                                                        V[x] = i;
                                                        gotKeyPress = true;
                                                        break;
                                                }
                                        }
                                }
                                PC += 2; }
                                break;
                        case 0x15:
                                delayTimer = V[x];
                                PC += 2;
                                break;
                        case 0x18:
                                soundTimer = V[x];
                                PC += 2;
                                break;
                        case 0x1E:
                                V[0xF] = ( I + V[x] ) > 0xFFF ? 1 : 0;
                                I += V[x];
                                PC += 2;
                                break;
                        case 0x29:
                                I = 5 * V[x];
                                PC += 2;
                                break;
                        case 0x33:
                                memory[I + 0] = ( V[x] % 1000 ) / 100;
                                memory[I + 1] = ( V[x] % 100 ) / 10;
                                memory[I + 2] = ( V[x] % 10 );
                                PC += 2;
                                break;
                        case 0x55:
                                for( int i=0; i<=x; i++ ) {
                                        memory[I + i] = V[i];
                                }
                                I += x + 1;
                                PC += 2;
                                break;
                        case 0x65:
                                for( int i=0; i<=x; i++ ) {
                                        V[i] = memory[I + i];
                                }
                                I += x + 1;
                                PC += 2;
                                break;
                        default:
                                UnknownOpcode( opcode );
                                break;
                }
        }

        void DrawSprite( uint8_t x, uint8_t y, uint8_t n ) {
                unsigned row = y, col = x;

                V[0xF] = 0;

                for( unsigned byteIndex = 0; byteIndex < n; byteIndex++) {

                        uint8_t byte = memory[I + byteIndex];

                        for( unsigned bitIndex = 0; bitIndex < 8; bitIndex++) {

                                uint8_t bit = (byte >> bitIndex) & 0x1;

                                uint8_t& pixelp = gfx[(row + byteIndex) % GFX_ROWS][(col + (7 - bitIndex)) % GFX_COLS];

                                if( bit == 1 && pixelp == 1 ) {
                                        V[0xF] = 1;
                                }

                                pixelp ^= bit;
                        }
                }
        }

        void DebugDraw() {
                for( int i=0; i<GFX_ROWS; i++ ) {
                        for( int j=0; j<GFX_COLS; j++ ) {
                                if( gfx[i][j] ) {
                                        fprintf( stderr, "0" );
                                }
                                else {
                                        fprintf( stderr, " " );
                                }
                        }
                        fprintf( stderr, "\n" );
                }
        }

        void Tick() {
                if( delayTimer > 0 ) {
                        delayTimer--;
                }
                if( soundTimer > 0 ) {
                        soundTimer--;
                        if( soundTimer == 0 ) {
                                fprintf( stderr, "BEEP\n" );
                        }
                }
        }

        void UnknownOpcode( uint16_t op ) {
                fprintf( stderr, "Unknown Opcode: 0x%x\n", op );
                fprintf( stderr, "nn: 0x%02x\n", nn );
        }

        void PrintState() {

                fprintf( stderr, "------------------------------------------------------------------\n" );

                fprintf( stderr, "V0: 0x%02x  V4: 0x%02x  V8: 0x%02x  VC: 0x%02x\n", V[0], V[4], V[8], V[12]  );
                fprintf( stderr, "V1: 0x%02x  V5: 0x%02x  V9: 0x%02x  VD: 0x%02x\n", V[1], V[5], V[9], V[13]  );
                fprintf( stderr, "V2: 0x%02x  V6: 0x%02x  VA: 0x%02x  VE: 0x%02x\n", V[2], V[6], V[10], V[14] );
                fprintf( stderr, "V3: 0x%02x  V7: 0x%02x  VB: 0x%02x  VF: 0x%02x\n", V[3], V[7], V[11], V[15] );

                fprintf( stderr, "PC: 0x%04x\n", PC );
                fprintf( stderr, "------------------------------------------------------------------\n" );

        }

        void CheckKeyboard() {
                key[0x0] = ( sf::Keyboard::isKeyPressed( sf::Keyboard::Num0 ) ) ? 1 : 0;
                key[0x1] = ( sf::Keyboard::isKeyPressed( sf::Keyboard::Num1 ) ) ? 1 : 0;
                key[0x2] = ( sf::Keyboard::isKeyPressed( sf::Keyboard::Num2 ) ) ? 1 : 0;
                key[0xC] = ( sf::Keyboard::isKeyPressed( sf::Keyboard::Num3 ) ) ? 1 : 0;

                key[0x4] = ( sf::Keyboard::isKeyPressed( sf::Keyboard::Q ) ) ? 1 : 0;
                key[0x5] = ( sf::Keyboard::isKeyPressed( sf::Keyboard::W ) ) ? 1 : 0;
                key[0x6] = ( sf::Keyboard::isKeyPressed( sf::Keyboard::E ) ) ? 1 : 0;
                key[0xD] = ( sf::Keyboard::isKeyPressed( sf::Keyboard::R ) ) ? 1 : 0;

                key[0x7] = ( sf::Keyboard::isKeyPressed( sf::Keyboard::A ) ) ? 1 : 0;
                key[0x8] = ( sf::Keyboard::isKeyPressed( sf::Keyboard::S ) ) ? 1 : 0;
                key[0x9] = ( sf::Keyboard::isKeyPressed( sf::Keyboard::D ) ) ? 1 : 0;
                key[0xE] = ( sf::Keyboard::isKeyPressed( sf::Keyboard::F ) ) ? 1 : 0;

                key[0xA] = ( sf::Keyboard::isKeyPressed( sf::Keyboard::Z ) ) ? 1 : 0;
                key[0x0] = ( sf::Keyboard::isKeyPressed( sf::Keyboard::X ) ) ? 1 : 0;
                key[0xB] = ( sf::Keyboard::isKeyPressed( sf::Keyboard::C ) ) ? 1 : 0;
                key[0xF] = ( sf::Keyboard::isKeyPressed( sf::Keyboard::V ) ) ? 1 : 0;
        }

        uint8_t GetRandom() {
                return rand();
        }
};

int main( int argc, char *argv[] ) {

        std::string filePath;

//        if( argc > 2 ) {
//                fprintf( stdout, "Usage:\n\tChip8.exe \"Rom\"\n" );
//                exit( EXIT_FAILURE );
//        }
//        else if( argc == 2 ){
//                filePath = argv[1];
//        }
//        else if( argc == 1 ) {
//                fprintf( stdout, "Path to the Rom:" );
//                getline( std::cin, filePath );
//        }

        filePath = "pong2.c8";

        emulator::Initialize();

        emulator::LoadRom( filePath );

        char c;

        emulator::PrintState();

        sf::RenderWindow window(sf::VideoMode(512, 256), "Chip8");

        sf::Event event;

        sf::Uint8 pixels[64 * 32 * 4];

        sf::Texture texture;
        texture.create(64, 32);

        sf::Sprite sprite(texture);

        sprite.setScale( 8.0f, 8.0f );

        while (window.isOpen()) {
                while (window.pollEvent(event)) {
                        if (event.type == sf::Event::Closed)
                                window.close();
                }

                emulator::CheckKeyboard();

                emulator::Cycle();

                emulator::PrintState();

                window.clear();

                if( emulator::drawFlag ) {

                        for( int i = 0,x=0,y=0; i < 64*32*4; i += 4 ) {
                                pixels[i + 0] = emulator::gfx[x][y] ? 255 : 0;
                                pixels[i + 1] = emulator::gfx[x][y] ? 255 : 0;
                                pixels[i + 2] = emulator::gfx[x][y] ? 255 : 0;
                                pixels[i + 3] = emulator::gfx[x][y] ? 255 : 0;
                                y++;
                                y %= 64;
                                x += ( y == 0 );
                        }

                        texture.update(pixels);

                        emulator::drawFlag = false;
                }


                window.draw(sprite);

                window.display();
        }

        return 0;
}

