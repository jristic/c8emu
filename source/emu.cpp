
byte ram[4096] = {};

uchar V[16] = {};
ushort I = 0;

ushort stack[16] = {};
uchar sp = 0;

ushort pc = 0;

byte screen[64*32] = {};

bool keys[16] = {};

uchar DT = 0;
uchar ST = 0;

int gFramesToSim = 0;
int gUpdateTargetAddress = false;
int gTargetAddress = 0x200;

GLuint display_tex;
GLuint quad_shader;
GLuint quad_vertexbuffer;
GLuint quad_vertexobject;

// See http://devernay.free.fr/hacks/chip8/C8TECH10.HTM#font for font codes
#define FONT_SPRITE_BASE_ADDRESS 0x0
byte font_sprite[] = {
	0xF0, 0x90,	0x90, 0x90,	0xF0, // "0"
	0x20, 0x60, 0x20, 0x20,	0x70, // "1"
	0xF0, 0x10,	0xF0, 0x80,	0xF0, // "2"
	0xF0, 0x10,	0xF0, 0x10, 0xF0, // "3"
	0x90, 0x90, 0xF0, 0x10,	0x10, // "4"
	0xF0, 0x80, 0xF0, 0x10, 0xF0, // "5"
	0xF0, 0x80, 0xF0, 0x90, 0xF0, // "6"
	0xF0, 0x10, 0x20, 0x40, 0x40, // "7"
	0xF0, 0x90, 0xF0, 0x90, 0xF0, // "8"
	0xF0, 0x90, 0xF0, 0x10, 0xF0, // "9"
	0xF0, 0x90, 0xF0, 0x90, 0x90, // "A"
	0xE0, 0x90, 0xE0, 0x90, 0xE0, // "B"
	0xF0, 0x80, 0x80, 0x80, 0xF0, // "C"
	0xE0, 0x90, 0x90, 0x90, 0xE0, // "D"
	0xF0, 0x80, 0xF0, 0x80, 0xF0, // "E"
	0xF0, 0x80, 0xF0, 0x80, 0x80, // "F"
};

