#include <SDL.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MEMORY_SIZE 4096
#define REGISTER_COUNT 16
#define STACK_SIZE 16
#define KEYPAD_SIZE 16
#define DISPLAY_WIDTH 64
#define DISPLAY_HEIGHT 32

typedef struct {
  uint8_t memory[MEMORY_SIZE];
  uint8_t V[REGISTER_COUNT]; // CPU registers V0 to VF
  uint16_t I;                // Index register
  uint16_t pc;               // Program counter
  uint8_t delay_timer;       // Delay timer
  uint8_t sound_timer;       // Sound timer
  uint16_t stack[STACK_SIZE];
  uint16_t sp; // Stack pointer
  uint8_t keypad[KEYPAD_SIZE];
  uint8_t display[DISPLAY_WIDTH * DISPLAY_HEIGHT]; // 64x32 pixels
  uint16_t opcode;
} Chip8;

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;

void init_SDL() {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
    exit(1);
  }

  window = SDL_CreateWindow("Chippy CHIP-8 Emulator", SDL_WINDOWPOS_UNDEFINED,
                            SDL_WINDOWPOS_UNDEFINED, DISPLAY_WIDTH * 10,
                            DISPLAY_HEIGHT * 10, // Scaling up the window size
                            SDL_WINDOW_SHOWN);
  if (window == NULL) {
    printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
    exit(1);
  }

  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  if (renderer == NULL) {
    printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
    exit(1);
  }
}

void render_chip8_display(uint8_t *display) {
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderClear(renderer);

  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

  for (int y = 0; y < DISPLAY_HEIGHT; ++y) {
    for (int x = 0; x < DISPLAY_WIDTH; ++x) {
      if (display[y * DISPLAY_WIDTH + x]) {
        SDL_Rect pixel = {x * 10, y * 10, 10,
                          10}; // Each CHIP-8 pixel is scaled 10x10
        SDL_RenderFillRect(renderer, &pixel);
      }
    }
  }

  SDL_RenderPresent(renderer);
}

void handle_events(int *quit, Chip8 *chip8) {
  SDL_Event e;
  while (SDL_PollEvent(&e) != 0) {
    if (e.type == SDL_QUIT) {
      *quit = 1;
    }
    // Handle key press events
    if (e.type == SDL_KEYDOWN) {
      switch (e.key.keysym.sym) {
      case SDLK_ESCAPE:
        *quit = 1;
        break;
      case SDLK_0:
        chip8->keypad[0] = 1;
        break;
      case SDLK_1:
        chip8->keypad[1] = 1;
        break;
      case SDLK_2:
        chip8->keypad[2] = 1;
        break;
      case SDLK_3:
        chip8->keypad[3] = 1;
        break;
      case SDLK_4:
        chip8->keypad[4] = 1;
        break;
      case SDLK_5:
        chip8->keypad[5] = 1;
        break;
      case SDLK_6:
        chip8->keypad[6] = 1;
        break;
      case SDLK_7:
        chip8->keypad[7] = 1;
        break;
      case SDLK_8:
        chip8->keypad[8] = 1;
        break;
      case SDLK_9:
        chip8->keypad[9] = 1;
        break;
      case SDLK_a:
        chip8->keypad[10] = 1;
        break;
      case SDLK_b:
        chip8->keypad[11] = 1;
        break;
      case SDLK_c:
        chip8->keypad[12] = 1;
        break;
      case SDLK_d:
        chip8->keypad[13] = 1;
        break;
      case SDLK_e:
        chip8->keypad[14] = 1;
        break;
      case SDLK_f:
        chip8->keypad[15] = 1;
        break;
      default:
        break;
      }
    }
    // Handle key up events
    if (e.type == SDL_KEYUP) {
      switch (e.key.keysym.sym) {
      case SDLK_0:
        chip8->keypad[0] = 0;
        break;
      case SDLK_1:
        chip8->keypad[1] = 0;
        break;
      case SDLK_2:
        chip8->keypad[2] = 0;
        break;
      case SDLK_3:
        chip8->keypad[3] = 0;
        break;
      case SDLK_4:
        chip8->keypad[4] = 0;
        break;
      case SDLK_5:
        chip8->keypad[5] = 0;
        break;
      case SDLK_6:
        chip8->keypad[6] = 0;
        break;
      case SDLK_7:
        chip8->keypad[7] = 0;
        break;
      case SDLK_8:
        chip8->keypad[8] = 0;
        break;
      case SDLK_9:
        chip8->keypad[9] = 0;
        break;
      case SDLK_a:
        chip8->keypad[10] = 0;
        break;
      case SDLK_b:
        chip8->keypad[11] = 0;
        break;
      case SDLK_c:
        chip8->keypad[12] = 0;
        break;
      case SDLK_d:
        chip8->keypad[13] = 0;
        break;
      case SDLK_e:
        chip8->keypad[14] = 0;
        break;
      case SDLK_f:
        chip8->keypad[15] = 0;
        break;
      default:
        break;
      }
    }
  }
}

