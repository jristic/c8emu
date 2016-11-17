
void emu_init(unsigned char* rom, int rom_size, GLuint* display_tex);
bool emu_update(bool* keys);
void emu_render(GLuint display_tex);