bool SPrintInstr(int op, char* buf)
{
	int op_category = op >> 12;
	switch(op_category)
	{
	case 0:
	{
		if (op == 0x00E0)
			SPrint(buf, "CLS");
		else if (op == 0x00EE)
			SPrint(buf, "RET");
		else
			SPrint(buf, "NOP");
		break;
	}
	case 0x1:
	{
		ushort addr = (op & 0xFFF);
		SPrint(buf, "JP 0x%X", addr);
		break;
	}
	case 0x2:
	{
		ushort addr = (op & 0xFFF);
		SPrint(buf, "CALL 0x%X", addr);
		break;
	}
	case 0x3:
	{
		ushort k = (op & 0xFF);
		uchar vx = (op >> 8) & 0xF;
		SPrint(buf, "SE V%X, 0x%X", vx, k);
		break;
	}
	case 0x4:
	{
		ushort k = (op & 0xFF);
		uchar vx = (op >> 8) & 0xF;
		SPrint(buf, "SNE V%X, 0x%X", vx, k);
		break;
	}
	case 0x5:
	{
		// Assert((op & 0xF) == 0, "what is even going on");
		uchar vy = (op >> 4) & 0xF;
		uchar vx = (op >> 8) & 0xF;
		SPrint(buf, "SE V%X, V%X", vx, vy);
		break;
	}
	case 0x6:
	{
		ushort literal = (op & 0xFF);
		uchar reg = (op >> 8) & 0xF;
		SPrint(buf, "LD V%X, 0x%X", reg, literal);
		break;
	}
	case 0x7:
	{
		ushort k = (op & 0xFF);
		uchar vx = (op >> 8) & 0xF;
		SPrint(buf, "ADD V%X, 0x%X", vx, k);
		break;
	}
	case 0x8:
	{
		uchar sub_op = (op & 0xF);
		uchar vy = (op >> 4) & 0xF;
		uchar vx = (op >> 8) & 0xF;
		switch (sub_op)
		{
		case 0x0:
			SPrint(buf, "LD V%X, V%X", vx, vy);
			break;
		case 0x1:
			SPrint(buf, "OR V%X, V%X", vx, vy);
			break;
		case 0x2:
			SPrint(buf, "AND V%X, V%X", vx, vy);
			break;
		case 0x3:
			SPrint(buf, "XOR V%X, V%X", vx, vy);
			break;
		case 0x4:
			SPrint(buf, "ADD V%X, V%X", vx, vy);
			break;
		case 0x5:
			SPrint(buf, "SUB V%X, V%X", vx, vy);
			break;
		case 0x6:
			SPrint(buf, "SHR V%X {, V%X}", vx, vy);
			break;
		case 0x7:
			SPrint(buf, "SUBN V%X, V%X", vx, vy);
			break;
		case 0xE:
			SPrint(buf, "SHL V%X{, V%X}", vx, vy);
			break;
		default:
			return false;
		}
		break;
	}
	case 0x9:
	{
		if ((op & 0xF) != 0)
			return false;
		uchar vy = (op >> 4) & 0xF;
		uchar vx = (op >> 8) & 0xF;
		SPrint(buf, "SNE V%X, V%X", vx, vy);
		break;
	}
	case 0xA:
	{
		ushort literal = (op & 0xFFF);
		SPrint(buf, "LD I, 0x%X", literal);
		break;
	}
	case 0xB:
	{
		ushort addr = (op & 0xFF);
		SPrint(buf, "JP V0, 0x%X", addr);
		break;
	}
	case 0xC:
	{
		ushort n = (op & 0xFF);
		uchar vx = (op >> 8) & 0xF;
		SPrint(buf, "RND V%X, 0x%X", vx, n);
		break;
	}
	case 0xD:
	{
		uchar n = (op & 0xF);
		uchar vy = (op >> 4) & 0xF;
		uchar vx = (op >> 8) & 0xF;
		SPrint(buf, "DRW V%X, V%X, 0x%X", vx, vy, n);
		break;
	}
	case 0xE:
	{	
		ushort subop = op & 0xFF;
		switch (subop)
		{
		case 0x9E:
		{
			uchar vx = (op >> 8) & 0xF;
			SPrint(buf, "SKP V%X", vx);
			break;
		}
		case 0xA1:
		{
			uchar vx = (op >> 8) & 0xF;
			SPrint(buf, "SKNP V%X", vx);
			break;
		}
		default:
			return false;
		}
		break;
	}
	case 0xF:
	{
		ushort subop = op & 0xFF;
		uchar vx = (op >> 8) & 0xF;
		switch(subop)
		{
		case 0x07:
			SPrint(buf, "LD V%X, DT", vx);
			break;
		case 0x0A:
			SPrint(buf, "LD V%X, K", vx);
			break;
		case 0x15:
			SPrint(buf, "LD DT, V%X", vx);
			break;
		case 0x18:
			SPrint(buf, "LD ST, V%X", vx);
			break;
		case 0x1E:
			SPrint(buf, "ADD I, V%X", vx);
			break;
		case 0x29:
			SPrint(buf, "LD F, V%X", vx);
			break;
		case 0x33:
			SPrint(buf, "LD B, V%X", vx);
			break;
		case 0x55:
			SPrint(buf, "LD [I], V%X", vx);
			break;
		case 0x65:
			SPrint(buf, "LD V%X, [I]", vx);
			break;
		default:
			return false;
		}
		break;
	}
	default:
		return false;
	}
	
	return true;
}

bool PrintInstr(int op)
{
	char buf[1024];
	bool valid = SPrintInstr(op, buf);
	
	if (valid)
		Print("%s", buf);
	else
		Print("");
	
	return valid;
}