void initialize(Chip8 *chip8) {
  chip8->pc = 0x200; // Program counter starts at 0x200
  chip8->opcode = 0;
  chip8->I = 0;
  chip8->sp = 0;

  // Clear display, stack, registers, and memory
  memset(chip8->display, 0, sizeof(chip8->display));
  memset(chip8->stack, 0, sizeof(chip8->stack));
  memset(chip8->V, 0, sizeof(chip8->V));
  memset(chip8->memory, 0, sizeof(chip8->memory));

  // Load font set into memory
  uint8_t chip8_fontset[80] = {
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

  for (int i = 0; i < 80; ++i) {
    chip8->memory[i] = chip8_fontset[i];
  }

  // Reset timers
  chip8->delay_timer = 0;
  chip8->sound_timer = 0;
}

void load_program(Chip8 *chip8, const char *filename) {
  FILE *file = fopen(filename, "rb");
  if (file == NULL) {
    fprintf(stderr, "Failed to open ROM: %s\n", filename);
    exit(1);
  }

  fseek(file, 0, SEEK_END);
  long size = ftell(file);
  rewind(file);

  if (size > (MEMORY_SIZE - 0x200)) {
    fprintf(stderr, "ROM too large to fit in memory\n");
    exit(1);
  }

  fread(&chip8->memory[0x200], sizeof(uint8_t), size, file);
  fclose(file);
}

#define PC(c8) ((c8)->opcode)
#define X(c8) (((c8)->opcode & 0x0F00) >> 8)
#define Y(c8) (((c8)->opcode & 0x00F0) >> 4)
#define N(c8) (((c8)->opcode & 0x000F))
#define KK(c8) (((c8)->opcode & 0xFF))
#define V(c8, i) (((c8)->V[i]))

void emulate_cycle(Chip8 *chip8) {
  // Fetch the next opcode
  chip8->opcode = chip8->memory[chip8->pc] << 8 | chip8->memory[chip8->pc + 1];
  printf("opcode: %x -- ", chip8->opcode);

  // Decode and execute opcode
  switch (chip8->opcode & 0xF000) {
  case 0x0000:
    switch (chip8->opcode & 0x00FF) {
    case 0x00E0: // 0x00E0: Clears the screen
      memset(chip8->display, 0, sizeof(chip8->display));
      chip8->pc += 2;
      printf("cleared screen\n");
      break;
    case 0x00EE: // 0x00EE: Returns from subroutine
      chip8->pc = chip8->stack[chip8->sp];
      --chip8->sp;
      chip8->pc += 2;
      break;

    default:
      printf("Unknown opcode [0x0000]: 0x%X\n", chip8->opcode);
    }
    break;
  case 0x1000: // 0x1NNN: Jumps to address NNN
    chip8->pc = chip8->opcode & 0x0FFF;
    printf("jumped to address\n");
    break;
  case 0x2000: // Call subroutine at NNN
    chip8->sp++;
    chip8->stack[chip8->sp] = chip8->pc;
    chip8->pc = chip8->opcode & 0x0FFF;
    printf("called subroutine at nnn\n");
    break;
  case 0x3000: // 3xkk skip nexty instruction if Vx=kk
    if (chip8->V[X(chip8)] == KK(chip8)) {
      chip8->pc += 2;
    }
    chip8->pc += 2;
    printf("skip instruction\n");
    break;
  case 0x4000: // 4xkk skip next instruction if Vx!=kk
    if (chip8->V[X(chip8)] != KK(chip8)) {
      chip8->pc += 2;
    }
    chip8->pc += 2;
    printf("skip instruction if not equal\n");
    break;
  case 0x5000: // 5xy0 skip next instruction if Vx==Vy
    if (chip8->V[X(chip8)] == chip8->V[Y(chip8)]) {
      chip8->pc += 2;
    }
    chip8->pc += 2;
    printf("skip instruction Vx==Vy\n");
    break;
  case 0x6000: // 0x6xkk: put kk in Vx
    chip8->V[X(chip8)] = chip8->opcode & 0x00FF;
    chip8->pc += 2;
    printf("put kk in Vx\n");
    printf("%x\n", chip8->V[X(chip8)]);
    break;
  case 0x7000: // 7xkk
    chip8->V[X(chip8)] += KK(chip8);
    chip8->pc += 2;
    printf("add Vx\n");
    break;
  case 0x8000:
    switch (chip8->opcode & 0x000F) {
    case 0x0000: // 8xy0 set Vx = Vy
      V(chip8, X(chip8)) = V(chip8, Y(chip8));
      chip8->pc += 2;
      printf("set Vx = Vy\n");
      break;
    case 0x0001: // 8xy1 set Vx | Vy
      V(chip8, X(chip8)) |= V(chip8, Y(chip8));
      chip8->pc += 2;
      printf("set Vx | Vy\n");
      break;
    case 0x0002: // 8xy2 set Vx & Vy
      V(chip8, X(chip8)) &= V(chip8, Y(chip8));
      chip8->pc += 2;
      printf("set Vx & Vy\n");
      break;
    case 0x0003: // 8xy3 set Vx XOR Vy
      V(chip8, X(chip8)) ^= V(chip8, Y(chip8));
      chip8->pc += 2;
      printf("set Vx ^ Vy\n");
      break;
    case 0x0004: // 8xy4 set Vx add Vy
      V(chip8, X(chip8)) += V(chip8, Y(chip8));
      chip8->pc += 2;
      printf("set Vx + Vy\n");
      break;
    case 0x0005: // 8xy5 set Vx sub Vy
      V(chip8, X(chip8)) -= V(chip8, Y(chip8));
      chip8->pc += 2;
      printf("set Vx - Vy\n");
      break;
    case 0x0006: // 8xy6 set Vx shr Vy
      V(chip8, X(chip8)) = V(chip8, X(chip8)) >> 1;
      chip8->pc += 2;
      printf("set Vx shift right\n");
      break;
    case 0x0007: // 8xy7 set Vx subn Vy
      V(chip8, X(chip8)) = V(chip8, Y(chip8)) - V(chip8, X(chip8));
      chip8->pc += 2;
      printf("set Vy - Vx\n");
      break;
    case 0x000E: // 8xyE set Vx shl
      V(chip8, X(chip8)) = V(chip8, X(chip8)) << 1;
      chip8->pc += 2;
      printf("set Vx shifted left\n");
      break;
    default: // 8xyE set Vx shl
      printf("code not implemented in 8!!!!\n");
      break;
    }
    break;
  case 0x9000: // 9xy0 SNE if Vx != Vy
    if (V(chip8, X(chip8)) != V(chip8, Y(chip8))) {
      chip8->pc += 2;
    }
    chip8->pc += 2;
    printf("skipping instr 9000\n");
    break;
  case 0xA000: // Set I to nnn
    chip8->I = chip8->opcode & 0x0FFF;
    chip8->pc += 2;
    printf("set I to nnn\n");
    break;
  case 0xB000: // jp to nnn + v0
    chip8->pc = (chip8->opcode & 0x0FFF) + V(chip8, 0);
    printf("jump to nnn + v0\n");
    break;
  case 0xC000: // jp to nnn + v0
    chip8->pc += 2;
    printf("random Vx - NOT IMPLEMENTED !!!\n");
    break;
  case 0xD000: // Dxyn - draw n-byte sprite starting at location I at (Vx,Vy)
    printf("x: %x, y: %x, n: %x\n", V(chip8, X(chip8)), V(chip8, Y(chip8)),
           N(chip8));
    for (int i = 0; i < N(chip8); i++) {
      for (int j = 0; j < 8; j++) {
        // printf("x: %x, y: %x, i: %x, j: %x\n", X(chip8), Y(chip8), i, j);
        chip8->display[((i + V(chip8, Y(chip8))) * DISPLAY_WIDTH) +
                       V(chip8, X(chip8)) + j] ^=
            (chip8->memory[chip8->I + i] >> (8 - 1 - j)) & 0x1;
      }
    }

    chip8->pc += 2;
    printf("wrote to display\n");
    break;
  case 0xE000:
    switch (chip8->opcode & 0x00FF) {
    case 0x009E:
      if (chip8->keypad[V(chip8, X(chip8))] == 1) {
        chip8->pc += 2;
      }
      chip8->pc += 2;
      break;
    case 0x00A1:
      if (chip8->keypad[V(chip8, X(chip8))] == 0) {
        chip8->pc += 2;
      }
      chip8->pc += 2;
      break;
    default:
      printf("not implemented!!!\n");
      break;
    }
    break;
  case 0xF000:
    switch (chip8->opcode & 0x00FF) {
    case 0x001E:
      chip8->I += V(chip8, X(chip8));
      printf("added Vx to I\n");
      chip8->pc += 2;
      break;
    case 0x0007:
      chip8->V[X(chip8)] = chip8->delay_timer;
      printf("set Vx from delay timer\n");
      chip8->pc += 2;
      break;
    case 0x0015:
      chip8->delay_timer = V(chip8, X(chip8));
      printf("set delay timer\n");
      chip8->pc += 2;
      break;
    case 0x0033:
      chip8->memory[chip8->I] = (int)(V(chip8, X(chip8)) / 100);
      chip8->memory[chip8->I + 1] = (int)(V(chip8, X(chip8)) % 100) / 10;
      chip8->memory[chip8->I + 2] = (int)((V(chip8, X(chip8)) % 10));
      chip8->pc += 2;
      printf("stored decimal places\n");
      break;
    case 0x0055:
      for (int i = 0; i <= X(chip8); i++) {
        chip8->memory[chip8->I + i] = chip8->V[i];
      }
      chip8->pc += 2;
      printf("wrote mem out from Vx\n");
      break;
    case 0x0065:
      for (int i = 0; i <= X(chip8); i++) {
        chip8->V[i] = chip8->memory[chip8->I + i];
      }
      chip8->pc += 2;
      printf("read mem into Vx\n");
      break;
    default:
      printf("not implemented!!!\n");
      break;
    }
    break;
  default:
    printf("Unknown opcode: 0x%X\n", chip8->opcode);
  }

  // Update timers
  if (chip8->delay_timer > 0)
    --chip8->delay_timer;

  if (chip8->sound_timer > 0) {
    if (chip8->sound_timer == 1)
      printf("BEEP!\n");
    --chip8->sound_timer;
  }
}

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("Usage: %s <ROM file>\n", argv[0]);
    return 1;
  }

  Chip8 chip8;
  initialize(&chip8);
  load_program(&chip8, argv[1]);
  init_SDL();

  int quit = 0;
  while (!quit) {
    emulate_cycle(&chip8);
    handle_events(&quit, &chip8);
    render_chip8_display(chip8.display);
    SDL_Delay(16); // Delay to control speed, roughly 60 frames per second
  }

  return 0;
}