void emu_init(unsigned char* rom, int rom_size)
{
	// // Print ROM contents for debug purposes
	// int num_ops = rom_size / 2;
	// for (int i = 0 ; i < num_ops ; ++i)
	// {
	// 	int op = (rom[2*i] << 8) | rom[2*i+1];
	// 	Printnln("0x%.3x: %.4x = ", 0x200 + 2*i, op);
	// 	PrintInstr(op);
	// }
	
	// Copy font sprites into memory
	memcpy(ram, font_sprite + FONT_SPRITE_BASE_ADDRESS, sizeof(font_sprite));

	// Copy ROM into memory
	for (int i = 0 ; i < rom_size ; ++i)
	{
		ram[0x200+i] = rom[i];
	}
	// Initialize PC to start of ROM location
	pc = 0x200;
	
	
	/* ------------------ Setup opengl resources ------------------ */
	// Create and compile our GLSL program from the shaders
	std::string errors;
	bool success = make_shader_program("shader/quad.vert", "shader/quad.frag",
		quad_shader, errors);
	Assert(success, "failed to compile quad shader: %s", errors.c_str());
	
	glGenTextures(1, &display_tex);
	glBindTexture(GL_TEXTURE_2D, display_tex);
	// Give an empty image to OpenGL ( the last "0" )
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 64, 32, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	// Use box filter
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	
	// Setup vertex buffers and shader for rendering texture to screen
	const GLfloat g_quad_vertex_buffer_data[] = {
		-1.0f, -1.0f,
		3.0f, -1.0f,
		-1.0f,  3.0f,
	};
	glGenBuffers(1, &quad_vertexbuffer);
	glGenVertexArrays(1, &quad_vertexobject);
	glBindVertexArray(quad_vertexobject);
	glBindBuffer(GL_ARRAY_BUFFER, quad_vertexbuffer);
	GLuint g_AttribLocationPosition = glGetAttribLocation(quad_shader, "Position");
	glEnableVertexAttribArray(g_AttribLocationPosition);
	glVertexAttribPointer(g_AttribLocationPosition, 2, GL_FLOAT, GL_FALSE, 8, 0);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_quad_vertex_buffer_data),
		g_quad_vertex_buffer_data, GL_STATIC_DRAW);
	
	GLuint tex_id = glGetUniformLocation(quad_shader, "inputTex");
	glUseProgram(quad_shader);
	// Set our "inputTex" sampler to user Texture Unit 0
	glUniform1i(tex_id, 0);
	glUseProgram(0);
}

bool emu_sim_step(int tick)
{
	if (gFramesToSim == 0)
	{
		return true;
	}
	else if (gFramesToSim > 0)
	{
		--gFramesToSim;
	}
	
	// Update the timers if we've ticked
	static int last_tick = tick;
	if (tick != last_tick)
	{
		if (DT > 0)
			--DT;
		// TODO: sound for the sound timer
		if (ST > 0)
			--ST;
		
		last_tick = tick;
	}
	
	int op = (ram[pc] << 8) | ram[pc+1];
	int op_category = op >> 12;
	
	switch(op_category)
	{
	case 0:
	{
		if (op == 0x00E0)
		{
			// CLS
			for (int y = 0 ; y < 32 ; ++y)
			for (int x = 0 ; x < 64 ; ++x)
				screen[64*y + x] = 0;
			pc+=2;
		}
		else if (op == 0x00EE)
		{
			// RET
			Assert(sp > 0, "unmatched return")
			pc = stack[--sp];
			pc+=2;
		}
		else
			Assert(false, "invalid op doofus: 0x%X, pc=%X", op, pc);
		break;
	}
	case 0x1:
	{
		// JP addr
		ushort addr = (op & 0xFFF);
		pc = addr;
		break;
	}
	case 0x2:
	{
		// CALL addr
		ushort addr = (op & 0xFFF);
		Assert(sp < 15, "stack overflow incoming");
		stack[sp] = pc;
		++sp;
		pc = addr;
		break;
	}
	case 0x3:
	{
		// SE Vx, k
		ushort k = (op & 0xFF);
		byte vx = (op >> 8) & 0xF;
		if (V[vx] == k)
			pc+=2;
		pc+=2;
		break;
	}
	case 0x4:
	{
		// SNE Vx, k
		ushort k = (op & 0xFF);
		byte vx = (op >> 8) & 0xF;
		if (V[vx] != k)
			pc+=2;
		pc+=2;
		break;
	}
	case 0x5:
	{
		// SE Vx, Vy
		Assert((op & 0xF) == 0, "what is even going on");
		byte vy = (op >> 4) & 0xF;
		byte vx = (op >> 8) & 0xF;
		if (V[vx] == V[vy])
			pc+=2;
		pc+=2;
		break;
	}
	case 0x6:
	{
		// LD Vx, k
		byte k = (op & 0xFF);
		byte vx = (op >> 8) & 0xF;
		V[vx] = k;
		pc+=2;
		break;
	}
	case 0x7:
	{
		// ADD Vx, k
		byte k = (op & 0xFF);
		byte vx = (op >> 8) & 0xF;
		V[vx] += k;
		pc+=2;
		break;
	}
	case 0x8:
	{
		byte sub_op = (op & 0xF);
		byte vy = (op >> 4) & 0xF;
		byte vx = (op >> 8) & 0xF;
		switch (sub_op)
		{
		case 0x0:
			// LD Vx, Vy
			V[vx] = V[vy];
			break;
		case 0x1:
			// OR Vx, Vy
			V[vx] = V[vx] | V[vy];
			break;
		case 0x2:
			// AND Vx, Vy
			V[vx] = V[vx] & V[vy];
			break;
		case 0x3:
			// XOR Vx, Vy
			V[vx] = V[vx] ^ V[vy];
			break;
		case 0x4:
		{
			// ADD Vx, Vy
			ushort res = V[vx] + V[vy];
			V[0xF] = (res > 0xFF) ? 1 : 0;
			V[vx] = res & 0xFF;
			break;
		}
		case 0x5:
		{
			// SUB Vx, Vy
			V[0xF] = (V[vx] > V[vy]) ? 1 : 0;
			V[vx] = V[vx] - V[vy];
			break;
		}
		case 0x6:
		{
			// SHR Vx {, Vy}  -- Vy unused
			V[0xF] = (V[vx] & 0x1) ? 1 : 0;
			V[vx] = V[vx] >> 1;
			break;
		}
		case 0x7:
		{
			// SUBN Vx, Vy
			V[0xF] = (V[vy] > V[vx]) ? 1 : 0;
			V[vx] = V[vy] - V[vx];
			break;
		}
		case 0xE:
		{
			// SHR Vx {, Vy}  -- Vy unused
			V[0xF] = (V[vx] & 0x80) ? 1 : 0;
			V[vx] = V[vx] << 2;
			break;
		}
		default:
			Assert(false, "unimplemented op");
			break;
		}
		pc += 2;
		break;
	}
	case 0x9:
	{
		// SNE Vy, Vy
		Assert((op & 0xF) == 0, "what is even going on");
		byte vy = (op >> 4) & 0xF;
		byte vx = (op >> 8) & 0xF;
		if (V[vx] != V[vy])
			pc+=2;
		pc+=2;
		break;
	}
	case 0xA:
	{
		// LD I, k
		ushort k = (op & 0xFFF);
		I = k;
		pc+=2;
		break;
	}
	case 0xB:
	{
		// JP V0, addr
		byte addr = (op & 0xFF);
		pc = V[0x0] + addr;
		break;
	}
	case 0xC:
	{
		// RND Vx, k
		byte k = (op & 0xFF);
		byte vx = (op >> 8) & 0xF;
		V[vx] = (rand() % 256) & k;
		pc+=2;
		break;
	}
	case 0xD:
	{
		// DRW Vx, Vy, n
		byte n = (op & 0xF);
		byte vy = (op >> 4) & 0xF;
		byte vx = (op >> 8) & 0xF;
		byte carry = 0;
		for (int y = 0 ; y < n ; ++y)
		{
			byte bits = ram[I + y];
			int ty = (V[vy] + y) % 32;
			for (int x = 0 ; x < 8 ; ++x)
			{
				int tx = (V[vx] + x) % 64;
				byte screen_val = screen[64*ty + tx];
				byte sprite_val = (bits >> (7 - x)) & 0x1;
				if (screen_val && sprite_val)
					carry = 1;
				screen[64*ty + tx] = screen_val ^ sprite_val;
			}
		}
		V[0xf] = carry;
		pc+=2;
		break;
	}
	case 0xE:
	{
		ushort subop = op & 0xFF;
		uchar vx = (op >> 8) & 0xF;
		switch (subop)
		{
		case 0x9E:
			if (keys[V[vx]])
				pc+=2;
			break;
		case 0xA1:
				
		default:
			Assert(false, "unimplemented command subop=0x%x, V%x", subop, vx);
		}
		pc+=2;
		break;
	}
	case 0xF:
	{
		ushort subop = op & 0xFF;
		uchar vx = (op >> 8) & 0xF;
		switch(subop)
		{
		case 0x07:
			// LD Vx, DT
			V[vx] = DT;
			pc+=2;
			break;
		case 0x0A:
			// LD Vx, K
			for (byte i = 0 ; i < 0x10 ; ++i)
			{
				if (keys[i])
				{
					V[vx] = i;
					pc+=2;
					break;
				}
			}
			break;
		case 0x15:
			DT = V[vx];
			pc+=2;
			break;
		case 0x18:
			ST = V[vx];
			pc+=2;
			break;
		case 0x1E:
			// ADD I, Vx
			I = I + V[vx];
			pc+=2;
			break;
		case 0x29:
			// LD F, Vx
			I = FONT_SPRITE_BASE_ADDRESS + V[vx] * 5;
			pc+=2;
			break;
		case 0x33:
			// LD B, Vx
			V[I] = V[vx] / 100;
			V[I] = (V[vx] % 100) / 10;
			V[I] = V[vx] % 10;
			pc+=2;
			break;
		case 0x55:
			// LD [I], Vx
			for (byte v = 0 ; v <= vx ; ++v)
			{
				ram[I + v] = V[v];
			}
			pc+=2;
			break;
		case 0x65:
			// LD Vx, [I]
			for (byte v = 0 ; v <= vx ; ++v)
			{
				V[v] = ram[I + v];
			}
			pc+=2;
			break;
		default:
			Assert(false, "unimplemented op 0x%x", subop);
			break;
		}
		break;
	}
	default:
		Assert(false, "unimplemented command");
		break;
	}
	
	if (gUpdateTargetAddress)
	{
		gTargetAddress = pc;
		gUpdateTargetAddress = false;
	}
	
	return false;
}

void emu_update()
{
	bool display_has_focus = false;
	
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImColor(IM_COL32_BLACK));
	ImGui::Begin("Display", nullptr,
		ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse);
	{
		display_has_focus = ImGui::IsWindowFocused();
		
		ImVec2 canvas_size = ImGui::GetContentRegionAvail();
		canvas_size.x = max(floor(canvas_size.x / 64), 2) * 64;
		canvas_size.y = canvas_size.x / 2;
		ImGui::Image((ImTextureID)display_tex, canvas_size);
	}
	ImGui::End();
	
	ImGuiIO& io = ImGui::GetIO();
	if (display_has_focus)
	{
		keys[0x1] = io.KeysDown[GLFW_KEY_1];
		keys[0x2] = io.KeysDown[GLFW_KEY_2];
		keys[0x3] = io.KeysDown[GLFW_KEY_3];
		keys[0xC] = io.KeysDown[GLFW_KEY_4];
		keys[0x4] = io.KeysDown[GLFW_KEY_Q];
		keys[0x5] = io.KeysDown[GLFW_KEY_W];
		keys[0x6] = io.KeysDown[GLFW_KEY_E];
		keys[0xD] = io.KeysDown[GLFW_KEY_R];
		keys[0x7] = io.KeysDown[GLFW_KEY_A];
		keys[0x8] = io.KeysDown[GLFW_KEY_S];
		keys[0x9] = io.KeysDown[GLFW_KEY_D];
		keys[0xE] = io.KeysDown[GLFW_KEY_F];
		keys[0xA] = io.KeysDown[GLFW_KEY_Z];
		keys[0x0] = io.KeysDown[GLFW_KEY_X];
		keys[0xB] = io.KeysDown[GLFW_KEY_C];
		keys[0xF] = io.KeysDown[GLFW_KEY_V];
	}
	
	ImGui::Begin("Keys", nullptr,
		ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);
	{
		static bool key_repeat = false;
		
		ImVec2 canvas_size = ImGui::GetContentRegionAvail();
		#define KEY_BUTTON(key)	{										\
			bool old_key_val = keys[0x##key];							\
			if (old_key_val) 											\
				ImGui::PushStyleColor(ImGuiCol_Button, 					\
					ImColor(IM_COL32_WHITE));							\
			if (!key_repeat) {											\
				if (ImGui::Button(#key)) keys[0x##key] = 1;				\
			} else {													\
				ImGui::Button(#key);									\
				if (ImGui::IsItemActive()) keys[0x##key] = 1;			\
			}															\
			if (old_key_val) ImGui::PopStyleColor();					\
		}
		
		// Line 1
		KEY_BUTTON(1);
		ImGui::SameLine();
		KEY_BUTTON(2);
		ImGui::SameLine();
		KEY_BUTTON(3);
		ImGui::SameLine();
		KEY_BUTTON(C);
		// Line 2
		KEY_BUTTON(4);
		ImGui::SameLine();
		KEY_BUTTON(5);
		ImGui::SameLine();
		KEY_BUTTON(6);
		ImGui::SameLine();
		KEY_BUTTON(D);
		// Line 3
		KEY_BUTTON(7);
		ImGui::SameLine();
		KEY_BUTTON(8);
		ImGui::SameLine();
		KEY_BUTTON(9);
		ImGui::SameLine();
		KEY_BUTTON(E);
		// Line 4
		KEY_BUTTON(A);
		ImGui::SameLine();
		KEY_BUTTON(0);
		ImGui::SameLine();
		KEY_BUTTON(B);
		ImGui::SameLine();
		KEY_BUTTON(F);
		
		ImGui::Checkbox("Repeat", &key_repeat);
	}
	ImGui::End();
	
	ImGui::Begin("Instruction View", nullptr, 0);
	{
		static int last_target_address = gTargetAddress;
		bool update_address = last_target_address != gTargetAddress;
		last_target_address = gTargetAddress;
		
		ImGui::BeginChild("mem", ImGui::GetContentRegionAvail(), false, 0);
		{
			int start_address = 0x200 + (gTargetAddress & 0x1);
			for (int i = start_address ; i < 0x600 - 1 ; i+=2)
			{
				int value = (ram[i] << 8) | ram[i + 1];
				
				if (i == pc)
					ImGui::PushStyleColor(ImGuiCol_Text, ImColor(IM_COL32(255,0,0,255)));
				
				char instr_buf[128];
				bool valid_instr = SPrintInstr(value, instr_buf);
				
				if (valid_instr)
				{
					ImGui::Text("0x%.3x: 0x%.4x - %s", i, value, instr_buf);
				}
				else
				{
					ImGui::Text("0x%.3x: 0x%.4x", i, value);
				}
				
				if (i == pc)
				{
					ImGui::PopStyleColor();
				}
				
				if (update_address &&
					(i <= gTargetAddress) && (gTargetAddress < (i+1)) &&
					!ImGui::IsItemVisible())
				{
					ImGui::SetScrollHere(0.f);
				}
			}
			
		}
		ImGui::EndChild();
	}
	ImGui::End();
	
	
	ImGui::Begin("Registers", nullptr, 0);
	{
		ImGui::Text("PC: 0x%.3x", pc);
		ImGui::Text("I: 0x%x", I);
		for (int i = 0 ; i < 16 ; ++i)
		{
			ImGui::Text("V%x = 0x%.2x", i, V[i]);
			if ((i & 1) == 0)
			{
				ImGui::SameLine();
			}
		}
	}
	ImGui::End();
	
	ImGui::Begin("Execution Control", nullptr, 0);
	{
		if (ImGui::Button("Play"))
			gFramesToSim = -1;
		ImGui::SameLine();
		if (ImGui::Button("Pause"))
		{
			gFramesToSim = 0;
			gUpdateTargetAddress = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Step"))
		{
			gFramesToSim = 1;
			gUpdateTargetAddress = true;
		}
	}
	ImGui::End();
	
	// Great for reference
	// ImGui::ShowTestWindow();
	
	// TODO: clock speed control - games which don't use the delay timer really need this (KALEID)
	// TODO: edit memory - dependency on flow control being implemented
	// TODO: fix issue with key input going to game while trying to use debug tools - check focus on game window
	// TODO: Dockable windows
	
	ImGui::PopStyleColor();
}

void emu_render()
{
	glBindTexture(GL_TEXTURE_2D, display_tex);
	
	uint pixels[64*32] = {0};
	for (int i = 0 ; i < 64*32 ; ++i)
	{
		pixels[i] = screen[i] ? (0xff << 24 | 0x00FFFFFF) : 0x00 ;
	}
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 32, 0, GL_RGBA, GL_UNSIGNED_BYTE,
		pixels);
	
	glBindTexture(GL_TEXTURE_2D, 0);
}